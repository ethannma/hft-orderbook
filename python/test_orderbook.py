"""
Unit tests for the HFT OrderBook Python bindings.
"""

import sys
sys.path.insert(0, '.')

import unittest
from pyorderbook import OrderBook, Side

class TestOrderBook(unittest.TestCase):
    
    def setUp(self):
        self.ob = OrderBook("TEST")
    
    def test_initial_state(self):
        """Test orderbook initial state."""
        self.assertEqual(self.ob.get_symbol(), "TEST")
        self.assertEqual(self.ob.get_order_count(), 0)
        self.assertEqual(self.ob.get_trade_count(), 0)
        self.assertIsNone(self.ob.get_best_bid())
        self.assertIsNone(self.ob.get_best_ask())
    
    def test_add_single_order(self):
        """Test adding a single order."""
        self.assertTrue(self.ob.add_limit_order(1, Side.BUY, 100.0, 50))
        self.assertEqual(self.ob.get_order_count(), 1)
        self.assertEqual(self.ob.get_best_bid(), 100.0)
        self.assertIsNone(self.ob.get_best_ask())
    
    def test_price_priority(self):
        """Test price priority."""
        self.ob.add_limit_order(1, Side.BUY, 100.0, 10)
        self.ob.add_limit_order(2, Side.BUY, 101.0, 20)
        self.ob.add_limit_order(3, Side.BUY, 99.0, 30)
        
        self.assertEqual(self.ob.get_best_bid(), 101.0)
        
        self.ob.add_limit_order(4, Side.SELL, 105.0, 10)
        self.ob.add_limit_order(5, Side.SELL, 103.0, 20)
        
        self.assertEqual(self.ob.get_best_ask(), 103.0)
    
    def test_full_match(self):
        """Test full order match."""
        self.ob.add_limit_order(1, Side.BUY, 100.0, 50)
        self.ob.add_limit_order(2, Side.SELL, 100.0, 50)
        
        self.assertEqual(self.ob.get_trade_count(), 1)
        self.assertEqual(self.ob.get_order_count(), 0)
        
        trades = self.ob.get_trades()
        self.assertEqual(len(trades), 1)
        self.assertEqual(trades[0].price, 100.0)
        self.assertEqual(trades[0].quantity, 50)
    
    def test_partial_match(self):
        """Test partial order match."""
        self.ob.add_limit_order(1, Side.BUY, 100.0, 50)
        self.ob.add_limit_order(2, Side.SELL, 100.0, 30)
        
        self.assertEqual(self.ob.get_trade_count(), 1)
        self.assertEqual(self.ob.get_order_count(), 1)
        self.assertEqual(self.ob.get_bid_volume_at_price(100.0), 20)
    
    def test_market_order(self):
        """Test market order execution."""
        self.ob.add_limit_order(1, Side.SELL, 100.0, 50)
        self.ob.add_limit_order(2, Side.SELL, 101.0, 30)
        
        self.ob.add_market_order(3, Side.BUY, 60)
        
        self.assertEqual(self.ob.get_trade_count(), 2)
        self.assertEqual(self.ob.get_best_ask(), 101.0)
        self.assertEqual(self.ob.get_ask_volume_at_price(101.0), 20)
    
    def test_cancel_order(self):
        """Test order cancellation."""
        self.ob.add_limit_order(1, Side.BUY, 100.0, 50)
        self.ob.add_limit_order(2, Side.BUY, 99.0, 30)
        
        self.assertTrue(self.ob.cancel_order(1))
        self.assertEqual(self.ob.get_order_count(), 1)
        self.assertEqual(self.ob.get_best_bid(), 99.0)
        
        self.assertFalse(self.ob.cancel_order(1))  # Already cancelled
        self.assertFalse(self.ob.cancel_order(999))  # Doesn't exist
    
    def test_modify_order(self):
        """Test order modification."""
        self.ob.add_limit_order(1, Side.BUY, 100.0, 50)
        
        self.assertTrue(self.ob.modify_order(1, 75))
        self.assertEqual(self.ob.get_bid_volume_at_price(100.0), 75)
        
        self.assertTrue(self.ob.modify_order(1, 25))
        self.assertEqual(self.ob.get_bid_volume_at_price(100.0), 25)
    
    def test_market_depth(self):
        """Test market depth queries."""
        self.ob.add_limit_order(1, Side.BUY, 100.0, 10)
        self.ob.add_limit_order(2, Side.BUY, 99.0, 20)
        self.ob.add_limit_order(3, Side.BUY, 98.0, 30)
        
        bids = self.ob.get_bids(2)
        self.assertEqual(len(bids), 2)
        self.assertEqual(bids[0][0], 100.0)
        self.assertEqual(bids[0][1], 10)
    
    def test_spread(self):
        """Test bid-ask spread calculation."""
        self.ob.add_limit_order(1, Side.BUY, 100.0, 50)
        self.ob.add_limit_order(2, Side.SELL, 101.0, 30)
        
        self.assertEqual(self.ob.get_mid_price(), 100.5)
        self.assertEqual(self.ob.get_spread(), 1.0)

if __name__ == "__main__":
    unittest.main()
