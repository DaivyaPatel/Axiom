#include <iostream>
#include <string>
#include <vector>
#include "../include/OrderBook.hpp"
#include "../include/FIXGateway.hpp"

using namespace axiom;

int main() {
    std::cout << "Starting AXIOM FIX Gateway Test Engine..." << std::endl;
    
    OrderBook book;
    FIXGateway gateway(book);
    
    // Test dataset of raw FIX 4.2 string messages (pipe separated)
    std::vector<std::string> messages = {
        "8=FIX.4.2|35=D|49=CLIENT1|56=AXIOM|11=ORD001|55=AAPL|54=1|38=100|44=15000|40=2|",
        "8=FIX.4.2|35=D|49=CLIENT2|56=AXIOM|11=ORD002|55=TSLA|54=2|38=50|44=20000|40=2|",
        "8=FIX.4.2|35=D|49=CLIENT1|56=AXIOM|11=ORD003|55=MSFT|54=1|38=500|44=0|40=1|",    // Market Order
        "8=FIX.4.2|35=D|49=ALGO|56=AXIOM|11=ORD004|55=NVDA|54=3|38=100|44=12000|40=2|"   // Invalid side '3'
    };
    
    std::cout << "\n--- Submitting Messages ---" << std::endl;
    
    for (const auto& raw_msg : messages) {
        std::cout << "\nIncoming : " << raw_msg << std::endl;
        
        std::string exec_report = gateway.submit(raw_msg);
        
        std::cout << "Outgoing : " << exec_report << std::endl;
    }
    
    std::cout << "\n--- Processing OrderBook ---" << std::endl;
    book.process_queue();
    
    std::cout << "Engine successfully processed " << book.order_count() << " orders." << std::endl;
    
    return 0;
}
