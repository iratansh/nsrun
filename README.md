## nsrun

nsrun is a tiny, learning-focused container runtime that shows how Linux containers work using namespaces, cgroups, and basic networking. It does not use a VM; it launches a regular Linux process that believes it runs in its own isolated environment.

This repo is a work-in-progress intended for exploration and education, not production use.

### Goals

- Create and manage Linux namespaces (UTS/hostname, PID, mount; later net)
- Change root filesystem into a provided rootfs directory (chroot/pivot_root)
- Apply basic resource limits using cgroups (memory/CPU/PIDs)
- Set up simple container networking with a veth pair and an optional bridge
- Exec a specified command inside this isolated environment

Example target CLI (design-stage):

- nsrun --rootfs ./alpine-rootfs /bin/sh
- nsrun --rootfs ./nginx-rootfs --hostname web --memory 256M --cpu 0.5 /usr/sbin/nginx
- nsrun --rootfs ./app-rootfs --port 8080:80 --detach /start.sh

## Status

- Header files define the public APIs and data shapes.
- .c files are intentionally empty for step-by-step implementation.
- main.c sketches the orchestration; Linux-only features are referenced (clone, CLONE_*).

## Layout

- src/
  - main.c        — prototype entrypoint to orchestrate container setup
  - container.[ch] — container object that groups one or more namespaces
  - namespace.[ch] — Namespace struct and helpers (name, rootfs, command, hostname)
  - cgroups.[ch]  — minimal API to create/apply/destroy cgroups and attach pids
  - network.[ch]  — minimal API to set up veth pairs, bridges, and netns wiring
- rootfs/         — put your minimal root filesystem here (e.g., Alpine minirootfs)
- Makefile        — simple build script (see notes)

## Build (Linux only)

Prereqs:

- Linux kernel with namespaces and cgroups enabled
- GCC/Clang, make, glibc headers

Build from the project root:

```bash
make clean && make
```

## Usage

nsrun is a complete container runtime that creates isolated processes using Linux namespaces and cgroups:

```bash
sudo ./nsrun --rootfs ./alpine-rootfs /bin/sh
```

### Supported flags:

- `--rootfs <dir>`       Path to the container root filesystem (required)
- `--hostname <name>`    UTS namespace hostname
- `--memory <bytes|M|G>` Memory limit via cgroups (e.g., 256M, 1G)
- `--cpu <fraction>`     CPU share via CFS quota/period (e.g., 0.5 for 50%)
- `--pids <max>`         Maximum number of processes
- `--bridge <name>`      Bridge name for networking
- `--ip <cidr>`          Container IP address (e.g., 10.0.0.2/24)
- `--gateway <ip>`       Default gateway IP

### Examples:

```bash
# Basic container
sudo ./nsrun --rootfs ./alpine-rootfs /bin/sh

# With resource limits
sudo ./nsrun --rootfs ./alpine-rootfs --memory 256M --cpu 0.5 --hostname web-container /bin/sh

# With networking
sudo ./nsrun --rootfs ./alpine-rootfs --ip 10.0.0.2/24 --gateway 10.0.0.1 /bin/sh
```

## Usage (target behavior)

Once implemented, nsrun should be usable as:

```sh
sudo ./nsrun --rootfs ./alpine-rootfs /bin/sh
```

Planned flags:

- --rootfs `<dir>`       Path to the container root filesystem
- --hostname `<name>`    UTS namespace hostname
- --memory <bytes|M>   Memory limit via cgroups
- --cpu `<fraction>`     CPU share via CFS quota/period
- --port <h:p>         Simple host:container port mapping (via NAT)
- --detach             Run in background

## Rootfs quickstart (example)

Use a minimal Alpine Linux minirootfs (matching your CPU arch). Extract into rootfs/ so that /bin/sh exists within the rootfs.

Example layout:

```
rootfs/
  bin/
  etc/
  lib/
  usr/
  ...
```

## Architecture

- **namespace.[ch]**
  - Defines Namespace with name, rootfs, command, hostname, plus optional pid/clone_flags
  - create_namespace/destroy_namespace + simple setters
- **container.[ch]**
  - Opaque Container that can add/get Namespace instances by name
- **cgroups.[ch]**
  - CgroupLimits (memory, cpu quota/period, pids) + helpers to create/apply/attach/destroy
- **network.[ch]**
  - Helpers to create veth pairs, manage a bridge, move ifaces to a netns, configure IP, bring links up
- **main.c**
  - Parses args, creates namespaces, sets hostname, chroot, applies cgroups, sets up networking, execs command
  - Proper error handling and resource cleanup
  - Cross-platform detection (Linux-only with helpful error messages)

## Caveats

- Requires root privileges for namespace, cgroup, and network operations
- Linux-only (with helpful error messages on other platforms)
- Behavior may differ across distros/kernels (cgroup v1 vs v2)
- Educational code; not recommended for production use
- Requires a properly set up rootfs with necessary binaries

## Testing

The application includes cross-platform detection - it will provide clear error messages when run on non-Linux systems. On Linux systems with proper setup:

1. Ensure you have root privileges
2. Set up a minimal rootfs (e.g., Alpine Linux minirootfs)
3. Run: `sudo ./nsrun --rootfs ./rootfs /bin/sh`

### Example test session:

```bash
# Build the application
make clean && make

# Test basic functionality (will show Linux requirement on macOS)
./nsrun --help

# On Linux with rootfs:
sudo ./nsrun --rootfs ./alpine-rootfs --hostname test-container /bin/sh
```

## Background

A container runtime is fundamentally about process isolation. When you run something like:

```sh
docker run alpine /bin/sh
```

you're starting a regular Linux process with namespaces and cgroups configured so it appears to run in its own world. nsrun builds that up from first principles.
