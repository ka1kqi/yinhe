#include <iostream>
#include <limits>
#include <memory>
#include <random>

#include "order.hpp"
#include "orderLog.hpp"
#include "orderbook.hpp"
#include "trade.hpp"

/*change to false if you don't want to create log files when testing*/

// cancel goodforday orders if its the end of the day
/*check simulation tick*/
/*TODO FINISH*/

/*intialize orderbook with optional logfile with optional location specifier*/
Orderbook::Orderbook() {
  std::cout << "Initializing OrderbookLogger" << std::endl;
  if (ENABLE_LOGGER) {
    if (CLEAR_LOGS_ON_INIT)
      Logger.flush_log_Dir();
    Logger.init_Log();
  }
  last_sim_tick = 0;
}

Orderbook::Orderbook(std::string logfile_location) {
  if (ENABLE_LOGGER) {
    Logger.set_logfile_save_location(logfile_location);
    Orderbook();
  }
}

Trades Orderbook::match() {
  Trades trades;
  /*reserve size in case we can match all orders for no memory problems later
   * on*/
  trades.reserve(orders_.size());

  while (true) {
    if (bids_.empty() || asks_.empty())
      break;

    auto &[bid_price, bids] = *bids_.begin();
    auto &[ask_price, asks] = *asks_.begin();

    if (bid_price < ask_price)
      break; /*can't match if best bid is lower than best ask for current
                level*/

    /*match in current level until empty*/
    while (!bids.empty() && !asks.empty()) {
      auto bid = bids.front();
      auto ask = asks.front();

      /*reject fill or kill orders that cannot be fully filled*/
      if (bid->get_order_type() == orderType::FILLORKILL &&
          !can_fully_fill(Side::BUY, bid->get_order_price(),
                          bid->get_remaining_quantity())) {
        bids.pop_front();
        orders_.erase(bid->get_order_id());
        continue;
      }
      if (ask->get_order_type() == orderType::FILLORKILL &&
          !can_fully_fill(Side::SELL, ask->get_order_price(),
                          ask->get_remaining_quantity())) {
        asks.pop_front();
        orders_.erase(ask->get_order_id());
        continue;
      }

      /*we can trade at most the minimum quantity between the two orders*/
      Quantity trade_quantity = std::min(bid->get_remaining_quantity(),
                                         ask->get_remaining_quantity());
      bid->fill(trade_quantity);
      ask->fill(trade_quantity);

      if (bid->isFilled()) {
        bids.pop_front();
        orders_.erase(bid->get_order_id());
      }
      if (ask->isFilled()) {
        asks.pop_front();
        orders_.erase(ask->get_order_id());
      }

      /*trade is settled at ask price if the bid price is higher than ask for
       * simplicity*/
      /*push the new trade to the back of the trades vector*/
      trades.push_back(
          Trade(tradeInfo{bid->get_order_id(), ask_price, trade_quantity},
                tradeInfo{ask->get_order_id(), ask_price, trade_quantity}));

      if (ENABLE_LOGGER)
        Logger.log_Trade(&trades.back(), last_sim_tick);
    }
    if (bids.empty()) /*erase current price level if there are no more orders at
                         the level*/
      bids_.erase(bid_price);
    if (asks.empty())
      asks_.erase(ask_price);
  }

  /*prune fill and kill and fill or kill orders that are not fully filled*/
  if (!bids_.empty()) {
  }
  if (!asks_.empty()) {
    ;
  }

  return trades;
}

bool Orderbook::can_fully_fill(Side side, Price price, Quantity quantity) {
  if (!can_match(side, price))
    return false;

  if (side == Side::BUY) {
    for (auto it = asks_.begin(); it != asks_.end(); ++it) {
      auto &[ask_price, asks] = *it;
      if (ask_price > price)
        return false;
      for (auto &order : asks) {
        Quantity available = order->get_remaining_quantity();
        if (available >= quantity)
          return true;
        quantity -= available;
      }
    }
  } else {
    for (auto it = bids_.begin(); it != bids_.end(); ++it) {
      auto &[bid_price, bids] = *it;
      if (bid_price < price)
        return false;
      for (auto &order : bids) {
        Quantity available = order->get_remaining_quantity();
        if (available >= quantity)
          return true;
        quantity -= available;
      }
    }
  }
  return false;
}

bool Orderbook::can_match(Side side, Price price) {
  if (side == Side::BUY) {
    /*check in sell orders*/
    /*if best sell is higher than the bid price, then we can't match*/
    if (asks_.empty()) /*cannot match if there are no orders to match with*/
      return false;
    const auto [ask, _] = *asks_.begin();
    return ask <= price;
  } else {
    /*we check buy orders to match a sell order*/
    if (bids_.empty())
      return false;
    const auto [bid, _] = *bids_.begin();
    return bid >= price;
  }
}

/*generate a random order id, retries on collision*/
const OrderID Orderbook::gen_order_id() const {
  static std::mt19937_64 gen(std::random_device{}());
  std::uniform_int_distribution<OrderID> distr(
      1, std::numeric_limits<OrderID>::max());

  OrderID ID;
  do {
    ID = distr(gen);
  } while (orders_.count(ID));
  return ID;
}

[[nodiscard]] Trades Orderbook::add_order_ptr(order_ptr add_order_) {
  Side side = add_order_->get_order_side();
  auto &v = (side == Side::BUY) ? bids_[add_order_->get_order_price()]
                                : asks_[add_order_->get_order_price()];
  v.push_back(add_order_);
  /*add to orders_ map*/
  order_ptr_list::iterator it;
  it = std::prev(v.end());
  orders_.insert({add_order_->get_order_id(), orderEntry{add_order_, it}});

  /*return matched orders*/
  return match();
}

[[nodiscard]] Trades Orderbook::add_order(Side side, Price price,
                                          Quantity quantity,
                                          orderType type = orderType::LIMIT) {
  const auto &ID = gen_order_id();
  Order new_order(side, ID, price, quantity, type);
  order_ptr new_order_ptr = std::make_shared<Order>(new_order);
  return add_order_ptr(new_order_ptr);
}

/*cancel order, return 0 on successful deletion and -1 on unsuccessful
 * deletion*/
int Orderbook::cancel_order(OrderID cancel_order_id) {
  if (!orders_.count(cancel_order_id))
    return -1;

  const auto &[order, itr] = orders_.at(cancel_order_id);
  Price price = order->get_order_price();
  Side side = order->get_order_side();

  if (side == Side::BUY) {
    auto &level = bids_.at(price);
    level.erase(itr);
    if (level.empty())
      bids_.erase(price);
  } else {
    auto &level = asks_.at(price);
    level.erase(itr);
    if (level.empty())
      asks_.erase(price);
  }

  orders_.erase(cancel_order_id);
  return 0;
}

/*delete all orders*/
void Orderbook::flush_orderbook() {
  Logger.log_message("Flushing orderbook", last_sim_tick);
  for (auto [order, _] : orders_) {
    cancel_order(order);
  }
}

std::size_t Orderbook::get_size() { return orders_.size(); }

/*accumulate total quantity of price level from specified side of the
 * orderbook*/
uint32_t Orderbook::get_level_quantity(order_ptr_list orderbook_side) {
  Quantity side_total_quantity = 0;
  for (auto &order : orderbook_side) {
    side_total_quantity +=
        order->get_remaining_quantity(); /*sum the quantity from every order on
                                            the price level*/
  }
  return side_total_quantity;
}

[[nodiscard]] OrderbookLevelInfos Orderbook::get_levelInfos() {
  levelInfos bidInfos, askInfos;
  bidInfos.reserve(orders_.size());
  askInfos.reserve(orders_.size());

  for (const auto &[price, level_orders] : bids_) {
    Quantity level_quantity = get_level_quantity(level_orders);
    bidInfos.push_back(levelInfo{price, level_quantity});
  }
  for (const auto &[price, level_orders] : asks_) {
    Quantity level_quantity = get_level_quantity(level_orders);
    askInfos.push_back(levelInfo{price, level_quantity});
  }
  return OrderbookLevelInfos(bidInfos, askInfos);
}