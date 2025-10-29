#include "orderbook.hpp"
#include <algorithm>
#include <stdexcept>

namespace hft {

OrderBook::OrderBook(const std::string& symbol)
    : symbol_(symbol), timestamp_counter_(0) {}

bool OrderBook::add_limit_order(uint64_t order_id, Side side, double price, uint64_t quantity) {
    if (orders_.find(order_id) != orders_.end()) {
        return false; // Order ID already exists
    }
    
    if (quantity == 0 || price <= 0.0) {
        return false; // Invalid order parameters
    }
    
    auto order = std::make_shared<Order>(order_id, side, price, quantity, timestamp_counter_++);
    orders_[order_id] = order;
    
    // Try to match the order first
    match_order(order);
    
    // If there's remaining quantity, add to book
    if (order->quantity > 0) {
        if (side == Side::BUY) {
            auto& level = bids_.try_emplace(price, price).first->second;
            level.add_order(order);
        } else {
            auto& level = asks_.try_emplace(price, price).first->second;
            level.add_order(order);
        }
    } else {
        // Order fully filled, remove from tracking
        orders_.erase(order_id);
    }
    
    return true;
}

bool OrderBook::add_market_order(uint64_t order_id, Side side, uint64_t quantity) {
    if (orders_.find(order_id) != orders_.end()) {
        return false;
    }
    
    if (quantity == 0) {
        return false;
    }
    
    // Market orders use sentinel value to distinguish from limit orders
    auto order = std::make_shared<Order>(order_id, side, MARKET_ORDER_PRICE, quantity, 
                                        timestamp_counter_++, OrderType::MARKET);
    orders_[order_id] = order;
    
    match_order(order);
    
    // Market orders should not rest in the book
    orders_.erase(order_id);
    
    return true;
}

bool OrderBook::cancel_order(uint64_t order_id) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    auto order = it->second;
    
    // Remove from appropriate side
    if (order->side == Side::BUY) {
        auto level_it = bids_.find(order->price);
        if (level_it != bids_.end()) {
            level_it->second.remove_order(order_id);
            if (level_it->second.is_empty()) {
                bids_.erase(level_it);
            }
        }
    } else {
        auto level_it = asks_.find(order->price);
        if (level_it != asks_.end()) {
            level_it->second.remove_order(order_id);
            if (level_it->second.is_empty()) {
                asks_.erase(level_it);
            }
        }
    }
    
    orders_.erase(it);
    return true;
}

bool OrderBook::modify_order(uint64_t order_id, uint64_t new_quantity) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    if (new_quantity == 0) {
        return cancel_order(order_id);
    }
    
    auto order = it->second;
    uint64_t old_quantity = order->quantity;
    
    // If quantity increases, lose time priority (move to back of queue)
    if (new_quantity > old_quantity) {
        // Cancel and re-add to lose time priority
        Side side = order->side;
        double price = order->price;
        
        cancel_order(order_id);
        add_limit_order(order_id, side, price, new_quantity);
        return true;
    }
    
    // Quantity decrease: maintain time priority, just update quantity
    if (order->side == Side::BUY) {
        auto level_it = bids_.find(order->price);
        if (level_it != bids_.end()) {
            level_it->second.total_volume += (new_quantity - old_quantity);
        }
    } else {
        auto level_it = asks_.find(order->price);
        if (level_it != asks_.end()) {
            level_it->second.total_volume += (new_quantity - old_quantity);
        }
    }
    
    order->quantity = new_quantity;
    return true;
}

void OrderBook::match_order(std::shared_ptr<Order> order) {
    if (order->side == Side::BUY) {
        // Match buy order with asks
        while (order->quantity > 0 && !asks_.empty()) {
            auto& best_ask_level = asks_.begin()->second;
            
            // For market orders, match at any price; for limit orders, check price
            if (order->type == OrderType::LIMIT && order->price < best_ask_level.price) {
                break; // No more matches possible
            }
            
            auto& passive_order = best_ask_level.orders.front();
            uint64_t trade_quantity = std::min(order->quantity, passive_order->quantity);
            
            // Buy order is aggressive, sell order (passive_order) is passive
            execute_trade(order, passive_order, trade_quantity);
            
            order->quantity -= trade_quantity;
            passive_order->quantity -= trade_quantity;
            
            if (passive_order->quantity == 0) {
                orders_.erase(passive_order->order_id);
                best_ask_level.orders.pop_front();
                best_ask_level.total_volume -= trade_quantity;
                
                if (best_ask_level.is_empty()) {
                    asks_.erase(asks_.begin());
                }
            } else {
                best_ask_level.total_volume -= trade_quantity;
            }
        }
    } else {
        // Match sell order with bids
        while (order->quantity > 0 && !bids_.empty()) {
            auto& best_bid_level = bids_.begin()->second;
            
            // For market orders, match at any price; for limit orders, check price
            if (order->type == OrderType::LIMIT && order->price > best_bid_level.price) {
                break; // No more matches possible
            }
            
            auto& passive_order = best_bid_level.orders.front();
            uint64_t trade_quantity = std::min(order->quantity, passive_order->quantity);
            
            // Sell order is aggressive, buy order (passive_order) is passive
            execute_trade(passive_order, order, trade_quantity);
            
            order->quantity -= trade_quantity;
            passive_order->quantity -= trade_quantity;
            
            if (passive_order->quantity == 0) {
                orders_.erase(passive_order->order_id);
                best_bid_level.orders.pop_front();
                best_bid_level.total_volume -= trade_quantity;
                
                if (best_bid_level.is_empty()) {
                    bids_.erase(bids_.begin());
                }
            } else {
                best_bid_level.total_volume -= trade_quantity;
            }
        }
    }
}

void OrderBook::execute_trade(std::shared_ptr<Order> buy_order,
                              std::shared_ptr<Order> sell_order,
                              uint64_t quantity) {
    // Determine which order was on the book first (passive order)
    // Trade executes at passive order's price
    auto passive_order = (buy_order->timestamp < sell_order->timestamp) ? buy_order : sell_order;
    
    Trade trade{
        buy_order->order_id,
        sell_order->order_id,
        passive_order->price, // Trade executes at passive order's price
        quantity,
        timestamp_counter_++
    };
    trades_.push_back(trade);
}

std::optional<double> OrderBook::get_best_bid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<double> OrderBook::get_best_ask() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

std::optional<double> OrderBook::get_mid_price() const {
    auto bid = get_best_bid();
    auto ask = get_best_ask();
    
    if (!bid || !ask) {
        return std::nullopt;
    }
    
    return (*bid + *ask) / 2.0;
}

std::optional<double> OrderBook::get_spread() const {
    auto bid = get_best_bid();
    auto ask = get_best_ask();
    
    if (!bid || !ask) {
        return std::nullopt;
    }
    
    return *ask - *bid;
}

uint64_t OrderBook::get_bid_volume_at_price(double price) const {
    auto it = bids_.find(price);
    return it != bids_.end() ? it->second.total_volume : 0;
}

uint64_t OrderBook::get_ask_volume_at_price(double price) const {
    auto it = asks_.find(price);
    return it != asks_.end() ? it->second.total_volume : 0;
}

uint64_t OrderBook::get_total_bid_volume() const {
    uint64_t total = 0;
    for (const auto& [price, level] : bids_) {
        total += level.total_volume;
    }
    return total;
}

uint64_t OrderBook::get_total_ask_volume() const {
    uint64_t total = 0;
    for (const auto& [price, level] : asks_) {
        total += level.total_volume;
    }
    return total;
}

std::vector<std::pair<double, uint64_t>> OrderBook::get_bids(size_t depth) const {
    std::vector<std::pair<double, uint64_t>> result;
    result.reserve(std::min(depth, bids_.size()));
    
    size_t count = 0;
    for (const auto& [price, level] : bids_) {
        if (count >= depth) break;
        result.emplace_back(price, level.total_volume);
        ++count;
    }
    
    return result;
}

std::vector<std::pair<double, uint64_t>> OrderBook::get_asks(size_t depth) const {
    std::vector<std::pair<double, uint64_t>> result;
    result.reserve(std::min(depth, asks_.size()));
    
    size_t count = 0;
    for (const auto& [price, level] : asks_) {
        if (count >= depth) break;
        result.emplace_back(price, level.total_volume);
        ++count;
    }
    
    return result;
}

} // namespace hft
