#ifndef YINHE_SRC_ENGINE_ORDERBOOK_H
#define YINHE_SRC_ENGINE_ORDERBOOK_H

#include <map>
#include "order.hpp"
#include "levelInfo.hpp"
#include "tradeUtils/trade.hpp"

struct orderEntry {
    order_ptr order_;
    order_ptr_list::iterator order_itr;
};

using levelInfos = std::vector<levelInfo>;

class OrderbookLevelInfos {
public: 
    OrderbookLevelInfos(const levelInfos& bids, const levelInfos& asks) :
        bids_(bids),
        asks_(asks) {}
    const levelInfos& get_bids() {return bids_;}
    const levelInfos& get_asks() {return asks_;}
private:
    const levelInfos& bids_;
    const levelInfos& asks_;
};

class Orderbook {
public:
    Trades match(); /*matches bids and asks and returns vector of resulting trades*/
    std::size_t get_size();
    void print_levels(); /*print levels of the orderbook*/
    int cancel_order(OrderID order_id); /*returns 0 on successful deletion*/
    void add_order(Side side, Price price, Quantity quantity); /*generates order and adds to orderbook*/
    Order get_order(OrderID order_id);

private: 
    /*store bids and asks as a map of prices to a list of orderpointers*/
    /*bids is sorted in decending order, asks sorted in ascending order*/
    std::map<Price,order_ptr_list, std::greater<Price> > bids_;
    std::map<Price,order_ptr_list> asks_;

    //map OrderIDs to list of orderpointers for ease of search based on ID
    std::unordered_map<OrderID, orderEntry> orders_; 

    bool can_match(Side side,Price price); /*check if order can be matched, used internally for can_fully_fill()*/
    bool can_fully_fill(Side side, Price price, Quantity quantity); /*check if an order can be fully filled, for fill or kill orders*/
    uint32_t get_level_quantity(Side side, Price price);
    OrderbookLevelInfos get_levelInfos();
};

#endif