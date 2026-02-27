#include "order.hpp"
#include "types.hpp"

Order::Order(Side side_, OrderID orderId_, Price price_, Quantity quantity_,
             orderType type_ = orderType::LIMIT)
    : order_side(side_), id(orderId_), price(price_), init_quantity(quantity_),
      remain_quantity(quantity_), order_type(type_) {};

Side Order::get_order_side() { return order_side; }

OrderID Order::get_order_id() { return id; }

Price Order::get_order_price() { return price; }

Quantity Order::get_init_quantity() { return init_quantity; }

Quantity Order::get_remaining_quantity() { return remain_quantity; }

Quantity Order::get_filled_quantity() {
  return init_quantity - remain_quantity;
}

orderType Order::get_order_type() { return order_type; }

bool Order::isFilled() { return get_remaining_quantity() == 0; }

void Order::fill(Quantity quantity) {
  if (quantity > remain_quantity)
    throw std::logic_error("Order cannot be filled");
  remain_quantity -= quantity;
}