"""
Example usage of the HFT OrderBook Python bindings.
Demonstrates basic order management, matching, and market data queries.
"""

import sys
sys.path.insert(0, '.')

from pyorderbook import OrderBook, Side

def main():
    # Create orderbook
    ob = OrderBook("AAPL")
    print(f"Created orderbook: {ob}\n")
    
    # Add limit orders
    print("=== Adding Limit Orders ===")
    ob.add_limit_order(1, Side.BUY, 150.00, 100)
    ob.add_limit_order(2, Side.BUY, 149.95, 200)
    ob.add_limit_order(3, Side.BUY, 149.90, 150)
    
    ob.add_limit_order(4, Side.SELL, 150.10, 100)
    ob.add_limit_order(5, Side.SELL, 150.15, 200)
    ob.add_limit_order(6, Side.SELL, 150.20, 150)
    
    print(f"Orders in book: {ob.get_order_count()}")
    print(f"Best bid: ${ob.get_best_bid():.2f}")
    print(f"Best ask: ${ob.get_best_ask():.2f}")
    print(f"Mid price: ${ob.get_mid_price():.4f}")
    print(f"Spread: ${ob.get_spread():.4f}\n")
    
    # Display market depth
    print("=== Market Depth ===")
    print("Bids:")
    for price, volume in ob.get_bids(5):
        print(f"  ${price:.2f} - {volume} shares")
    
    print("Asks:")
    for price, volume in ob.get_asks(5):
        print(f"  ${price:.2f} - {volume} shares")
    print()
    
    # Execute a trade
    print("=== Executing Trade ===")
    print("Adding aggressive buy order at $150.15 for 150 shares")
    ob.add_limit_order(7, Side.BUY, 150.15, 150)
    
    print(f"\nTrades executed: {ob.get_trade_count()}")
    for trade in ob.get_trades():
        print(f"  Trade: Buy #{trade.buy_order_id} x Sell #{trade.sell_order_id}")
        print(f"    Price: ${trade.price:.2f}, Quantity: {trade.quantity}")
    
    print(f"\nNew best ask: ${ob.get_best_ask():.2f}")
    print(f"New mid price: ${ob.get_mid_price():.4f}\n")
    
    # Market order
    print("=== Market Order ===")
    print("Adding market sell order for 250 shares")
    ob.add_market_order(8, Side.SELL, 250)
    
    print(f"Total trades: {ob.get_trade_count()}")
    print(f"New best bid: ${ob.get_best_bid():.2f}")
    print(f"Remaining bid volume at $149.95: {ob.get_bid_volume_at_price(149.95)}\n")
    
    # Cancel order
    print("=== Order Management ===")
    print(f"Orders before cancel: {ob.get_order_count()}")
    ob.cancel_order(6)
    print(f"Orders after canceling order #6: {ob.get_order_count()}")
    print(f"New best ask: ${ob.get_best_ask():.2f}\n")
    
    # Volume statistics
    print("=== Volume Statistics ===")
    print(f"Total bid volume: {ob.get_total_bid_volume()} shares")
    print(f"Total ask volume: {ob.get_total_ask_volume()} shares")
    
    print("\n" + ob.__repr__())

if __name__ == "__main__":
    main()
