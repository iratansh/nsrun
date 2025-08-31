// network.h - Minimal container networking helpers

#ifndef NSRUN_NETWORK_H
#define NSRUN_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/in.h>
#include <sys/types.h>

// High-level API to set up a veth pair and a simple bridge for a container
// network namespace. Actual implementation details reside in network.c.

// Create a veth pair (host_if, cont_if). Returns 0 on success.
int net_create_veth_pair(const char *host_if, const char *cont_if);

// Create or ensure a linux bridge exists; returns 0 on success.
int net_ensure_bridge(const char *br_name);

// Attach an interface to the bridge; returns 0 on success.
int net_attach_to_bridge(const char *if_name, const char *br_name);

// Move an interface to a target network namespace (by pid); returns 0 on success.
int net_move_if_to_ns(const char *if_name, pid_t target_pid);

// Configure an interface inside a netns with IP/mask and bring it up; 0 on success.
int net_configure_if_in_ns(pid_t target_pid, const char *if_name,
						   const char *cidr, const char *gw);

#ifdef __cplusplus
}
#endif

#endif // NSRUN_NETWORK_H

