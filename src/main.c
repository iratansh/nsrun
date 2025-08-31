#include <errno.h>
#include <stdio.h>
#include <sys/_types/_null.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>
#include "container.h"
#include "namespace.h"

// Container Run Command: 
// # Basic usage
// nsrun --rootfs ./alpine-rootfs /bin/sh
// nsrun --rootfs ./nginx-rootfs /usr/sbin/nginx

// # With options
// nsrun --rootfs ./alpine-rootfs --memory 256M --cpu 0.5 --hostname web-container /bin/sh
// nsrun --rootfs ./app-rootfs --port 8080:80 --detach /start.sh

// stack allocation for child process
#define STACK_SIZE (1024 * 1024) // 1MB
char child_stack[STACK_SIZE];

// Data to pass between functions: container config (rootfs path, command, hostname)
struct ArgsData {
    char *rootfs;
    char *command;
    char *hostname;
};

char *parse_args(int argc, char *argv[])
{
    // Simple argument parsing
    if (argc < 2) {
        return NULL;
    }
    return argv[1];
}

char *parse_input(char *input)
{
    fgets(input, 256, stdin);
    return input;
}

// Child function that runs inside the new namespaces
void *child_func(void *arg)
{
    struct ArgsData *data = (struct ArgsData *)arg;

    // Error handling
    if (chroot(data->rootfs) != 0) {
        perror("chroot failed");
        return NULL;
    }
    if (chdir("/") != 0) {
        perror("chdir failed");
        return NULL;
    }

    // Execute the command
    execlp(data->command, data->command, NULL);
    perror("execlp failed");
    exit(1);
}

int main(int argc, char *argv[])
{
    // Command Line Validation
    char *namespace_name = parse_args(argc, argv);
    if (!namespace_name) {
        fprintf(stderr, "Usage: %s <namespace_name>\n", argv[0]);
        return 1;
    }

    // Create a new namespace
    Namespace *ns = create_namespace("my_namespace");
    if (!ns) {
        fprintf(stderr, "Failed to create namespace\n");
        return 1;
    }

    namespace_set_rootfs(ns, "./rootfs");
    namespace_set_command(ns, "/bin/sh");
    namespace_set_hostname(ns, "my_namespace");

    // Set the namespace name
    ns->name = namespace_name;

    // Set up the arguments for the child process
    struct ArgsData args;
    args.rootfs = "./rootfs";
    args.command = "/bin/sh";
    args.hostname = "my_namespace";

    pid_t pid = clone(child_func, child_stack + STACK_SIZE, CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS, &args);
    if (pid == -1) {
        perror("clone failed");
        destroy_namespace(ns);
        return 1;
    }
    waitpid(pid, NULL, 0);

    // Initialize the container
    Container *container = create_container();

    // Add the namespace to the container
    add_namespace(container, ns);

    // Retrieve the namespace from the container
    Namespace *retrieved_ns = get_namespace(container, "my_namespace");
    if (retrieved_ns) {
        printf("Namespace '%s' found!\n", retrieved_ns->name);
    } else {
        printf("Namespace not found.\n");
    }

    // Clean up
    destroy_container(container);
    return 0;
}