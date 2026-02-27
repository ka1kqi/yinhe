#ifndef YINHE_SRC_ENGINE_ORDERBOOK_H
#define YINHE_SRC_ENGINE_ORDERBOOK_H

#include "levelInfo.hpp"
#include "order.hpp"
#include "orderLog.hpp"
#include "tradeUtils/trade.hpp"
#include <map>
#include <string>

constexpr bool ENABLE_LOGGER = true;
constexpr bool CLEAR_LOGS_ON_INIT = true;

struct orderEntry {
  order_list::iterator itr;
};

using levelInfos = std::vector<levelInfo>;

class OrderbookLevelInfos {
public:
  OrderbookLevelInfos(levelInfos bids, levelInfos asks)
      : bids_(std::move(bids)), asks_(std::move(asks)) {}
  const levelInfos &get_bids() const { return bids_; }
  const levelInfos &get_asks() const { return asks_; }

private:
  levelInfos bids_;
  levelInfos asks_;
};

class Orderbook {
public:
  Orderbook();
  Orderbook(std::string logfile_location);
  [[nodiscard]] std::size_t get_size();
  void print_levels();                       /*print levels of the orderbook*/
  int cancel_order(OrderID cancel_order_id); /*returns 0 on successful deletion,
                                                -1 if not found*/
  [[nodiscard]] Order get_order(OrderID get_order_id);
  [[nodiscard]] Trades
  add_order(Side side, Price price, Quantity quantity,
            orderType type); /*generates order and calls
                                                    internal add_order_ptr*/

private:
  /*store bids and asks as a map of prices to a list of orderpointers*/
  /*bids is sorted in decending order, asks sorted in ascending order*/
  std::map<Price, order_list, std::greater<Price>> bids_;
  std::map<Price, order_list, std::less<Price>> asks_;

  /*map OrderIDs to list of orderpointers for ease of search based on ID*/
  /*we don't worry about order since we only search based on ID*/
  std::unordered_map<OrderID, orderEntry> orders_;

  OrderbookLogger Logger;
  SimTick last_sim_tick;

  Trades
  match(); /*matches bids and asks and returns vector of resulting trades*/
  [[nodiscard]] Trades
  add_order_ptr(Order add_order);         /*adds order to orderbook*/
  bool can_match(Side side, Price price); /*check if order can be matched, used
                                             internally for can_fully_fill()*/
  bool can_fully_fill(Side side, Price price,
                      Quantity quantity); /*check if an order can be fully
                                             filled, for fill or kill orders*/
  bool can_fully_fill_unchecked(Side side, Price price, Quantity quantity);
  Quantity get_level_quantity(const order_list &level_orders);
  [[nodiscard]] OrderbookLevelInfos get_levelInfos();
  OrderID gen_order_id();
  uint64_t next_order_id_ = 0;
  void flush_orderbook();

  friend class OrderbookTest;
  friend class OrderbookBench;
};

#endif