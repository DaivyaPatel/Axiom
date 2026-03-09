#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include "OrderBook.hpp"
#include "TradeExporter.hpp"

using namespace axiom;
using namespace std::chrono;

int main() {
    std::cout << "AXIOM Matching Engine Benchmark\n" << std::endl;
    std::cout << "Generating 1,000,000 random orders..." << std::endl;

    const size_t NUM_ORDERS = 1000000;
    
    // We initialize the book with enough capacity for all generated orders 
    // to sit in the lock-free queue before we start processing.
    OrderBook book(NUM_ORDERS + 10); 

    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Random side (BUY/SELL)
    std::uniform_int_distribution<> side_dist(0, 1);
    
    // Random price between 99 and 101 to force high contention/matches
    std::uniform_int_distribution<uint64_t> price_dist(99, 101);
    
    // Random quantity between 1 and 100
    std::uniform_int_distribution<uint32_t> qty_dist(1, 100);
    
    // Mix of LIMIT (80%) and MARKET (20%)
    std::uniform_int_distribution<> type_dist(1, 100);

    std::vector<Order> orders(NUM_ORDERS);
    for (size_t i = 0; i < NUM_ORDERS; ++i) {
        orders[i].order_id = i + 1;
        orders[i].price = price_dist(gen);
        orders[i].quantity = qty_dist(gen);
        orders[i].side = (side_dist(gen) == 0) ? Side::BUY : Side::SELL;
        orders[i].type = (type_dist(gen) <= 80) ? OrderType::LIMIT : OrderType::MARKET;
    }

    std::cout << "Submitting orders to lock-free queue..." << std::endl;
    for (const auto& order : orders) {
        while (!book.submit_order(order)) {
            // Spin-wait if the queue is temporarily full 
            // (shouldn't happen with our pre-allocated capacity)
        }
    }

    std::cout << "Processing queue and matching orders...\n" << std::endl;
    
    // Start timing the single consumer matching thread
    auto start_time = high_resolution_clock::now();
    
    book.process_queue();
    
    auto end_time = high_resolution_clock::now();
    
    // Calculate metrics
    auto duration_ms = duration_cast<milliseconds>(end_time - start_time).count();
    auto duration_ns = duration_cast<nanoseconds>(end_time - start_time).count();
    
    double ops_per_sec = (duration_ms > 0) ? (NUM_ORDERS * 1000.0) / duration_ms : 0;
    double avg_latency_ns = (NUM_ORDERS > 0) ? static_cast<double>(duration_ns) / NUM_ORDERS : 0;

    // Print benchmark stats
    std::cout << "--- Benchmark Results ---" << std::endl;
    std::cout << "Total orders processed: " << book.order_count() << std::endl;
    std::cout << "Total trades generated: " << book.trade_count() << std::endl;
    std::cout << "Time taken:             " << duration_ms << " ms" << std::endl;
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Orders per second:      " << ops_per_sec << " ops/sec" << std::endl;
    std::cout << "Avg latency per order:  " << avg_latency_ns << " ns" << std::endl;

    std::cout << "\nExporting trades to trades.json..." << std::endl;
    if (axiom::TradeExporter::export_to_json(book.get_trades(), "trades.json")) {
        std::cout << "Export complete!" << std::endl;
    } else {
        std::cerr << "Failed to export trades.json!" << std::endl;
    }

    return 0;
}
