#!/bin/bash
# ==============================================================================
# SNAP TTS Engine - macOS (Apple Silicon M1/M2/M3 & Intel) Build & Test Script
# ==============================================================================
set -e

echo "=================================================================="
echo "  SNAP TTS Engine - macOS Build & E2E Verification Script"
echo "=================================================================="

# Check CMake availability
if ! command -v cmake &> /dev/null; then
    echo "[ERROR] 'cmake' is not installed. Please install it via 'brew install cmake'"
    exit 1
fi

# Directory setup
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/snap_cpp/build_macos"

echo "[1/3] Configuring C++ CMake build for macOS (Universal2 / arm64 / x86_64)..."
cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}/snap_cpp" -DCMAKE_BUILD_TYPE=Release

echo "[2/3] Compiling libsnap_cpp.dylib and test_e2e binary..."
cmake --build "${BUILD_DIR}" --config Release -j$(sysctl -n hw.ncpu)

echo "[3/3] Running macOS C++ Native E2E Test..."
"${BUILD_DIR}/test_e2e" "${SCRIPT_DIR}" ko "NVIDIA GeForce RTX 4090 및 i7 13700K 프로세서를 15층에서 구매했다."

echo "=================================================================="
echo "  [SUCCESS] macOS C++ Build and E2E Test completed successfully!"
echo "=================================================================="
