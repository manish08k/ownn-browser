#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUILD_TYPE="${1:-Release}"
BUILD_TESTS="${2:-OFF}"

echo "=== Nova Browser Build ==="
echo "Build type : ${BUILD_TYPE}"
echo "Build tests: ${BUILD_TESTS}"
echo ""

# Configure
cmake -B "${BUILD_DIR}" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DBUILD_TESTS="${BUILD_TESTS}" \
    "${SCRIPT_DIR}"

# Build
cmake --build "${BUILD_DIR}" --parallel "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo ""
echo "=== Build complete ==="
echo "Binary: ${BUILD_DIR}/NovaBrowser"

if [ "${BUILD_TESTS}" = "ON" ]; then
    echo ""
    echo "=== Running tests ==="
    ctest --test-dir "${BUILD_DIR}" --output-on-failure
fi
