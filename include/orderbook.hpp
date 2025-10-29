#pragma once

#include <cstdint>
#include <map>
#include <deque>
#include <unordered_map>
#include <memory>
#include <string>
#include <optional>
#include <limits>

namespace hft {

enum class Side : uint8_t {
    BUY,
    SELL
};

enum class OrderType : uint8_t {
    LIMIT,
    MARKET
};

// Sentinel value for market orders (use max double instead of 0.0)
constexpr double MARKET_ORDER_PRICE = std::numeric_limits<double>::max();

struct Order {
    uint64_t order_id;
    Side side;
    double price;
    uint64_t quantity;
    uint64_t timestamp;
    OrderType type;
    
    Order(uint64_t id, Side s, double p, uint64_t qty, uint64_t ts, OrderType t = OrderType::LIMIT)
        : order_id(id), side(s), price(p), quantity(qty), timestamp(ts), type(t) {}
};

struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double price;
    uint64_t quantity;
    uint64_t timestamp;
};

// Price level containing all orders at a specific price
struct PriceLevel {
    double price;
    uint64_t total_volume;
    std::deque<std::shared_ptr<Order>> orders;
    
    explicit PriceLevel(double p) : price(p), total_volume(0) {}
    
    void add_order(std::shared_ptr<Order> order) {
        orders.push_back(order);
        total_volume += order->quantity;
    }
    
    void remove_order(uint64_t order_id) {
        auto it = std::find_if(orders.begin(), orders.end(),
            [order_id](const auto& order) { return order->order_id == order_id; });
        if (it != orders.end()) {
            total_volume -= (*it)->quantity;
            orders.erase(it);
        }
    }
    
    bool is_empty() const {
        return orders.empty();
    }
};

class OrderBook {
public:
    OrderBook(const std::string& symbol);
    
    // Order management
    bool add_limit_order(uint64_t order_id, Side side, double price, uint64_t quantity);
    bool add_market_order(uint64_t order_id, Side side, uint64_t quantity);
    bool cancel_order(uint64_t order_id);
    bool modify_order(uint64_t order_id, uint64_t new_quantity);
    
    // Query methods
    __attribute__((noinline)) std::optional<double> get_best_bid() const;
    __attribute__((noinline)) std::optional<double> get_best_ask() const;
    std::optional<double> get_mid_price() const;
    std::optional<double> get_spread() const;
    uint64_t get_bid_volume_at_price(double price) const;
    uint64_t get_ask_volume_at_price(double price) const;
    uint64_t get_total_bid_volume() const;
    uint64_t get_total_ask_volume() const;
    
    // Market depth (top N levels)
    std::vector<std::pair<double, uint64_t>> get_bids(size_t depth = 10) const;
    std::vector<std::pair<double, uint64_t>> get_asks(size_t depth = 10) const;
    
    // Trade history
    const std::vector<Trade>& get_trades() const { return trades_; }
    
    // Statistics
    size_t get_order_count() const { return orders_.size(); }
    size_t get_trade_count() const { return trades_.size(); }
    
    const std::string& get_symbol() const { return symbol_; }
    
private:
    void match_order(std::shared_ptr<Order> order);
    void execute_trade(std::shared_ptr<Order> buy_order, 
                      std::shared_ptr<Order> sell_order,
                      uint64_t quantity);
    
    std::string symbol_;
    uint64_t timestamp_counter_;
    
    // Price levels: price -> PriceLevel
    // Bids: highest to lowest (using std::greater)
    std::map<double, PriceLevel, std::greater<double>> bids_;
    // Asks: lowest to highest (using std::less - default)
    std::map<double, PriceLevel> asks_;
    
    // Order tracking
    std::unordered_map<uint64_t, std::shared_ptr<Order>> orders_;
    
    // Trade history
    std::vector<Trade> trades_;
};

} // namespace hft
