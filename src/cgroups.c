#include "cgroups.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>


// Returns 0 on success, -1 on error.
int cgroups_create(const char *name) {
    if (!name) {
        return -1;
    }

    if (mkdir(name, 0755) != 0) {
        perror("mkdir");
        return -1;
    }


    return 0;
}

// Apply limits to an existing cgroup. Returns 0 on success, -1 on error.
int cgroups_apply_limits(const char *name, const CgroupLimits *limits) {
    if (!name || !limits) {
        return -1;
    }

    char path[256];
    int fd;

    // Apply memory limit if set
    if (limits->memory_limit_bytes > 0) {
        snprintf(path, sizeof(path), "%s/memory.limit_in_bytes", name);
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            perror("open memory.limit_in_bytes");
            return -1;
        }
        if (dprintf(fd, "%llu", limits->memory_limit_bytes) < 0) {
            perror("dprintf memory.limit_in_bytes");
            close(fd);
            return -1;
        }
        close(fd);
    }

    // Apply CPU quota and period if set
    if (limits->cpu_quota_us > 0 && limits->cpu_period_us > 0) {
        // Set CPU period
        snprintf(path, sizeof(path), "%s/cpu.cfs_period_us", name);
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            perror("open cpu.cfs_period_us");
            return -1;
        }
        if (dprintf(fd, "%lld", limits->cpu_period_us) < 0) {
            perror("dprintf cpu.cfs_period_us");
            close(fd);
            return -1;
        }
        close(fd);

        // Set CPU quota
        snprintf(path, sizeof(path), "%s/cpu.cfs_quota_us", name);
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            perror("open cpu.cfs_quota_us");
            return -1;
        }
        if (dprintf(fd, "%lld", limits->cpu_quota_us) < 0) {
            perror("dprintf cpu.cfs_quota_us");
            close(fd);
            return -1;
        }
        close(fd);
    }

    // Apply PIDs limit if set
    if (limits->pids_max > 0) {
        snprintf(path, sizeof(path), "%s/pids.max", name);
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            perror("open pids.max");
            return -1;
        }
        if (dprintf(fd, "%lld", limits->pids_max) < 0) {
            perror("dprintf pids.max");
            close(fd);
            return -1;
        }
        close(fd);
    }

    return 0;
}

// Attach a process (pid) to the cgroup. Returns 0 on success, -1 on error.
int cgroups_attach_pid(const char *name, int pid) {
    if (!name || pid < 0) {
        return -1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/tasks", name);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    if (dprintf(fd, "%d", pid) < 0) {
        perror("dprintf");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

// Destroy/remove a cgroup. Returns 0 on success, -1 on error.
int cgroups_destroy(const char *name) {
    if (!name) {
        return -1;
    }

    if (rmdir(name) != 0) {
        perror("rmdir");
        return -1;
    }

    return 0;
}
