# High-Frequency Trading OrderBook

A high-performance limit order book implementation in modern C++ with Python bindings. This project explores low-latency data structures and matching engine algorithms used in electronic trading systems.

## Features

- **Low-Latency Matching Engine**: Sub-microsecond order processing with price-time priority
- **Modern C++17**: Efficient STL containers and memory management
- **Python Bindings**: Full Python API via pybind11 for testing and analysis
- **Comprehensive Testing**: Unit tests using Google Test framework
- **Performance Benchmarks**: Detailed latency and throughput measurements
- **Market Data Queries**: Real-time best bid/ask, market depth, and spread calculations

## Technical Highlights

### Core Components

- **OrderBook**: Main order matching engine with FIFO price-time priority
- **Price Levels**: Efficient O(log n) price level lookup using `std::map`
- **Order Queue**: O(1) order insertion/removal within price levels using `std::deque`
- **Trade Execution**: Automatic matching with price improvement for aggressive orders

### Supported Operations

- Limit orders (with partial fills)
- Market orders
- Order cancellation
- Order modification (enforces time priority loss on quantity increases)
- Market depth queries (top N levels)
- Volume aggregation at price levels
- Real-time spread and mid-price calculations

## Performance

Benchmarked on macOS with Apple Clang 17.0, Release build (-O3), single-threaded:

| Operation | Mean | Median (p50) | p90 | p99 | Throughput |
|-----------|------|--------------|-----|-----|------------|
| Order Insertion | 0.336 µs | 0.21 µs | 0.54 µs | 1.33 µs | ~3.0M ops/sec |
| Order Cancellation | 0.812 µs | 0.58 µs | 1.08 µs | 1.58 µs | ~1.2M ops/sec |
| Order Matching | 0.155 µs | 0.13 µs | 0.21 µs | 0.42 µs | ~6.4M ops/sec |
| Market Depth (10 levels) | ~0.082 µs | - | - | - | ~12.3M queries/sec |
| Best Bid/Ask | O(1)* | - | - | - | Sub-100ns when hot |

*O(1) pointer dereference; below timer resolution

**Benchmark Configuration:**
- Single-threaded execution with warmup
- Apple Clang 17.0, -O3 optimization
- macOS on Apple Silicon

**Note:** Cancel performance is slower than insert due to hash map lookup and cleanup overhead. See [BENCHMARK_RESULTS.md](BENCHMARK_RESULTS.md) for optimization opportunities.

**Key Features:**
- Sub-microsecond median latencies for all core operations
- Millions of operations per second throughput
- Price-time priority matching with FIFO order execution
- O(log n) price level lookup, O(1) order operations within levels
- Efficient memory layout for cache locality

**Architecture:**
- `std::map` for sorted price levels (bid: high→low, ask: low→high)
- `std::deque` for FIFO order queues at each price
- `std::unordered_map` for O(1) order ID lookups

*Note: Performance depends on hardware, order book depth, and workload patterns*

## Building the Project

### Prerequisites

- CMake 3.15+
- C++17 compatible compiler (GCC 7+, Clang 5+, or MSVC 2019+)
- Python 3.7+ (for Python bindings)

### Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build . -j$(nproc)

# Run C++ tests
./orderbook_test

# Run C++ benchmarks
./orderbook_benchmark
```

### Building Python Bindings

```bash
# Install pybind11 (one-time setup)
pip3 install --user pybind11

# Build project (Python bindings included automatically)
cmake -B build && cmake --build build -j

# Run Python examples
python3 python/example.py
python3 python/benchmark.py
python3 python/test_orderbook.py
```

## Usage Examples

### C++ API

```cpp
#include "orderbook.hpp"
using namespace hft;

// Create orderbook
OrderBook ob("AAPL");

// Add limit orders
ob.add_limit_order(1, Side::BUY, 150.00, 100);
ob.add_limit_order(2, Side::SELL, 150.10, 100);

// Query market data
auto best_bid = ob.get_best_bid();
auto best_ask = ob.get_best_ask();
auto spread = ob.get_spread();

// Get market depth
auto bids = ob.get_bids(5);  // Top 5 bid levels
auto asks = ob.get_asks(5);  // Top 5 ask levels

// Execute trades
ob.add_market_order(3, Side::BUY, 50);

// Cancel orders
ob.cancel_order(1);
```

### Python API

```python
from pyorderbook import OrderBook, Side

# Create orderbook
ob = OrderBook("AAPL")

# Add orders
ob.add_limit_order(1, Side.BUY, 150.00, 100)
ob.add_limit_order(2, Side.SELL, 150.10, 100)

# Query market data
print(f"Best bid: ${ob.get_best_bid():.2f}")
print(f"Best ask: ${ob.get_best_ask():.2f}")
print(f"Spread: ${ob.get_spread():.4f}")

# Get market depth
for price, volume in ob.get_bids(5):
    print(f"  ${price:.2f} - {volume} shares")

# View trades
for trade in ob.get_trades():
    print(f"Trade: ${trade.price:.2f} x {trade.quantity}")
```

## Architecture

### Data Structures

```
OrderBook
├── bids_: map<double, PriceLevel, greater<>>  // Sorted highest to lowest
├── asks_: map<double, PriceLevel>             // Sorted lowest to highest
├── orders_: unordered_map<uint64_t, Order*>   // Fast order lookup
└── trades_: vector<Trade>                     // Trade history

PriceLevel
├── price: double
├── total_volume: uint64_t
└── orders: deque<Order*>                      // FIFO queue
```

### Matching Algorithm

1. **Price Priority**: Best prices match first (highest bid, lowest ask)
2. **Time Priority**: Orders at same price level match in FIFO order
3. **Pro-rata not supported**: Pure FIFO within price levels
4. **Execution Price**: Passive order's price (price improvement for aggressor)

## Testing

### Run C++ Tests

```bash
cd build
./orderbook_test
```

### Run Python Tests

```bash
python3 python/test_orderbook.py
```

### Run Benchmarks

```bash
# C++ benchmarks
cd build
./orderbook_benchmark

# Python benchmarks
python3 python/benchmark.py
```

## Project Structure

```
hft-orderbook/
├── include/
│   └── orderbook.hpp          # OrderBook header
├── src/
│   └── orderbook.cpp          # OrderBook implementation
├── python/
│   ├── bindings.cpp           # pybind11 bindings
│   ├── example.py             # Usage examples
│   ├── benchmark.py           # Performance benchmarks
│   └── test_orderbook.py      # Python unit tests
├── tests/
│   └── test_orderbook.cpp     # C++ unit tests
├── benchmarks/
│   └── benchmark.cpp          # C++ benchmarks
├── CMakeLists.txt             # Build configuration
├── LICENSE                    # MIT License
└── README.md                  # This file
```

## Key Concepts

### Order Book Mechanics
- **Price-time priority**: Best price first, FIFO within price levels
- **Market microstructure**: Limit orders, market orders, partial fills
- **Trade execution**: Price improvement for aggressive orders

### Technical Implementation
- **Low-latency design**: Optimized data structures for microsecond-level performance
- **Memory management**: Efficient use of smart pointers and STL containers
- **Cache efficiency**: Data locality with contiguous storage (deque vs list)
- **C++/Python integration**: Modern FFI techniques with pybind11
- **Performance benchmarking**: Percentile-based latency analysis

## Future Enhancements

- [ ] Order book snapshots and replay
- [ ] Market maker protection (self-trade prevention)
- [ ] Stop and iceberg orders
- [ ] VWAP and TWAP calculation
- [ ] FIX protocol integration
- [ ] Lock-free concurrent orderbook
- [ ] Memory pooling and intrusive data structures for improved cancel performance

## License

MIT License - see LICENSE file for details
