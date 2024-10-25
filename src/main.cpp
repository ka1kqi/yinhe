#include <iostream>

#include "engine/orderbook.hpp"
#include "common/orderLog.hpp"

int main() {
    Orderbook orderbook;
    Trades trades;
    trades = orderbook.add_order(Side::BUY,100,50);
    
    orderbook.print_levels();
    return 0;
}