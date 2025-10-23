#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>
#include <vector>
#include <algorithm>
#include "orderbook.hpp"

using namespace hft;

// Memory barrier to prevent reordering
#define COMPILER_BARRIER() asm volatile("" : : : "memory")

class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
    
    double elapsed_us() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::micro>(end - start_).count();
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_;
};

double calculate_percentile(std::vector<double>& latencies, double percentile) {
    size_t n = latencies.size();
    size_t index = static_cast<size_t>(percentile / 100.0 * n);
    if (index >= n) index = n - 1;
    std::nth_element(latencies.begin(), latencies.begin() + index, latencies.end());
    return latencies[index];
}

void benchmark_order_insertion(size_t num_orders) {
    OrderBook ob("BENCHMARK");
    std::mt19937 gen(42);
    std::uniform_real_distribution<> price_dist(99.0, 101.0);
    std::uniform_int_distribution<> qty_dist(1, 100);
    std::uniform_int_distribution<> side_dist(0, 1);
    
    std::vector<double> latencies;
    latencies.reserve(num_orders);
    
    // Warmup
    for (size_t i = 0; i < 1000; ++i) {
        Side side = side_dist(gen) == 0 ? Side::BUY : Side::SELL;
        ob.add_limit_order(i, side, price_dist(gen), qty_dist(gen));
    }
    
    OrderBook ob2("BENCHMARK");
    Timer total_timer;
    for (size_t i = 0; i < num_orders; ++i) {
        Side side = side_dist(gen) == 0 ? Side::BUY : Side::SELL;
        double price = price_dist(gen);
        uint64_t qty = qty_dist(gen);
        
        Timer timer;
        ob2.add_limit_order(i, side, price, qty);
        latencies.push_back(timer.elapsed_us());
    }
    double elapsed = total_timer.elapsed_ms();
    
    std::cout << "Order Insertion Benchmark:\n";
    std::cout << "  Total orders: " << num_orders << "\n";
    std::cout << "  Time: " << std::fixed << std::setprecision(2) << elapsed << " ms\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(0) 
              << (num_orders / elapsed * 1000.0) << " orders/sec\n";
    std::cout << "  Latency (mean): " << std::fixed << std::setprecision(3) 
              << (elapsed * 1000.0 / num_orders) << " µs/order\n";
    std::cout << "  Latency (p50): " << std::fixed << std::setprecision(3) 
              << calculate_percentile(latencies, 50) << " µs\n";
    std::cout << "  Latency (p90): " << std::fixed << std::setprecision(3) 
              << calculate_percentile(latencies, 90) << " µs\n";
    std::cout << "  Latency (p99): " << std::fixed << std::setprecision(3) 
              << calculate_percentile(latencies, 99) << " µs\n\n";
}

void benchmark_order_cancellation(size_t num_orders) {
    OrderBook ob("BENCHMARK");
    std::mt19937 gen(42);
    std::uniform_real_distribution<> price_dist(99.0, 101.0);
    std::uniform_int_distribution<> qty_dist(1, 100);
    
    // Insert orders
    for (size_t i = 0; i < num_orders; ++i) {
        ob.add_limit_order(i, Side::BUY, price_dist(gen), qty_dist(gen));
    }
    
    std::vector<double> latencies;
    latencies.reserve(num_orders);
    
    // Benchmark cancellation
    Timer total_timer;
    for (size_t i = 0; i < num_orders; ++i) {
        Timer timer;
        ob.cancel_order(i);
        latencies.push_back(timer.elapsed_us());
    }
    double elapsed = total_timer.elapsed_ms();
    
    std::cout << "Order Cancellation Benchmark:\n";
    std::cout << "  Total orders: " << num_orders << "\n";
    std::cout << "  Time: " << std::fixed << std::setprecision(2) << elapsed << " ms\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(0) 
              << (num_orders / elapsed * 1000.0) << " cancels/sec\n";
    std::cout << "  Latency (mean): " << std::fixed << std::setprecision(3) 
              << (elapsed * 1000.0 / num_orders) << " µs/cancel\n";
    std::cout << "  Latency (p50): " << std::fixed << std::setprecision(3) 
              << calculate_percentile(latencies, 50) << " µs\n";
    std::cout << "  Latency (p90): " << std::fixed << std::setprecision(3) 
              << calculate_percentile(latencies, 90) << " µs\n";
    std::cout << "  Latency (p99): " << std::fixed << std::setprecision(3) 
              << calculate_percentile(latencies, 99) << " µs\n\n";
}

void benchmark_matching_engine(size_t num_orders) {
    OrderBook ob("BENCHMARK");
    std::mt19937 gen(42);
    std::uniform_int_distribution<> qty_dist(1, 100);
    
    // Place resting orders on both sides
    for (size_t i = 0; i < num_orders / 2; ++i) {
        ob.add_limit_order(i, Side::BUY, 99.0 + (i % 10) * 0.01, qty_dist(gen));
        ob.add_limit_order(num_orders / 2 + i, Side::SELL, 101.0 + (i % 10) * 0.01, qty_dist(gen));
    }
    
    std::vector<double> latencies;
    size_t matches = 1000;
    latencies.reserve(matches);
    
    // Benchmark aggressive orders that match
    Timer total_timer;
    for (size_t i = num_orders; i < num_orders + matches; ++i) {
        bool buy = (i % 2) == 0;
        
        Timer timer;
        if (buy) {
            ob.add_limit_order(i, Side::BUY, 102.0, qty_dist(gen));
        } else {
            ob.add_limit_order(i, Side::SELL, 98.0, qty_dist(gen));
        }
        latencies.push_back(timer.elapsed_us());
    }
    double elapsed = total_timer.elapsed_ms();
    double throughput = matches / elapsed * 1000.0;
    
    std::cout << "Matching Engine Benchmark:\n";
    std::cout << "  Aggressive orders: " << matches << "\n";
    std::cout << "  Trades executed: " << ob.get_trade_count() << "\n";
    std::cout << "  Time: " << std::fixed << std::setprecision(3) << elapsed << " ms\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(0) 
              << throughput << " orders/sec\n";
    std::cout << "  Latency (mean): " << std::fixed << std::setprecision(3) 
              << (elapsed * 1000.0 / matches) << " µs/order\n";
    std::cout << "  Latency (p50): " << std::fixed << std::setprecision(3) 
              << calculate_percentile(latencies, 50) << " µs\n";
    std::cout << "  Latency (p90): " << std::fixed << std::setprecision(3) 
              << calculate_percentile(latencies, 90) << " µs\n";
    std::cout << "  Latency (p99): " << std::fixed << std::setprecision(3) 
              << calculate_percentile(latencies, 99) << " µs\n\n";
}

void benchmark_market_data_queries() {
    OrderBook ob("BENCHMARK");
    std::mt19937 gen(42);
    std::uniform_real_distribution<> price_dist(99.0, 101.0);
    std::uniform_int_distribution<> qty_dist(1, 100);
    
    // Build orderbook
    for (size_t i = 0; i < 10000; ++i) {
        Side side = (i % 2) == 0 ? Side::BUY : Side::SELL;
        ob.add_limit_order(i, side, price_dist(gen), qty_dist(gen));
    }
    
    std::cout << "Market Data Query Benchmark:\n";
    std::cout << "  Note: Best bid/ask queries are below timer resolution.\n";
    std::cout << "        Operation is O(1) pointer dereference (sub-100ns when hot).\n\n";
    
    // Benchmark depth queries (this is measurable)
    size_t num_depth = 100000;
    volatile uint64_t depth_sink = 0;
    
    // Warmup
    for (size_t i = 0; i < 10000; ++i) {
        auto bids = ob.get_bids(10);
        auto asks = ob.get_asks(10);
        depth_sink += bids.size() + asks.size();
    }
    
    COMPILER_BARRIER();
    Timer timer2;
    for (size_t i = 0; i < num_depth; ++i) {
        auto bids = ob.get_bids(10);
        auto asks = ob.get_asks(10);
        depth_sink += bids.size() + asks.size();
    }
    COMPILER_BARRIER();
    double elapsed2 = timer2.elapsed_us();
    
    std::cout << "  Depth queries (10 levels): " << num_depth << "\n";
    std::cout << "  Time: " << std::fixed << std::setprecision(2)
              << (elapsed2 / 1000.0) << " ms total\n";
    std::cout << "  Latency: " << std::fixed << std::setprecision(4) 
              << (elapsed2 / num_depth) << " µs/query\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(0)
              << (num_depth / (elapsed2 / 1e6)) << " queries/sec\n";
    std::cout << "  (checksum: depth_sink=" << depth_sink << ")\n\n";
}

void print_system_info() {
    std::cout << "System Information:\n";
    
#ifdef __APPLE__
    std::cout << "  OS: macOS\n";
#elif __linux__
    std::cout << "  OS: Linux\n";
#elif _WIN32
    std::cout << "  OS: Windows\n";
#endif

#ifdef __clang__
    std::cout << "  Compiler: Clang " << __clang_major__ << "." << __clang_minor__ << "\n";
#elif __GNUC__
    std::cout << "  Compiler: GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "\n";
#elif _MSC_VER
    std::cout << "  Compiler: MSVC " << _MSC_VER << "\n";
#endif

    std::cout << "  Optimization: "
#ifdef NDEBUG
              << "Release (-O3)\n";
#else
              << "Debug\n";
#endif

    std::cout << "  C++ Standard: " << __cplusplus << "\n\n";
}

int main() {
    std::cout << "=== High-Frequency Trading OrderBook Benchmarks ===\n\n";
    
    print_system_info();
    
    benchmark_order_insertion(100000);
    benchmark_order_cancellation(100000);
    benchmark_matching_engine(10000);
    benchmark_market_data_queries();
    
    std::cout << "=== Benchmarks Complete ===\n";
    
    return 0;
}
