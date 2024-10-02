#ifndef YINHE_SRC_ENGINE_ORDER_H
#define YINHE_SRC_ENGINE_ORDER_H

#include <cstdint>
#include <enums.hpp>
#include <types.hpp>

using Side = side;

class Order {
public:
    Order(Side side_, OrderID orderId_, Price price_, Quantity quantity_);
    
    side get_order_side();

    OrderID get_order_id();
    
    Price get_order_price();

    Quantity get_order_quantity();

private:
    Side order_side;
    OrderID id;
    Price price;
    Quantity quantity;
};

#endif