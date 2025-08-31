#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __linux__
#define _GNU_SOURCE
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include "container.h"
#include "namespace.h"
#include "cgroups.h"
#include "network.h"

// stack allocation for child process
#define STACK_SIZE (1024 * 1024) // 1MB
char child_stack[STACK_SIZE];

// Configuration structure for the container
struct ContainerConfig {
    char *rootfs;
    char *command;
    char *hostname;
    unsigned long long memory_limit_bytes;
    long long cpu_quota_us;
    long long cpu_period_us;
    long long pids_max;
    char *bridge_name;
    char *host_if;
    char *cont_if;
    char *cont_ip;
    char *gateway;
};

// Parse command line arguments
int parse_args(int argc, char *argv[], struct ContainerConfig *config) {
    static struct option long_options[] = {
        {"rootfs", required_argument, 0, 'r'},
        {"hostname", required_argument, 0, 'h'},
        {"memory", required_argument, 0, 'm'},
        {"cpu", required_argument, 0, 'c'},
        {"pids", required_argument, 0, 'p'},
        {"bridge", required_argument, 0, 'b'},
        {"ip", required_argument, 0, 'i'},
        {"gateway", required_argument, 0, 'g'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "r:h:m:c:p:b:i:g:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'r':
                config->rootfs = strdup(optarg);
                break;
            case 'h':
                config->hostname = strdup(optarg);
                break;
            case 'm':
                // Parse memory (support M, G suffixes)
                config->memory_limit_bytes = strtoull(optarg, NULL, 10);
                if (strstr(optarg, "M") || strstr(optarg, "m")) {
                    config->memory_limit_bytes *= 1024 * 1024;
                } else if (strstr(optarg, "G") || strstr(optarg, "g")) {
                    config->memory_limit_bytes *= 1024 * 1024 * 1024;
                }
                break;
            case 'c':
                // Parse CPU as fraction (e.g., 0.5 = 50% = 50000/100000)
                {
                    double cpu_fraction = strtod(optarg, NULL);
                    config->cpu_period_us = 100000; // 100ms
                    config->cpu_quota_us = (long long)(cpu_fraction * 100000);
                }
                break;
            case 'p':
                config->pids_max = strtoll(optarg, NULL, 10);
                break;
            case 'b':
                config->bridge_name = strdup(optarg);
                break;
            case 'i':
                config->cont_ip = strdup(optarg);
                break;
            case 'g':
                config->gateway = strdup(optarg);
                break;
            default:
                return -1;
        }
    }

    // Command is the remaining argument
    if (optind < argc) {
        config->command = strdup(argv[optind]);
    }

    return 0;
}

// Child function that runs inside the new namespaces
int child_func(void *arg) {
    struct ContainerConfig *config = (struct ContainerConfig *)arg;

    // Set hostname in UTS namespace
    if (config->hostname && sethostname(config->hostname, strlen(config->hostname)) != 0) {
        perror("sethostname failed");
        return 1;
    }

    // Configure network interface if specified
    if (config->cont_if && config->cont_ip) {
        if (net_configure_if_in_ns(getpid(), config->cont_if, config->cont_ip, config->gateway) != 0) {
            fprintf(stderr, "Failed to configure network interface\n");
            return 1;
        }
    }

    // Change root filesystem
    if (chroot(config->rootfs) != 0) {
        perror("chroot failed");
        return 1;
    }
    if (chdir("/") != 0) {
        perror("chdir failed");
        return 1;
    }

    // Execute the command
    execlp(config->command, config->command, NULL);
    perror("execlp failed");
    return 1;
}

int main(int argc, char *argv[]) {
    // Initialize configuration with defaults
    struct ContainerConfig config = {
        .rootfs = "./rootfs",
        .command = "/bin/sh",
        .hostname = "nsrun-container",
        .memory_limit_bytes = 0, // unlimited
        .cpu_quota_us = 0,       // unlimited
        .cpu_period_us = 0,      // unlimited
        .pids_max = 0,           // unlimited
        .bridge_name = "nsrun-br0",
        .host_if = "veth-host",
        .cont_if = "veth-cont",
        .cont_ip = "10.0.0.2/24",
        .gateway = "10.0.0.1"
    };

    // Parse command line arguments
    if (parse_args(argc, argv, &config) != 0) {
        fprintf(stderr, "Usage: %s --rootfs <path> [--hostname <name>] [--memory <bytes|M|G>] [--cpu <fraction>] [--pids <max>] [--bridge <name>] [--ip <cidr>] [--gateway <ip>] <command>\n", argv[0]);
        return 1;
    }

    // Validate required arguments
    if (!config.rootfs || !config.command) {
        fprintf(stderr, "Error: --rootfs and command are required\n");
        return 1;
    }

    // Create namespace
    Namespace *ns = create_namespace("container-ns");
    if (!ns) {
        fprintf(stderr, "Failed to create namespace\n");
        return 1;
    }

    // Set namespace properties
    if (namespace_set_rootfs(ns, config.rootfs) != 0 ||
        namespace_set_command(ns, config.command) != 0 ||
        namespace_set_hostname(ns, config.hostname) != 0) {
        fprintf(stderr, "Failed to set namespace properties\n");
        destroy_namespace(ns);
        return 1;
    }

    // Create cgroups and apply limits
    char cgroup_path[256];
    snprintf(cgroup_path, sizeof(cgroup_path), "/sys/fs/cgroup/nsrun-%d", getpid());

    if (cgroups_create(cgroup_path) != 0) {
        fprintf(stderr, "Failed to create cgroups\n");
        destroy_namespace(ns);
        return 1;
    }

    // Apply resource limits if specified
    CgroupLimits limits = {
        .memory_limit_bytes = config.memory_limit_bytes,
        .cpu_quota_us = config.cpu_quota_us,
        .cpu_period_us = config.cpu_period_us,
        .pids_max = config.pids_max
    };

    if (cgroups_apply_limits(cgroup_path, &limits) != 0) {
        fprintf(stderr, "Failed to apply cgroup limits\n");
        cgroups_destroy(cgroup_path);
        destroy_namespace(ns);
        return 1;
    }

    // Setup networking if IP is specified
    if (config.cont_ip) {
        // Ensure bridge exists
        if (net_ensure_bridge(config.bridge_name) != 0) {
            fprintf(stderr, "Failed to create bridge\n");
            cgroups_destroy(cgroup_path);
            destroy_namespace(ns);
            return 1;
        }

        // Create veth pair
        if (net_create_veth_pair(config.host_if, config.cont_if) != 0) {
            fprintf(stderr, "Failed to create veth pair\n");
            cgroups_destroy(cgroup_path);
            destroy_namespace(ns);
            return 1;
        }

        // Attach host interface to bridge
        if (net_attach_to_bridge(config.host_if, config.bridge_name) != 0) {
            fprintf(stderr, "Failed to attach interface to bridge\n");
            cgroups_destroy(cgroup_path);
            destroy_namespace(ns);
            return 1;
        }
    }

    // Clone child process with all namespaces
    pid_t pid = clone(child_func, child_stack + STACK_SIZE,
                     CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWNET,
                     &config);
    if (pid == -1) {
        perror("clone failed");
        cgroups_destroy(cgroup_path);
        destroy_namespace(ns);
        return 1;
    }

    // In parent: attach child PID to cgroups
    if (cgroups_attach_pid(cgroup_path, pid) != 0) {
        fprintf(stderr, "Failed to attach PID to cgroups\n");
        // Continue anyway, child might still work
    }

    // Move network interface to child namespace if networking is enabled
    if (config.cont_ip) {
        if (net_move_if_to_ns(config.cont_if, pid) != 0) {
            fprintf(stderr, "Failed to move interface to namespace\n");
            // Continue anyway
        }
    }

    // Store PID in namespace
    ns->pid = pid;

    // Create container and add namespace
    Container *container = create_container();
    if (!container) {
        fprintf(stderr, "Failed to create container\n");
        cgroups_destroy(cgroup_path);
        destroy_namespace(ns);
        return 1;
    }

    if (add_namespace(container, ns) != 0) {
        fprintf(stderr, "Failed to add namespace to container\n");
        destroy_container(container);
        cgroups_destroy(cgroup_path);
        return 1;
    }

    // Wait for child process
    int status;
    waitpid(pid, &status, 0);

    // Cleanup
    destroy_container(container);
    cgroups_destroy(cgroup_path);

    return WEXITSTATUS(status);
}

#else // Not Linux

int main(int argc, char *argv[]) {
    fprintf(stderr, "Error: nsrun requires Linux with namespace and cgroup support.\n");
    fprintf(stderr, "This application uses Linux-specific features that are not available on this platform.\n");
    fprintf(stderr, "Please run this on a Linux system with:\n");
    fprintf(stderr, "  - Linux kernel with namespaces enabled\n");
    fprintf(stderr, "  - Cgroups v1 or v2 support\n");
    fprintf(stderr, "  - Root privileges for namespace operations\n");
    return 1;
}

#endif // __linux__