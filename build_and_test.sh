#!/bin/bash
# Quick build and test script

set -e

echo "=== Building HFT OrderBook ==="

# Create build directory
mkdir -p build
cd build

# Configure and build
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build . -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)

echo ""
echo "=== Running C++ Tests ==="
./orderbook_test

echo ""
echo "=== Running C++ Benchmarks ==="
./orderbook_benchmark

echo ""
echo "=== Running Python Example ==="
cd ..
python3 python/example.py

echo ""
echo "=== Running Python Tests ==="
python3 python/test_orderbook.py

echo ""
echo "=== Build and Test Complete ==="
