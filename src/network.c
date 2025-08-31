#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

// Validate interface name to prevent command injection
static int validate_if_name(const char *name) {
    if (!name || strlen(name) == 0 || strlen(name) > 15) {
        return -1;
    }
    
    // Only allow alphanumeric, dash, underscore
    for (const char *p = name; *p; p++) {
        if (!isalnum(*p) && *p != '-' && *p != '_') {
            return -1;
        }
    }
    return 0;
}

// Safe system call with proper error checking
static int safe_system(const char *cmd) {
    int ret = system(cmd);
    if (ret == -1) {
        perror("system() failed");
        return -1;
    }
    // Check exit status
    if (WEXITSTATUS(ret) != 0) {
        fprintf(stderr, "Command failed with exit code %d: %s\n", WEXITSTATUS(ret), cmd);
        return -1;
    }
    return 0;
}

// Create a veth pair (host_if, cont_if). Returns 0 on success.
int net_create_veth_pair(const char *host_if, const char *cont_if) {
    if (validate_if_name(host_if) < 0 || validate_if_name(cont_if) < 0) {
        fprintf(stderr, "Invalid interface name\n");
        return -1;
    }

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ip link add %s type veth peer name %s", host_if, cont_if);
    
    if (safe_system(cmd) < 0) {
        fprintf(stderr, "Failed to create veth pair %s-%s\n", host_if, cont_if);
        return -1;
    }

    return 0;
}


// Create or ensure a linux bridge exists; returns 0 on success.
int net_ensure_bridge(const char *br_name) {
    if (validate_if_name(br_name) < 0) {
        fprintf(stderr, "Invalid bridge name\n");
        return -1;
    }

    char cmd[256];
    
    // Check if the bridge already exists by trying to read its state
    snprintf(cmd, sizeof(cmd), "ip link show %s >/dev/null 2>&1", br_name);
    if (system(cmd) == 0) {
        // Bridge exists
        return 0;
    }

    // Create the bridge using ip command (more modern than brctl)
    snprintf(cmd, sizeof(cmd), "ip link add name %s type bridge", br_name);
    if (safe_system(cmd) < 0) {
        fprintf(stderr, "Failed to create bridge %s\n", br_name);
        return -1;
    }

    // Bring the bridge up
    snprintf(cmd, sizeof(cmd), "ip link set %s up", br_name);
    if (safe_system(cmd) < 0) {
        fprintf(stderr, "Failed to bring bridge %s up\n", br_name);
        return -1;
    }

    return 0;
}

// Attach an interface to the bridge; returns 0 on success.
int net_attach_to_bridge(const char *if_name, const char *br_name) {
    if (validate_if_name(if_name) < 0 || validate_if_name(br_name) < 0) {
        fprintf(stderr, "Invalid interface or bridge name\n");
        return -1;
    }

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ip link set %s master %s", if_name, br_name);
    
    if (safe_system(cmd) < 0) {
        fprintf(stderr, "Failed to attach %s to bridge %s\n", if_name, br_name);
        return -1;
    }

    return 0;
}

// Move an interface to a target network namespace (by pid); returns 0 on success.
int net_move_if_to_ns(const char *if_name, pid_t target_pid) {
    if (validate_if_name(if_name) < 0) {
        fprintf(stderr, "Invalid interface name\n");
        return -1;
    }
    
    if (target_pid <= 0) {
        fprintf(stderr, "Invalid target PID\n");
        return -1;
    }

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ip link set %s netns %d", if_name, target_pid);
    
    if (safe_system(cmd) < 0) {
        fprintf(stderr, "Failed to move %s to namespace %d\n", if_name, target_pid);
        return -1;
    }

    return 0;
}

// Configure an interface inside a netns with IP/mask and bring it up; 0 on success.
int net_configure_if_in_ns(pid_t target_pid, const char *if_name,
						   const char *cidr, const char *gw) {
    if (validate_if_name(if_name) < 0) {
        fprintf(stderr, "Invalid interface name\n");
        return -1;
    }
    
    if (target_pid <= 0) {
        fprintf(stderr, "Invalid target PID\n");
        return -1;
    }
    
    if (!cidr) {
        fprintf(stderr, "CIDR cannot be NULL\n");
        return -1;
    }

    char cmd[512];
    
    // Configure the interface with the given IP/mask
    snprintf(cmd, sizeof(cmd), "ip netns exec %d ip addr add %s dev %s", target_pid, cidr, if_name);
    if (safe_system(cmd) < 0) {
        fprintf(stderr, "Failed to configure IP %s on %s in namespace %d\n", cidr, if_name, target_pid);
        return -1;
    }

    // Bring the interface up
    snprintf(cmd, sizeof(cmd), "ip netns exec %d ip link set %s up", target_pid, if_name);
    if (safe_system(cmd) < 0) {
        fprintf(stderr, "Failed to bring %s up in namespace %d\n", if_name, target_pid);
        return -1;
    }

    // Bring up loopback interface
    snprintf(cmd, sizeof(cmd), "ip netns exec %d ip link set lo up", target_pid);
    if (safe_system(cmd) < 0) {
        fprintf(stderr, "Failed to bring up loopback in namespace %d\n", target_pid);
        return -1;
    }

    // Configure the default gateway if provided
    if (gw && strlen(gw) > 0) {
        snprintf(cmd, sizeof(cmd), "ip netns exec %d ip route add default via %s", target_pid, gw);
        if (safe_system(cmd) < 0) {
            fprintf(stderr, "Failed to configure gateway %s in namespace %d\n", gw, target_pid);
            return -1;
        }
    }

    return 0;
}