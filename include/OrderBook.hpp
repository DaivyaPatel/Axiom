#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <list>
#include <optional>
#include <algorithm>
#include <atomic>
#include <array>

namespace axiom {

enum class Side {
    BUY,
    SELL
};

enum class OrderType {
    LIMIT,
    MARKET
};

struct Order {
    uint64_t order_id;
    uint64_t price;
    uint32_t quantity;
    Side side;
    OrderType type;
    
    // Default constructor is required for std::array
    Order() : order_id(0), price(0), quantity(0), side(Side::BUY), type(OrderType::LIMIT) {}
};

struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    uint64_t price;
    uint32_t quantity;
};

// Lock-free Single-Producer Single-Consumer Ring Buffer
template <typename T, size_t Capacity>
class SPSCRingBuffer {
public:
    SPSCRingBuffer() : buffer_(new std::array<T, Capacity>()) {}
    
    ~SPSCRingBuffer() { 
        delete buffer_; 
    }

    // Disable copy/move
    SPSCRingBuffer(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer& operator=(const SPSCRingBuffer&) = delete;

    bool push(const T& item) {
        size_t write_idx = head_.load(std::memory_order_relaxed);
        size_t next_write_idx = write_idx + 1;
        if (next_write_idx == Capacity) {
            next_write_idx = 0;
        }

        // If next write index matches read index (tail), full
        if (next_write_idx == tail_.load(std::memory_order_acquire)) {
            return false; 
        }

        (*buffer_)[write_idx] = item;
        head_.store(next_write_idx, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        size_t read_idx = tail_.load(std::memory_order_relaxed);
        
        // If read index matches write index (head), empty
        if (read_idx == head_.load(std::memory_order_acquire)) {
            return false; 
        }

        item = (*buffer_)[read_idx];

        size_t next_read_idx = read_idx + 1;
        if (next_read_idx == Capacity) {
            next_read_idx = 0;
        }

        tail_.store(next_read_idx, std::memory_order_release);
        return true;
    }

private:
    // head_ modified by producer (writer), aligned for cache line
    alignas(64) std::atomic<size_t> head_{0}; 

    // tail_ modified by consumer (reader), aligned for cache line
    alignas(64) std::atomic<size_t> tail_{0};

    // Heap-allocated to prevent enormous stack overflows (Capacity * sizeof(T) could be highly problematic)
    std::array<T, Capacity>* buffer_; 
};

// Lock-free order book core processing engine
class OrderBook {
public:
    // With std::array, capacity must be known at compile time.
    // We statically configure 2M slots, more than enough to handle 1M test orders
    OrderBook(size_t /*capacity*/ = 1000000) 
        : _order_count(0) {}

    // Lock-free submit (called by networking/client thread)
    bool submit_order(const Order& order) {
        return incoming_orders.push(order);
    }

    // Process orders (called by a single matching thread)
    void process_queue() {
        Order order;
        while (incoming_orders.pop(order)) {
            _order_count++;
            match(order);
        }
    }

    std::optional<uint64_t> get_best_bid() const {
        if (bids.empty()) return std::nullopt;
        return bids.begin()->first;
    }

    std::optional<uint64_t> get_best_ask() const {
        if (asks.empty()) return std::nullopt;
        return asks.begin()->first;
    }

    size_t order_count() const {
        return _order_count;
    }

    size_t trade_count() const {
        return trades.size();
    }

    const std::vector<Trade>& get_trades() const {
        return trades;
    }

private:
    // Single-producer single-consumer queue for extremely low latency
    SPSCRingBuffer<Order, 2097152> incoming_orders; // 2 million capacity

    // Bids: highest price first
    std::map<uint64_t, std::list<Order>, std::greater<uint64_t>> bids;
    // Asks: lowest price first
    std::map<uint64_t, std::list<Order>, std::less<uint64_t>> asks;

    std::vector<Trade> trades;
    size_t _order_count;

    void match(Order order) {
        if (order.side == Side::BUY) {
            match_buy(order);
        } else {
            match_sell(order);
        }
    }

    void match_buy(Order& order) {
        while (order.quantity > 0 && !asks.empty()) {
            auto best_ask_it = asks.begin();
            uint64_t ask_price = best_ask_it->first;

            if (order.type == OrderType::LIMIT && order.price < ask_price) {
                break; // Limit price not met
            }

            auto& ask_list = best_ask_it->second;
            auto it = ask_list.begin();

            while (it != ask_list.end() && order.quantity > 0) {
                Order& ask_order = *it;
                uint32_t trade_qty = std::min(order.quantity, ask_order.quantity);

                trades.push_back({
                    order.order_id,
                    ask_order.order_id,
                    ask_price,
                    trade_qty
                });

                order.quantity -= trade_qty;
                ask_order.quantity -= trade_qty;

                if (ask_order.quantity == 0) {
                    it = ask_list.erase(it);
                } else {
                    ++it;
                }
            }

            if (ask_list.empty()) {
                asks.erase(best_ask_it);
            }
        }

        // Add remaining to book if it's a limit order
        if (order.quantity > 0 && order.type == OrderType::LIMIT) {
            bids[order.price].push_back(order);
        }
    }

    void match_sell(Order& order) {
        while (order.quantity > 0 && !bids.empty()) {
            auto best_bid_it = bids.begin();
            uint64_t bid_price = best_bid_it->first;

            if (order.type == OrderType::LIMIT && order.price > bid_price) {
                break; // Limit price not met
            }

            auto& bid_list = best_bid_it->second;
            auto it = bid_list.begin();

            while (it != bid_list.end() && order.quantity > 0) {
                Order& bid_order = *it;
                uint32_t trade_qty = std::min(order.quantity, bid_order.quantity);

                trades.push_back({
                    bid_order.order_id,
                    order.order_id,
                    bid_price,
                    trade_qty
                });

                order.quantity -= trade_qty;
                bid_order.quantity -= trade_qty;

                if (bid_order.quantity == 0) {
                    it = bid_list.erase(it);
                } else {
                    ++it;
                }
            }

            if (bid_list.empty()) {
                bids.erase(best_bid_it);
            }
        }

        // Add remaining to book if it's a limit order
        if (order.quantity > 0 && order.type == OrderType::LIMIT) {
            asks[order.price].push_back(order);
        }
    }
};

} // namespace axiom
