"""
Performance benchmark for the HFT OrderBook.
Measures throughput and latency for various operations.
"""

import sys
sys.path.insert(0, '.')

import time
import random
from pyorderbook import OrderBook, Side

def benchmark_order_insertion(num_orders=100000):
    """Benchmark order insertion performance."""
    ob = OrderBook("BENCH")
    random.seed(42)
    
    start = time.perf_counter()
    for i in range(num_orders):
        side = Side.BUY if random.random() < 0.5 else Side.SELL
        price = random.uniform(99.0, 101.0)
        quantity = random.randint(1, 100)
        ob.add_limit_order(i, side, price, quantity)
    elapsed = time.perf_counter() - start
    
    print("Order Insertion Benchmark:")
    print(f"  Orders: {num_orders:,}")
    print(f"  Time: {elapsed:.3f} sec")
    print(f"  Throughput: {num_orders/elapsed:,.0f} orders/sec")
    print(f"  Latency: {elapsed/num_orders*1e6:.2f} µs/order")
    print()

def benchmark_order_cancellation(num_orders=100000):
    """Benchmark order cancellation performance."""
    ob = OrderBook("BENCH")
    random.seed(42)
    
    # Insert orders
    for i in range(num_orders):
        ob.add_limit_order(i, Side.BUY, random.uniform(99.0, 101.0), random.randint(1, 100))
    
    # Benchmark cancellation
    start = time.perf_counter()
    for i in range(num_orders):
        ob.cancel_order(i)
    elapsed = time.perf_counter() - start
    
    print("Order Cancellation Benchmark:")
    print(f"  Orders: {num_orders:,}")
    print(f"  Time: {elapsed:.3f} sec")
    print(f"  Throughput: {num_orders/elapsed:,.0f} cancels/sec")
    print(f"  Latency: {elapsed/num_orders*1e6:.2f} µs/cancel")
    print()

def benchmark_matching_engine(num_orders=10000):
    """Benchmark matching engine performance."""
    ob = OrderBook("BENCH")
    random.seed(42)
    
    # Place resting orders
    for i in range(num_orders):
        side = Side.BUY if i % 2 == 0 else Side.SELL
        base_price = 99.0 if side == Side.BUY else 101.0
        price = base_price + (i % 100) * 0.01
        ob.add_limit_order(i, side, price, random.randint(1, 100))
    
    # Benchmark aggressive orders
    num_aggressive = 1000
    start = time.perf_counter()
    for i in range(num_orders, num_orders + num_aggressive):
        if i % 2 == 0:
            ob.add_limit_order(i, Side.BUY, 102.0, random.randint(1, 100))
        else:
            ob.add_limit_order(i, Side.SELL, 98.0, random.randint(1, 100))
    elapsed = time.perf_counter() - start
    
    print("Matching Engine Benchmark:")
    print(f"  Aggressive orders: {num_aggressive:,}")
    print(f"  Trades executed: {ob.get_trade_count():,}")
    print(f"  Time: {elapsed:.3f} sec")
    print(f"  Throughput: {num_aggressive/elapsed:,.0f} orders/sec")
    print(f"  Latency: {elapsed/num_aggressive*1e6:.2f} µs/order")
    print()

def benchmark_market_data(num_queries=100000):
    """Benchmark market data query performance."""
    ob = OrderBook("BENCH")
    random.seed(42)
    
    # Build orderbook
    for i in range(10000):
        side = Side.BUY if i % 2 == 0 else Side.SELL
        ob.add_limit_order(i, side, random.uniform(99.0, 101.0), random.randint(1, 100))
    
    # Benchmark best bid/ask
    start = time.perf_counter()
    for _ in range(num_queries):
        bid = ob.get_best_bid()
        ask = ob.get_best_ask()
    elapsed = time.perf_counter() - start
    
    print("Market Data Query Benchmark (Best Bid/Ask):")
    print(f"  Queries: {num_queries:,}")
    print(f"  Time: {elapsed:.3f} sec")
    print(f"  Throughput: {num_queries/elapsed:,.0f} queries/sec")
    print(f"  Latency: {elapsed/num_queries*1e6:.3f} µs/query")
    print()
    
    # Benchmark depth queries
    num_depth = num_queries // 10
    start = time.perf_counter()
    for _ in range(num_depth):
        bids = ob.get_bids(10)
        asks = ob.get_asks(10)
    elapsed = time.perf_counter() - start
    
    print("Market Data Query Benchmark (10 Levels):")
    print(f"  Queries: {num_depth:,}")
    print(f"  Time: {elapsed:.3f} sec")
    print(f"  Throughput: {num_depth/elapsed:,.0f} queries/sec")
    print(f"  Latency: {elapsed/num_depth*1e6:.3f} µs/query")
    print()

def main():
    print("=== HFT OrderBook Performance Benchmarks ===\n")
    
    benchmark_order_insertion(100000)
    benchmark_order_cancellation(100000)
    benchmark_matching_engine(10000)
    benchmark_market_data(100000)
    
    print("=== Benchmarks Complete ===")

if __name__ == "__main__":
    main()
