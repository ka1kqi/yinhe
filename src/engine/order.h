#ifndef YINHE_SRC_ENGINE_ORDER_H
#define YINHE_SRC_ENGINE_ORDER_H

#include <cstdint>
#include <side.h>
#include <types.h>

class Order {
public:
    Order(side side_, OrderID orderId_, Price price_, Quantity quantity_);
    
    side get_order_side();

    OrderID get_order_id();
    
    Price get_order_price();

    Quantity get_order_quantity();

private:
    side order_side;
    OrderID id;
    Price price;
    Quantity quantity;
};

#endif