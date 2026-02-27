#include "order.hpp"
#include "types.hpp"

Order::Order(Side side_, OrderID orderId_, Price price_, Quantity quantity_,
             orderType type_)
    : id(orderId_), price(price_), init_quantity(quantity_),
      remain_quantity(quantity_), order_side(side_), order_type(type_) {}

void Order::fill(Quantity quantity) {
  if (quantity > remain_quantity)
    throw std::logic_error("Order cannot be filled");
  remain_quantity -= quantity;
}
