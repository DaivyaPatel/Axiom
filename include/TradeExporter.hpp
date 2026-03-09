#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include "OrderBook.hpp"

namespace axiom {

class TradeExporter {
public:
    static bool export_to_json(const std::vector<Trade>& trades, const std::string& filepath) {
        std::ofstream outfile(filepath);
        if (!outfile.is_open()) {
            return false;
        }

        // Get current system time formatted as ISO 8601
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* tm_info = std::localtime(&now_c);
        
        char time_buffer[80];
        std::strftime(time_buffer, 80, "%Y-%m-%dT%H:%M:%S", tm_info);
        std::string timestamp(time_buffer);

        outfile << "[\n";
        for (size_t i = 0; i < trades.size(); ++i) {
            const Trade& t = trades[i];
            
            // Convert price from integer cents (or basis points) back to double format for JSON export
            double price_decimal = static_cast<double>(t.price) / 100.0;
            
            outfile << "  {\n"
                    << "    \"buy_order_id\": " << t.buy_order_id << ",\n"
                    << "    \"sell_order_id\": " << t.sell_order_id << ",\n"
                    << "    \"price\": " << std::fixed << std::setprecision(2) << price_decimal << ",\n"
                    << "    \"quantity\": " << t.quantity << ",\n"
                    << "    \"timestamp\": \"" << timestamp << "\"\n"
                    << "  }";
            
            if (i < trades.size() - 1) {
                outfile << ",";
            }
            outfile << "\n";
        }
        outfile << "]\n";
        
        outfile.close();
        return true;
    }
};

} // namespace axiom
