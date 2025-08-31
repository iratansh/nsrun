// container.h - Container composition and lifecycle minimal API

#ifndef NSRUN_CONTAINER_H
#define NSRUN_CONTAINER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// Forward declaration to avoid header cycles
struct Namespace;
typedef struct Namespace Namespace;

// A very small container structure that owns/organizes one or more namespaces.
typedef struct Container Container;

// Create/destroy a container instance.
Container *create_container(void);
void destroy_container(Container *container);

// Add a namespace to the container. Returns 0 on success, -1 on error.
int add_namespace(Container *container, Namespace *ns);

// Retrieve a namespace by logical name; returns NULL if not found.
Namespace *get_namespace(Container *container, const char *name);

#ifdef __cplusplus
}
#endif

#endif // NSRUN_CONTAINER_H

