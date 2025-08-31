// cgroups.h - Minimal cgroups configuration API for nsrun

#ifndef NSRUN_CGROUPS_H
#define NSRUN_CGROUPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// Generic cgroup v1/v2 limits supported by this minimal header. The .c can
// decide how to map these into the actual filesystem and controllers.
typedef struct CgroupLimits {
	// Memory: bytes; 0 means unlimited/not set
	unsigned long long memory_limit_bytes;

	// CPU: quota/period (cfs). 0 values mean not set.
	long long cpu_quota_us;  // e.g., 50000 for 50ms quota
	long long cpu_period_us; // e.g., 100000 for 100ms period

	// PIDs: maximum number of processes; 0 means unlimited
	long long pids_max;
} CgroupLimits;

// Create a cgroup for a container or namespace. "name" is used for the cgroup path.
// Returns 0 on success, -1 on error.
int cgroups_create(const char *name);

// Apply limits to an existing cgroup. Returns 0 on success, -1 on error.
int cgroups_apply_limits(const char *name, const CgroupLimits *limits);

// Attach a process (pid) to the cgroup. Returns 0 on success, -1 on error.
int cgroups_attach_pid(const char *name, int pid);

// Destroy/remove a cgroup. Returns 0 on success, -1 on error.
int cgroups_destroy(const char *name);

#ifdef __cplusplus
}
#endif

#endif // NSRUN_CGROUPS_H

