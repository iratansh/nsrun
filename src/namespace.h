// namespace.h - Public API for lightweight Linux-style namespaces
// This header defines the Namespace struct and helpers used by main.c and container.c

#ifndef NSRUN_NAMESPACE_H
#define NSRUN_NAMESPACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>   // size_t
#include <sys/types.h> // pid_t

// Forward-declarable handle for a namespace. Fields are exposed because main.c
// accesses them directly (name, rootfs, command, hostname).
typedef struct Namespace {
	char *name;      // Logical namespace name (e.g., "my_namespace")
	char *rootfs;    // Path to root filesystem to chroot/pivot_root into
	char *command;   // Entrypoint/command to exec inside the namespace
	char *hostname;  // UTS namespace hostname
    Namespace *next; // Pointer to next namespace in a linked list
	// Optional runtime metadata (not required by main.c but useful to have)
	pid_t pid;       // Child/leader pid in this namespace (if applicable)
	int clone_flags; // CLONE_* flags used to create this namespace set
} Namespace;

// Allocate and initialize a Namespace with the provided logical name.
// Returns NULL on allocation failure.
Namespace *create_namespace(const char *name);

// Convenience setters. Return 0 on success, -1 on error (e.g., allocation).
int namespace_set_rootfs(Namespace *ns, const char *rootfs);
int namespace_set_command(Namespace *ns, const char *command);
int namespace_set_hostname(Namespace *ns, const char *hostname);

// Free all memory associated with a Namespace (does not kill processes).
void destroy_namespace(Namespace *ns);

#ifdef __cplusplus
}
#endif

#endif // NSRUN_NAMESPACE_H

