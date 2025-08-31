#!/bin/bash
# test-nsrun.sh - Test script to run inside the Docker container

set -e

echo "🔧 Testing nsrun compilation..."
make clean && make

echo ""
echo "📋 Available test commands:"
echo "1. Basic container test:"
echo "   ./nsrun --rootfs ./rootfs /bin/sh"
echo ""
echo "2. Container with hostname:"
echo "   ./nsrun --rootfs ./rootfs --hostname test-container /bin/sh" 
echo ""
echo "3. Container with memory limit:"
echo "   ./nsrun --rootfs ./rootfs --memory 64M --hostname mem-test /bin/sh"
echo ""
echo "4. Container with CPU limit:"
echo "   ./nsrun --rootfs ./rootfs --cpu 0.5 --hostname cpu-test /bin/sh"
echo ""
echo "5. Show help:"
echo "   ./nsrun --help"
echo ""

echo "🧪 Running basic test (will exit immediately)..."
echo "Testing: ./nsrun --rootfs ./rootfs --hostname nsrun-test /bin/echo 'Hello from container!'"

# Test basic functionality
if ./nsrun --rootfs ./rootfs --hostname nsrun-test /bin/echo "Hello from container!"; then
    echo "✅ Basic test passed!"
else
    echo "❌ Basic test failed"
    exit 1
fi

echo ""
echo "🎉 Ready to test interactively! Try the commands above."
echo "💡 Tip: Use 'exit' to leave any container shell and return to this environment."