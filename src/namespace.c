#include "namespace.h"
#include <stdlib.h>
#include <string.h>

Namespace *create_namespace(const char *name) {
    Namespace *ns = malloc(sizeof(Namespace));
    if (!ns) {
        return NULL;
    }
    ns->name = strdup(name);
    if (!ns->name) {
        free(ns);
        return NULL;
    }
    ns->clone_flags = 0;
    ns->pid = 0;
    ns->rootfs = NULL;
    ns->command = NULL;
    ns->hostname = NULL;
    return ns;
}


int namespace_set_rootfs(Namespace *ns, const char *rootfs) {
    if (!ns || !rootfs) {
        return -1;
    }
    free(ns->rootfs);
    ns->rootfs = strdup(rootfs);
    if (!ns->rootfs) {
        return -1;
    }
    return 0;
}

int namespace_set_command(Namespace *ns, const char *command) {
    if (!ns || !command) {
        return -1;
    }
    free(ns->command);
    ns->command = strdup(command);
    if (!ns->command) {
        return -1;
    }
    return 0;
}

int namespace_set_hostname(Namespace *ns, const char *hostname) {
    if (!ns || !hostname) {
        return -1;
    }
    free(ns->hostname);
    ns->hostname = strdup(hostname);
    if (!ns->hostname) {
        return -1;
    }
    return 0;
}

void destroy_namespace(Namespace *ns) {
    if (!ns) {
        return;
    }
    free(ns->name);
    free(ns->rootfs);
    free(ns->command);
    free(ns->hostname);
    free(ns);
}
