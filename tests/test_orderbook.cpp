#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "orderbook.hpp"

using namespace hft;

class OrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        ob = std::make_unique<OrderBook>("AAPL");
    }
    
    // Invariant checks
    void check_invariants() {
        // Best bid should be less than best ask if both exist
        auto bid = ob->get_best_bid();
        auto ask = ob->get_best_ask();
        if (bid && ask) {
            EXPECT_LT(*bid, *ask) << "Best bid must be less than best ask";
        }
        
        // Order count should never be negative
        EXPECT_GE(ob->get_order_count(), 0);
        
        // Trade count should be monotonically increasing
        static size_t last_trade_count = 0;
        EXPECT_GE(ob->get_trade_count(), last_trade_count);
        last_trade_count = ob->get_trade_count();
        
        // Total volumes should be sum of all levels
        uint64_t bid_sum = 0, ask_sum = 0;
        for (const auto& [price, vol] : ob->get_bids(100)) {
            bid_sum += vol;
            EXPECT_GT(vol, 0) << "Volume at price " << price << " must be positive";
        }
        for (const auto& [price, vol] : ob->get_asks(100)) {
            ask_sum += vol;
            EXPECT_GT(vol, 0) << "Volume at price " << price << " must be positive";
        }
        EXPECT_EQ(bid_sum, ob->get_total_bid_volume());
        EXPECT_EQ(ask_sum, ob->get_total_ask_volume());
    }
    
    std::unique_ptr<OrderBook> ob;
};

TEST_F(OrderBookTest, InitialState) {
    EXPECT_EQ(ob->get_symbol(), "AAPL");
    EXPECT_EQ(ob->get_order_count(), 0);
    EXPECT_EQ(ob->get_trade_count(), 0);
    EXPECT_FALSE(ob->get_best_bid().has_value());
    EXPECT_FALSE(ob->get_best_ask().has_value());
    EXPECT_FALSE(ob->get_mid_price().has_value());
    EXPECT_FALSE(ob->get_spread().has_value());
}

TEST_F(OrderBookTest, AddSingleBid) {
    EXPECT_TRUE(ob->add_limit_order(1, Side::BUY, 100.0, 50));
    EXPECT_EQ(ob->get_order_count(), 1);
    EXPECT_EQ(ob->get_best_bid().value(), 100.0);
    EXPECT_FALSE(ob->get_best_ask().has_value());
    EXPECT_EQ(ob->get_bid_volume_at_price(100.0), 50);
}

TEST_F(OrderBookTest, AddSingleAsk) {
    EXPECT_TRUE(ob->add_limit_order(1, Side::SELL, 101.0, 50));
    EXPECT_EQ(ob->get_order_count(), 1);
    EXPECT_EQ(ob->get_best_ask().value(), 101.0);
    EXPECT_FALSE(ob->get_best_bid().has_value());
    EXPECT_EQ(ob->get_ask_volume_at_price(101.0), 50);
}

TEST_F(OrderBookTest, AddBothSides) {
    EXPECT_TRUE(ob->add_limit_order(1, Side::BUY, 100.0, 50));
    EXPECT_TRUE(ob->add_limit_order(2, Side::SELL, 101.0, 30));
    
    EXPECT_EQ(ob->get_order_count(), 2);
    EXPECT_EQ(ob->get_best_bid().value(), 100.0);
    EXPECT_EQ(ob->get_best_ask().value(), 101.0);
    EXPECT_EQ(ob->get_mid_price().value(), 100.5);
    EXPECT_EQ(ob->get_spread().value(), 1.0);
    
    check_invariants();
}

TEST_F(OrderBookTest, PricePriority) {
    // Add bids at different prices
    ob->add_limit_order(1, Side::BUY, 100.0, 10);
    ob->add_limit_order(2, Side::BUY, 101.0, 20);
    ob->add_limit_order(3, Side::BUY, 99.0, 30);
    
    EXPECT_EQ(ob->get_best_bid().value(), 101.0);
    
    // Add asks at different prices
    ob->add_limit_order(4, Side::SELL, 105.0, 10);
    ob->add_limit_order(5, Side::SELL, 103.0, 20);
    ob->add_limit_order(6, Side::SELL, 104.0, 30);
    
    EXPECT_EQ(ob->get_best_ask().value(), 103.0);
}

TEST_F(OrderBookTest, TimePriority) {
    // Add three orders at same price
    ob->add_limit_order(1, Side::BUY, 100.0, 10);
    ob->add_limit_order(2, Side::BUY, 100.0, 20);
    ob->add_limit_order(3, Side::BUY, 100.0, 30);
    
    EXPECT_EQ(ob->get_bid_volume_at_price(100.0), 60);
    
    // Market sell should match with orders in FIFO order
    ob->add_market_order(4, Side::SELL, 25);
    
    EXPECT_EQ(ob->get_trade_count(), 2); // Should hit first two orders
    EXPECT_EQ(ob->get_bid_volume_at_price(100.0), 35); // 60 - 25 = 35
}

TEST_F(OrderBookTest, FullMatch) {
    ob->add_limit_order(1, Side::BUY, 100.0, 50);
    ob->add_limit_order(2, Side::SELL, 100.0, 50);
    
    EXPECT_EQ(ob->get_trade_count(), 1);
    EXPECT_EQ(ob->get_order_count(), 0); // Both orders fully filled
    EXPECT_FALSE(ob->get_best_bid().has_value());
    EXPECT_FALSE(ob->get_best_ask().has_value());
    
    auto trades = ob->get_trades();
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 100.0);
    EXPECT_EQ(trades[0].quantity, 50);
    EXPECT_GT(trades[0].quantity, 0) << "Trade quantity must be positive";
    
    check_invariants();
}

TEST_F(OrderBookTest, PartialMatch) {
    ob->add_limit_order(1, Side::BUY, 100.0, 50);
    ob->add_limit_order(2, Side::SELL, 100.0, 30);
    
    EXPECT_EQ(ob->get_trade_count(), 1);
    EXPECT_EQ(ob->get_order_count(), 1); // Buy order partially filled
    EXPECT_EQ(ob->get_best_bid().value(), 100.0);
    EXPECT_EQ(ob->get_bid_volume_at_price(100.0), 20);
    
    auto trades = ob->get_trades();
    EXPECT_EQ(trades[0].quantity, 30);
}

TEST_F(OrderBookTest, AggressiveOrder) {
    // Place passive orders
    ob->add_limit_order(1, Side::SELL, 100.0, 10);
    ob->add_limit_order(2, Side::SELL, 101.0, 20);
    ob->add_limit_order(3, Side::SELL, 102.0, 30);
    
    // Aggressive buy order crosses multiple levels
    ob->add_limit_order(4, Side::BUY, 101.5, 35);
    
    EXPECT_EQ(ob->get_trade_count(), 2); // Matches with 100.0 and 101.0
    EXPECT_EQ(ob->get_best_ask().value(), 102.0); // Only 102.0 remains
    EXPECT_EQ(ob->get_order_count(), 2); // One ask + partial buy
}

TEST_F(OrderBookTest, MarketOrder) {
    ob->add_limit_order(1, Side::SELL, 100.0, 50);
    ob->add_limit_order(2, Side::SELL, 101.0, 30);
    
    ob->add_market_order(3, Side::BUY, 60);
    
    EXPECT_EQ(ob->get_trade_count(), 2);
    EXPECT_EQ(ob->get_best_ask().value(), 101.0);
    EXPECT_EQ(ob->get_ask_volume_at_price(101.0), 20);
}

TEST_F(OrderBookTest, CancelOrder) {
    ob->add_limit_order(1, Side::BUY, 100.0, 50);
    ob->add_limit_order(2, Side::BUY, 99.0, 30);
    
    EXPECT_EQ(ob->get_order_count(), 2);
    EXPECT_TRUE(ob->cancel_order(1));
    EXPECT_EQ(ob->get_order_count(), 1);
    EXPECT_EQ(ob->get_best_bid().value(), 99.0);
    
    EXPECT_FALSE(ob->cancel_order(1)); // Already cancelled
    EXPECT_FALSE(ob->cancel_order(999)); // Doesn't exist
}

TEST_F(OrderBookTest, ModifyOrder) {
    ob->add_limit_order(1, Side::BUY, 100.0, 50);
    
    EXPECT_TRUE(ob->modify_order(1, 75));
    EXPECT_EQ(ob->get_bid_volume_at_price(100.0), 75);
    
    EXPECT_TRUE(ob->modify_order(1, 25));
    EXPECT_EQ(ob->get_bid_volume_at_price(100.0), 25);
    
    // Modify to 0 should cancel
    EXPECT_TRUE(ob->modify_order(1, 0));
    EXPECT_EQ(ob->get_order_count(), 0);
}

TEST_F(OrderBookTest, GetDepth) {
    // Add multiple price levels
    ob->add_limit_order(1, Side::BUY, 100.0, 10);
    ob->add_limit_order(2, Side::BUY, 99.0, 20);
    ob->add_limit_order(3, Side::BUY, 98.0, 30);
    
    ob->add_limit_order(4, Side::SELL, 101.0, 15);
    ob->add_limit_order(5, Side::SELL, 102.0, 25);
    ob->add_limit_order(6, Side::SELL, 103.0, 35);
    
    auto bids = ob->get_bids(2);
    EXPECT_EQ(bids.size(), 2);
    EXPECT_EQ(bids[0].first, 100.0);
    EXPECT_EQ(bids[0].second, 10);
    EXPECT_EQ(bids[1].first, 99.0);
    EXPECT_EQ(bids[1].second, 20);
    
    auto asks = ob->get_asks(2);
    EXPECT_EQ(asks.size(), 2);
    EXPECT_EQ(asks[0].first, 101.0);
    EXPECT_EQ(asks[0].second, 15);
    EXPECT_EQ(asks[1].first, 102.0);
    EXPECT_EQ(asks[1].second, 25);
}

TEST_F(OrderBookTest, TotalVolume) {
    ob->add_limit_order(1, Side::BUY, 100.0, 10);
    ob->add_limit_order(2, Side::BUY, 99.0, 20);
    ob->add_limit_order(3, Side::BUY, 98.0, 30);
    
    ob->add_limit_order(4, Side::SELL, 101.0, 15);
    ob->add_limit_order(5, Side::SELL, 102.0, 25);
    
    EXPECT_EQ(ob->get_total_bid_volume(), 60);
    EXPECT_EQ(ob->get_total_ask_volume(), 40);
}

TEST_F(OrderBookTest, InvalidOrders) {
    EXPECT_FALSE(ob->add_limit_order(1, Side::BUY, 0.0, 50)); // Invalid price
    EXPECT_FALSE(ob->add_limit_order(1, Side::BUY, -100.0, 50)); // Negative price
    EXPECT_FALSE(ob->add_limit_order(1, Side::BUY, 100.0, 0)); // Zero quantity
    
    ob->add_limit_order(1, Side::BUY, 100.0, 50);
    EXPECT_FALSE(ob->add_limit_order(1, Side::SELL, 101.0, 30)); // Duplicate ID
}

TEST_F(OrderBookTest, TradeExecutionPrice) {
    // Passive order at 100.0
    ob->add_limit_order(1, Side::SELL, 100.0, 50);
    
    // Aggressive order at 101.0 should execute at passive price (100.0)
    ob->add_limit_order(2, Side::BUY, 101.0, 50);
    
    auto trades = ob->get_trades();
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 100.0); // Price improvement for aggressive order
}

TEST_F(OrderBookTest, ModifyOrderLosesTimePriority) {
    // Add three orders at same price
    ob->add_limit_order(1, Side::BUY, 100.0, 50);
    ob->add_limit_order(2, Side::BUY, 100.0, 50);
    ob->add_limit_order(3, Side::BUY, 100.0, 50);
    
    // Increase quantity on order 1 (should lose time priority)
    EXPECT_TRUE(ob->modify_order(1, 100));
    
    // Add aggressive sell order that fills 50 shares
    ob->add_limit_order(4, Side::SELL, 100.0, 50);
    
    // Order 2 should have filled (was first after order 1 lost priority)
    auto trades = ob->get_trades();
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].buy_order_id, 2); // Order 2 should fill, not order 1
    
    // Order 1 and 3 should still be in the book
    EXPECT_EQ(ob->get_bid_volume_at_price(100.0), 150); // 100 + 50
}

TEST_F(OrderBookTest, ModifyOrderDecreaseKeepsTimePriority) {
    // Add three orders at same price
    ob->add_limit_order(1, Side::BUY, 100.0, 100);
    ob->add_limit_order(2, Side::BUY, 100.0, 50);
    ob->add_limit_order(3, Side::BUY, 100.0, 50);
    
    // Decrease quantity on order 1 (should keep time priority)
    EXPECT_TRUE(ob->modify_order(1, 50));
    
    // Add aggressive sell order that fills 50 shares
    ob->add_limit_order(4, Side::SELL, 100.0, 50);
    
    // Order 1 should have filled (kept time priority)
    auto trades = ob->get_trades();
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].buy_order_id, 1); // Order 1 should fill first
    
    // Orders 2 and 3 should still be in the book
    EXPECT_EQ(ob->get_bid_volume_at_price(100.0), 100); // 50 + 50
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
