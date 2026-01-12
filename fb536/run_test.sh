#!/bin/bash
# run_test.sh - Automatic test script for fb536 driver

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║        fb536 Driver Test Automation Script                ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (sudo ./run_test.sh)"
    exit 1
fi

# 1. Remove old module if loaded
echo "[1/6] Removing old module..."
rmmod fb536 2>/dev/null || echo "  No old module to remove"

# 2. Load the module
echo "[2/6] Loading fb536 module..."
insmod fb536.ko
if [ $? -eq 0 ]; then
    echo "  ✓ Module loaded successfully"
else
    echo "  ✗ Failed to load module"
    exit 1
fi

# 3. Get major number from /proc/devices
echo "[3/6] Getting major number..."
MAJOR=$(awk '$2=="fb536" {print $1}' /proc/devices)
if [ -z "$MAJOR" ]; then
    echo "  ✗ Could not find major number in /proc/devices"
    rmmod fb536
    exit 1
fi
echo "  Major number: $MAJOR"

# 4. Create device nodes
echo "[4/6] Creating device nodes..."
rm -f /dev/fb536_* 2>/dev/null
for i in 0 1 2 3; do
    mknod /dev/fb536_$i c $MAJOR $i
    chmod 666 /dev/fb536_$i
done
echo "  ✓ Device nodes created: /dev/fb536_0 - /dev/fb536_3"

# 5. Verify device
echo "[5/6] Verifying device..."
ls -l /dev/fb536_0
if [ ! -c /dev/fb536_0 ]; then
    echo "  ✗ Device node not created properly"
    rmmod fb536
    exit 1
fi
echo "  ✓ Device verified"

# 6. Compile and run test
echo "[6/6] Compiling and running tests..."
if [ ! -f test_fb536.c ]; then
    echo "  ✗ test_fb536.c not found"
    exit 1
fi

gcc -o test_fb536 test_fb536.c -pthread
if [ $? -ne 0 ]; then
    echo "  ✗ Compilation failed"
    exit 1
fi
echo "  ✓ Test program compiled"
echo ""
echo "════════════════════════════════════════════════════════════"
echo "                 Running Test Suite                         "
echo "════════════════════════════════════════════════════════════"
echo ""

./test_fb536
TEST_RESULT=$?

echo ""
echo "════════════════════════════════════════════════════════════"
if [ $TEST_RESULT -eq 0 ]; then
    echo "  ✓✓✓ ALL TESTS PASSED ✓✓✓"
else
    echo "  ✗✗✗ SOME TESTS FAILED ✗✗✗"
fi
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Module info:"
lsmod | grep fb536
echo ""
echo "To remove module: sudo rmmod fb536"
echo "To view kernel logs: dmesg | tail -50"

exit $TEST_RESULT
