#include <iostream>

#include <types.h>
#include "order.h"

Order::Order(side side_, OrderID orderId_, Price price_, Quantity quantity_) :
    order_side{side_},
    id(orderId_),
    price(price_),
    quantity(quantity_) {};

side Order::get_order_side() {
    return order_side;
}

OrderID Order::get_order_id() {
    return id;
}

Price Order::get_order_price() {
    return price;
}

Quantity Order::get_order_quantity() {
    return quantity; 
}