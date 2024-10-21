#ifndef YINHE_SRC_ENGINE_ORDER_H
#define YINHE_SRC_ENGINE_ORDER_H

#include <deque>
#include <iostream>

#include "enums.hpp"
#include "types.hpp"

class Order {
public:
    Order(Side side_, OrderID orderId_, Price price_, Quantity quantity_);
    
    Side get_order_side();

    OrderID get_order_id();
    
    Price get_order_price();

    Quantity get_init_quantity();

    Quantity get_remaining_quantity();

    Quantity get_filled_quantity();

    bool isFilled();

    //return 1 if filled, -1 if error
    void fill(Quantity quantity_);

private:
    Side order_side;
    OrderID id;
    Price price;
    Quantity init_quantity;
    Quantity remain_quantity;
};

using order_ptr = std::shared_ptr<Order>;
using order_ptr_list = std::deque<order_ptr>;

#endif