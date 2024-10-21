#ifndef YINHE_SRC_ENGINE_TRADEUTILS_TRADE_H
#define YINHE_SRC_ENGINE_TRADEUTILS_TRADE_H

#include "order.hpp"

//store side of trade
struct tradeInfo {
    OrderID orderID_;
    Price price_;
    Quantity quantity_;
};

//stores sell side and buy side of trade in tradeInfo struct
class Trade {
public:
    Trade(const tradeInfo& bid, const tradeInfo& ask) :
        bid_(bid),
        ask_(ask) {}

    tradeInfo get_bid_info() {
        return bid_;
    }
    tradeInfo get_ask_info(){
        return ask_;
    }
private:
    tradeInfo bid_;
    tradeInfo ask_;
};

using Trades = std::vector<Trade>;
#endif