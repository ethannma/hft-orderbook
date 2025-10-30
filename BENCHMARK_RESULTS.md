# HFT OrderBook - Benchmark Results

## System Configuration

- **OS**: macOS
- **Compiler**: Apple Clang 17.0
- **Build Type**: Release (-O3 -march=native)
- **C++ Standard**: C++17
- **Execution**: Single-threaded

## Performance Results

### Order Insertion
- **Throughput**: ~3.0M orders/sec
- **Latency (mean)**: 0.336 µs
- **Latency (p50)**: 0.21 µs
- **Latency (p90)**: 0.54 µs  
- **Latency (p99)**: 1.33 µs

### Order Cancellation
- **Throughput**: ~1.2M cancels/sec
- **Latency (mean)**: 0.812 µs
- **Latency (p50)**: 0.58 µs
- **Latency (p90)**: 1.08 µs
- **Latency (p99)**: 1.58 µs
- **Note**: Slower than inserts due to lookup/cleanup overhead

### Order Matching (Aggressive Orders)
- **Throughput**: ~6.4M orders/sec
- **Latency (mean)**: 0.155 µs
- **Latency (p50)**: 0.13 µs
- **Latency (p90)**: 0.21 µs
- **Latency (p99)**: 0.42 µs
- **Fill ratio**: ~2 trades per aggressive order

### Market Data Queries

#### Market Depth (10 levels)
- **Throughput**: ~12.3M queries/sec
- **Latency**: ~0.082 µs/query

*Note: Best bid/ask queries are near timer resolution and not reported separately*

## Benchmark Methodology

### Anti-Optimization Measures
- **noinline** attribute on query functions
- Volatile sinks for result accumulation
- Compiler memory barriers (`asm volatile`)
- Empty loop baseline subtraction
- 16 queries per iteration to amortize loop overhead

### Measurement
- **Warmup**: 1K-100K operations before timing
- **Percentiles**: nth_element on individual operation timings
- **Timer**: std::chrono::high_resolution_clock
- **Sample sizes**: 100K-10M operations depending on speed

## Correctness Verification

### Unit Tests (18/18 passing)
- Price priority (best prices match first)
- Time priority (FIFO within price levels)
- Time priority loss on order quantity increases
- Full and partial fills
- Market and limit orders
- Order modifications and cancellations
- Trade price correctness (passive order price)

### Invariant Checks
- Best bid < best ask (when both exist)
- All quantities > 0
- Trade count monotonically increasing
- Sum of price level volumes equals total volume

## Architecture

### Data Structures
- **Price Levels**: `std::map<double, PriceLevel>` - O(log n) sorted lookup
  - Bids: high→low (std::greater)
  - Asks: low→high (std::less)
- **Order Queue**: `std::deque<Order*>` - O(1) FIFO operations
- **Order Lookup**: `std::unordered_map<uint64_t, Order*>` - O(1) by ID

### Matching Engine
- **Price-time priority**: Best price first, FIFO within levels
- **Execution price**: Passive order's price (price improvement)
- **Order types**: Limit and market orders supported
- **Partial fills**: Supported with quantity tracking

## Summary

Single-threaded C++17 limit order book with price-time priority matching. Benchmarked on macOS with Clang 17 (-O3): ~3.0M inserts/s (p99 1.33 µs), ~1.2M cancels/s (p99 1.58 µs), ~6.4M aggressive orders/s match (p99 0.42 µs), 10-level depth ~0.082 µs/query; best bid/ask O(1) and below timer resolution when hot.

## Detailed Breakdown

**Performance (Single-threaded, macOS, Clang 17, -O3):**
- Order Insertion: ~3.0M ops/sec (mean: 0.336 µs, p99: 1.33 µs)
- Order Cancellation: ~1.2M ops/sec (mean: 0.812 µs, p99: 1.58 µs) *Slower than inserts due to lookup/cleanup costs*
- Order Matching: ~6.4M ops/sec (mean: 0.155 µs, p99: 0.42 µs)
- Market Depth (10 levels): ~12.3M queries/sec (~0.082 µs per query)
- Best Bid/Ask: O(1) pointer deref, sub-100ns when hot (below timer resolution)

**Architecture:**
- Price-time priority with O(log n) level lookup, O(1) order operations

## Performance Notes

### Cancel Latency Analysis
Current cancel performance (~1.2M ops/s) is slower than insert performance (~3.0M ops/s). This is expected due to:
- Hash map lookup overhead for order ID
- Deque iterator invalidation on erase
- Memory deallocation on hot path

### Optimization Opportunities (Future Work)
To improve cancel performance to match or exceed insert performance:

1. **Intrusive Data Structures**
   - Use intrusive linked lists instead of `std::deque` to avoid iterator invalidation
   - Eliminate shared_ptr destructor overhead on cancellation path

2. **Memory Management**
   - Implement per-side free lists to recycle Order objects
   - Defer memory deallocation to batch cleanup phase
   - Reserve hash map capacity to avoid rehashing

3. **Lookup Optimization**
   - Store direct node pointers in `unordered_map<id, Order*>` with reserved capacity
   - Use pointer-based erase operations instead of iterator-based

4. **Benchmark Improvements**
   - Thread pinning for consistent CPU scheduling
   - Multiple trial runs to reduce variance
   - Warmer cache state for representative measurements
