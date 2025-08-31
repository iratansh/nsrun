#!/bin/bash
# docker-run.sh - Easy script to run nsrun in Docker with proper privileges

set -e

echo "🐳 Building nsrun Docker container..."
docker build -t nsrun-dev .

echo "🚀 Running nsrun in privileged Docker container..."
echo "📝 Note: Privileged mode is required for namespace and cgroup operations"
echo ""

# Run with privileged access for namespace operations
docker run -it --rm \
    --privileged \
    --pid=host \
    --name nsrun-test \
    nsrun-dev

echo "✅ Container exited"