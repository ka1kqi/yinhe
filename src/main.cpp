#include <iostream>

#include "engine/order.hpp"
#include "common/enums.hpp"
#include "common/types.hpp"

int main() {
    Side side_ = BUY;
    Price p = 1;
    Quantity q = 10;
    OrderID id = 1ULL << 35;
    Order order(side_,id,p,q);
    std::cout << "ID: " << std::to_string(id) <<  "\nPrice: "  << std::to_string(p)  << "\nQuantity: " << std::to_string(q) << std::endl;
    return 0;
}