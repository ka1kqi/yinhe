#ifndef YINHE_SRC_ENGINE_ORDER_H
#define YINHE_SRC_ENGINE_ORDER_H

#include <list>

#include "enums.hpp"
#include "types.hpp"

class Order {
public:
  Order(Side side_, OrderID orderId_, Price price_, Quantity quantity_,
        orderType type_ = orderType::FILLANDKILL);

  Side get_order_side() const noexcept { return order_side; }
  OrderID get_order_id() const noexcept { return id; }
  Price get_order_price() const noexcept { return price; }
  Quantity get_init_quantity() const noexcept { return init_quantity; }
  Quantity get_remaining_quantity() const noexcept { return remain_quantity; }
  Quantity get_filled_quantity() const noexcept { return init_quantity - remain_quantity; }
  orderType get_order_type() const noexcept { return order_type; }
  bool isFilled() const noexcept { return remain_quantity == 0; }

  void fill(Quantity quantity_);

private:
  OrderID id;
  Price price;
  Quantity init_quantity;
  Quantity remain_quantity;
  Side order_side;
  orderType order_type;
};

using order_list = std::list<Order>;

#endif
