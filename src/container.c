#include "container.h"
#include "namespace.h"
#include <stdlib.h>
#include <string.h>

// Store multiple namespace by name using a linked list
typedef struct Container {
    Namespace *head;
    Namespace *tail;
    size_t size;
} Container;

Container *create_container(void) {
    Container *container = malloc(sizeof(Container));
    if (!container) {
        return NULL;
    }

    container->head = NULL;
    container->tail = NULL;
    container->size = 0;
    return container;
}

// Add a namespace to the container. Returns 0 on success, -1 on error.
int add_namespace(Container *container, Namespace *ns) {
    if (!container || !ns) {
        return -1;
    }

    // Add the namespace to the container
    if (!container->head) {
        container->head = ns;
    } else {
        container->tail->next = ns;
    }
    container->tail = ns;
    container->size++;
    return 0;
}

// Retrieve a namespace by logical name; returns NULL if not found.
Namespace *get_namespace(Container *container, const char *name) {
    if (!container || !name) {
        return NULL;
    }
    // Find the namespace by name
    Namespace *current = container->head;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void destroy_container(Container *container) {
    if (!container) {
        return;
    }
    // Free each namespace in the container
    Namespace *current = container->head;
    while (current) {
        Namespace *next = current->next;
        destroy_namespace(current);
        current = next;
    }
    free(container);
}