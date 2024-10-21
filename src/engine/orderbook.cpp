#include <iostream>

#include "order.hpp"
#include "orderbook.hpp"
#include "trade.hpp"
#include "orderLog.hpp"

//need to cancel goodforday orders if its the end of the day
/*check simulation tick*/
Trades Orderbook::match() {
    Trades trades;
    /*reserve size in case we can match all orders for no memory problems later on*/
    trades.reserve(orders_.size());

    while(true) {
        if(bids_.empty() || asks_.empty())
            break;
        
        auto& [bid_price,bids] = *bids_.begin();
        auto& [ask_price,asks] = *asks_.begin();
        if(bid_price < ask_price)
            return {}; /*can't match if best bid is lower than best ask for current level*/

        /*match in current level until empty*/
        while(!bids.empty() && !asks.empty()){
            auto bid = bids.front();
            auto ask = asks.front();

            Quantity trade_quantity = std::min(bid->get_remaining_quantity(),ask->get_remaining_quantity());
            bid->fill(trade_quantity);
            ask->fill(trade_quantity);

            if(bid->isFilled()){
                bids.pop_front();
                orders_.erase(bid->get_order_id());
            }
            if(ask->isFilled()){
                asks.pop_front();
                orders_.erase(ask->get_order_id());
            }
            /*trade is settled at ask price if the bid price is higher than ask*/
            /*push the new trade to the back of the trades vector*/
            trades.push_back(Trade(tradeInfo{bid->get_order_id(),ask_price,trade_quantity},
                                    tradeInfo{ask->get_order_id(),ask_price,trade_quantity}));
        }
        if(bids.empty()) /*erase current price level if there are no more orders at the level*/
            bids_.erase(bid_price);
        if(asks.empty())    
            asks_.erase(ask_price);
    }
    /*prune fill and kill / fill or kill orders*/
    return trades;
}

bool Orderbook::can_fully_fill(Side side, Price price, Quantity quantity) {
    if(!can_match(side,price))
        return false;
    
}

bool Orderbook::can_match(Side side, Price price) {
    if(side == Side::BUY) {
        /*check in sell orders*/
        /*if best sell is higher than the bid price, then we can't match*/
        if(asks_.empty()) /*cannot match if there are no orders to match with*/
            return false;
        const auto [ask,_] = *asks_.begin(); 
        return ask <= price;
    } 
    else {
        /*we check buy orders to match a sell order*/
        if(bids_.empty())
            return false;
        const auto [bid,_] = *bids_.begin();
        return bid >= price;
    }
    return true;
}

std::size_t Orderbook::get_size(){
    return orders_.size();
}

/*accumulate total quantity of price level from specified side of the orderbook*/
uint32_t Orderbook::get_level_quantity(Side side, Price price) {
    order_ptr_list orderbook_side = (side == Side::BUY) ? bids_[price]:asks_[price];
    uint32_t side_total_quantity = 0;
    for(auto& it : orderbook_side) {
        side_total_quantity+=it->get_remaining_quantity(); /*sum the quantity from every order on the price level*/
    }
    return side_total_quantity;
}

OrderbookLevelInfos Orderbook::get_levelInfos(){
    levelInfos bidInfos,askInfos;
    bidInfos.reserve(orders_.size());
    askInfos.reserve(orders_.size());

    for(const auto& [price,_] : bids_) {
        Quantity level_quantity = get_level_quantity(Side::BUY, price);
        bidInfos.push_back(levelInfo{price,level_quantity});
    }
    for(const auto& [price,_] : asks_) {
        Quantity level_quantity = get_level_quantity(Side::SELL, price);
        askInfos.push_back(levelInfo{price,level_quantity});
    }
    return OrderbookLevelInfos(bidInfos,askInfos);
}

/*print levels of the orderbook*/
void Orderbook::print_levels() {
    auto orderbook_level_infos = get_levelInfos();
    uint32_t idx = 0; /*keep track of how many levels there are to organize printing*/
    auto& bids = orderbook_level_infos.get_bids();
    auto& asks = orderbook_level_infos.get_asks();

    while(!asks.empty() && !bids.empty()) {

    }
}