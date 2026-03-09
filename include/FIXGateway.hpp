#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <random>
#include <atomic>
#include "OrderBook.hpp"

namespace axiom {

struct FIXMessage {
    char msg_type;          // 'D' = New Order Single, '8' = Execution Report
    std::string sender_comp_id;
    std::string target_comp_id;
    std::string cl_ord_id;  // Tag 11
    std::string symbol;     // Tag 55
    char side;              // '1' = Buy, '2' = Sell (Tag 54)
    uint32_t order_qty;     // Tag 38
    uint64_t price;         // In cents (Tag 44)
    char ord_type;          // '1' = Market, '2' = Limit (Tag 40)
    
    // For outgoing Execution Reports
    std::string order_id;   // Tag 37
    std::string exec_id;    // Tag 17
    char exec_type;         // '0' = New, '1' = Partial fill, '2' = Fill, '8' = Rejected (Tag 150)
    char ord_status;        // '0' = New, '1' = Partially filled, '2' = Filled (Tag 39)
};

class FIXParser {
public:
    static FIXMessage parse(const std::string& raw_msg) {
        FIXMessage msg{};
        msg.msg_type = '\0';
        msg.side = '\0';
        msg.order_qty = 0;
        msg.price = 0;
        msg.ord_type = '\0';

        std::stringstream ss(raw_msg);
        std::string token;
        
        while (std::getline(ss, token, '|')) {
            if (token.empty()) continue;
            
            size_t eq_pos = token.find('=');
            if (eq_pos == std::string::npos) continue;

            std::string tag_str = token.substr(0, eq_pos);
            std::string value_str = token.substr(eq_pos + 1);

            int tag = std::stoi(tag_str);

            switch (tag) {
                case 8:  break; // BeginString (e.g. FIX.4.2)
                case 35: msg.msg_type = value_str[0]; break;
                case 49: msg.sender_comp_id = value_str; break;
                case 56: msg.target_comp_id = value_str; break;
                case 11: msg.cl_ord_id = value_str; break;
                case 55: msg.symbol = value_str; break;
                case 54: msg.side = value_str[0]; break;
                case 38: msg.order_qty = std::stoul(value_str); break;
                case 44: msg.price = std::stoull(value_str); break;
                case 40: msg.ord_type = value_str[0]; break;
                default: break; // Ignore unknown tags
            }
        }

        if (msg.msg_type != 'D') {
            throw std::invalid_argument("Parser currently only supports New Order Single (D)");
        }
        
        return msg;
    }
};

class FIXGateway {
public:
    FIXGateway(OrderBook& book) : order_book_(book), internal_order_id_(1) {}

    std::string submit(const std::string& raw_fix_msg) {
        try {
            FIXMessage fix_msg = FIXParser::parse(raw_fix_msg);
            
            Order axiom_order;
            // Generate standard sequential internal ID
            axiom_order.order_id = internal_order_id_.fetch_add(1, std::memory_order_relaxed);
            
            // Map side ('1'=Buy, '2'=Sell) -> Enum
            if (fix_msg.side == '1') {
                axiom_order.side = Side::BUY;
            } else if (fix_msg.side == '2') {
                axiom_order.side = Side::SELL;
            } else {
                return generate_reject(fix_msg, "Invalid Side");
            }

            // Map order type ('1'=Market, '2'=Limit) -> Enum
            if (fix_msg.ord_type == '1') {
                axiom_order.type = OrderType::MARKET;
            } else if (fix_msg.ord_type == '2') {
                axiom_order.type = OrderType::LIMIT;
            } else {
                return generate_reject(fix_msg, "Invalid Order Type");
            }
            
            axiom_order.quantity = fix_msg.order_qty;
            axiom_order.price = fix_msg.price;
            
            // Push to ring buffer (Lock-Free)
            bool accepted = order_book_.submit_order(axiom_order);
            
            if (accepted) {
                // Instantly return EXecution Report [MsgType=8, ExecType=New(0), OrdStatus=New(0)]
                return generate_execution_report(fix_msg, axiom_order.order_id, '0', '0');
            } else {
                return generate_reject(fix_msg, "Engine Queue Full");
            }
            
        } catch (const std::exception& e) {
            // Unparseable FIX message throws
            return "8=FIX.4.2|35=3|58=" + std::string(e.what()) + "|"; // Reject
        }
    }

private:
    OrderBook& order_book_;
    std::atomic<uint64_t> internal_order_id_;

    // Generates a mock FIX timestamp wrapper locally
    std::string get_fix_time() {
        return "20260309-01:00:00.000"; // Mocking for pure logical demonstration
    }

    std::string generate_execution_report(const FIXMessage& original, uint64_t internal_id, char exec_type, char ord_status) {
        std::stringstream ss;
        ss << "8=FIX.4.2|"
           << "35=8|" // Execution Report
           << "49=" << original.target_comp_id << "|"
           << "56=" << original.sender_comp_id << "|"
           << "11=" << original.cl_ord_id << "|"
           << "37=" << std::to_string(internal_id) << "|" // Internal Order ID
           << "17=" << std::to_string(internal_id + 10000) << "|" // Fake Exec ID mapping
           << "150=" << exec_type << "|" // Exec type
           << "39=" << ord_status << "|" // Order status
           << "55=" << original.symbol << "|"
           << "54=" << original.side << "|"
           << "38=" << original.order_qty << "|"
           << "44=" << original.price << "|"
           << "14=0|"  // CumQty (0 since just accepted)
           << "151=" << original.order_qty << "|"; // LeavesQty
        return ss.str();
    }

    std::string generate_reject(const FIXMessage& msg, const std::string& reason) {
        std::stringstream ss;
        ss << "8=FIX.4.2|"
           << "35=8|" // Execution Report
           << "49=" << msg.target_comp_id << "|"
           << "56=" << msg.sender_comp_id << "|"
           << "11=" << msg.cl_ord_id << "|"
           << "37=NONE|" 
           << "17=REJ|"
           << "150=8|" // ExecType = Rejected
           << "39=8|"  // OrdStatus = Rejected
           << "58=" << reason << "|"; // Text reason
        return ss.str();
    }
};

} // namespace axiom
