#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>
#include <algorithm>
#include "../include/OrderBook.hpp"

using namespace axiom;

// Lorenz parameters
const double sigma = 10.0;
const double rho = 28.0;
const double beta = 8.0 / 3.0;

struct State {
    double x, y, z;
};

// Derivatives
State lorenz(const State& s) {
    return {
        sigma * (s.y - s.x),
        s.x * (rho - s.z) - s.y,
        s.x * s.y - beta * s.z
    };
}

// RK4 Step
State rk4_step(const State& s, double dt) {
    State k1 = lorenz(s);
    
    State s2 = {s.x + 0.5 * dt * k1.x, s.y + 0.5 * dt * k1.y, s.z + 0.5 * dt * k1.z};
    State k2 = lorenz(s2);
    
    State s3 = {s.x + 0.5 * dt * k2.x, s.y + 0.5 * dt * k2.y, s.z + 0.5 * dt * k2.z};
    State k3 = lorenz(s3);
    
    State s4 = {s.x + dt * k3.x, s.y + dt * k3.y, s.z + dt * k3.z};
    State k4 = lorenz(s4);
    
    return {
        s.x + (dt / 6.0) * (k1.x + 2.0 * k2.x + 2.0 * k3.x + k4.x),
        s.y + (dt / 6.0) * (k1.y + 2.0 * k2.y + 2.0 * k3.y + k4.y),
        s.z + (dt / 6.0) * (k1.z + 2.0 * k2.z + 2.0 * k3.z + k4.z)
    };
}

int main() {
    std::cout << "Starting AXIOM Chaos Simulator..." << std::endl;
    
    OrderBook book; // Uses default large capacity
    
    State s1 = {1.0, 1.0, 1.0};
    State s2 = {1.0001, 1.0, 1.0}; // For Lyapunov-like divergence calculation
    
    double dt = 0.01;
    const int TARGET_ORDERS = 100000;
    int orders_sent = 0;
    
    double base_price = 100.0;
    uint64_t order_id = 1;
    
    std::vector<double> price_series;
    double divergence_sum = 0.0;
    int steps = 0;
    
    while (orders_sent < TARGET_ORDERS) {
        s1 = rk4_step(s1, dt);
        s2 = rk4_step(s2, dt);
        
        // Calculate divergence
        double dx = s1.x - s2.x;
        double dy = s1.y - s2.y;
        double dz = s1.z - s2.z;
        double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (dist > 0) {
            divergence_sum += std::log(dist / 0.0001);
        }
        steps++;
        
        // Map Phase Space to Market Variables
        // x -> price fluctuation
        double current_price_double = base_price + (s1.x * 0.1);
        uint64_t current_price = static_cast<uint64_t>(std::round(current_price_double));
        
        if (steps % 100 == 0) {
            price_series.push_back(current_price_double);
        }
        
        // y -> order arrival rate (orders per tick)
        int arrival_rate = static_cast<int>(std::abs(s1.y) * 0.5);
        if (arrival_rate < 1) arrival_rate = 1;
        
        // z -> order size / quantity
        uint32_t quantity = static_cast<uint32_t>(std::abs(s1.z));
        if (quantity < 1) quantity = 1;
        if (quantity > 100) quantity = 100;
        
        Side side = (s1.x > 0) ? Side::BUY : Side::SELL;
        
        for (int i = 0; i < arrival_rate && orders_sent < TARGET_ORDERS; ++i) {
            Order order;
            order.order_id = order_id++;
            order.price = current_price;
            order.quantity = quantity;
            order.type = OrderType::LIMIT;
            order.side = side;
            
            while (!book.submit_order(order)) {
                // If queue is temporarily full, process it directly to drain
                book.process_queue();
            }
            orders_sent++;
        }
        
        // Process queue to organically simulate matching engine handling traffic
        if (steps % 10 == 0) {
            book.process_queue();
        }
    }
    
    // Process any remaining orders in the lock-free queue
    book.process_queue();
    
    double lyapunov = divergence_sum / (steps * dt);
    
    std::cout << "\n====== Microstructure Simulator Output ======" << std::endl;
    std::cout << "Total orders sent:    " << book.order_count() << std::endl;
    std::cout << "Total trades matched: " << book.trade_count() << std::endl;
    std::cout << "Lyapunov divergence:  " << lyapunov << std::endl;
    std::cout << "=============================================" << std::endl;
    
    // Simple ASCII chart
    if (!price_series.empty()) {
        std::cout << "\nChaos Price Signal (Sampled):\n";
        double min_p = price_series[0];
        double max_p = price_series[0];
        for (double p : price_series) {
            if (p < min_p) min_p = p;
            if (p > max_p) max_p = p;
        }
        
        // Cap max 50 points plotted to fit into console vertically gracefully
        int skip = std::max(1, static_cast<int>(price_series.size() / 50));
        for (size_t i = 0; i < price_series.size(); i += skip) {
            double p = price_series[i];
            int width = 50; 
            int pos = 0;
            if (max_p - min_p > 1e-9) {
                pos = static_cast<int>((p - min_p) / (max_p - min_p) * width);
            }
            
            std::cout << std::setw(8) << std::fixed << std::setprecision(2) << p << " | ";
            for (int j = 0; j < pos; ++j) std::cout << " ";
            std::cout << "*\n";
        }
    }
    
    return 0;
}