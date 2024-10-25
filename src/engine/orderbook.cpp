#include <iostream>
#include <memory>
#include <random>

#include "order.hpp"
#include "orderbook.hpp"
#include "trade.hpp"
#include "orderLog.hpp"

/*change to false if you don't want to create log files when testing*/

//cancel goodforday orders if its the end of the day
/*check simulation tick*/
/*TODO FINISH*/

/*intialize orderbook with optional logfile with optional location specifier*/
Orderbook::Orderbook() {
    std::cout << "Initializing OrderbookLogger" << std::endl;
    if(ENABLE_LOGGER){
        if(CLEAR_LOGS_ON_INIT)
            Logger.flush_log_Dir();
        Logger.init_Log();
    }
    last_sim_tick=0;
}

Orderbook::Orderbook(std::string logfile_location) {
    if(ENABLE_LOGGER) {
        Logger.set_logfile_save_location(logfile_location);
        Orderbook();
    }
}


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
            break; /*can't match if best bid is lower than best ask for current level*/

        /*match in current level until empty*/
        while(!bids.empty() && !asks.empty()){
            auto bid = bids.front();
            auto ask = asks.front();

            /*we can trade at most the minimum quantity between the two orders*/
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

            /*trade is settled at ask price if the bid price is higher than ask for simplicity*/
            /*push the new trade to the back of the trades vector*/
            trades.push_back(Trade(tradeInfo{bid->get_order_id(),ask_price,trade_quantity},
                                    tradeInfo{ask->get_order_id(),ask_price,trade_quantity}));
            
            if(ENABLE_LOGGER)
                Logger.log_Trade(&trades.back(),last_sim_tick);
        }
        if(bids.empty()) /*erase current price level if there are no more orders at the level*/
            bids_.erase(bid_price);
        if(asks.empty())    
            asks_.erase(ask_price);
    }
    
    /*prune fill and kill / fill or kill orders*/
    /*if fill and kill at the beinning then we need to remove*/
    /*TODO*/

    if(!bids_.empty()) {
        ;
    }
    if(!asks_.empty()){
        ;
    }

    return trades;
}

/*TODO*/
bool Orderbook::can_fully_fill(Side side, Price price, Quantity quantity) {
    if(!can_match(side,price)) /*if we can't match prices then we don't worry about fully filling*/
        return false;

    if(side == Side::BUY) {
        ;
    }
    else {
        ;
    }
    return true;
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
}

/*does something random*/
const OrderID Orderbook::gen_order_id() const {
    std::time_t t = std::time(0);
    std::tm* tm_now = std::localtime(&t);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0,64);

    OrderID ID = 1 << distr(gen);
    ID += tm_now->tm_min + tm_now->tm_sec;
    ID *= tm_now->tm_hour*tm_now->tm_min*tm_now->tm_sec;
    ID = ID << distr(gen);
    return ID;
}

[[nodiscard]] Trades Orderbook::add_order_ptr(order_ptr add_order_) {
    Side side = add_order_->get_order_side();
    auto& v = (side==Side::BUY) ? bids_[add_order_->get_order_price()] : asks_[add_order_->get_order_price()];
    v.push_back(add_order_);
    /*add to orders_ map*/
    order_ptr_list::iterator it;
    it = std::prev(v.end());
    orders_.insert({add_order_->get_order_id(),orderEntry{add_order_,it}});
    
    /*return matched orders*/
    return match();
}

[[nodiscard]] Trades Orderbook::add_order(Side side, Price price, Quantity quantity) {
    const auto& ID = gen_order_id();
    Order new_order(side,ID,price,quantity);
    order_ptr new_order_ptr = std::make_shared<Order>(new_order);
    return add_order_ptr(new_order_ptr);
}  

/*cancel order, return 0 on successful deletion and -1 on unsuccessful deletion*/
int Orderbook::cancel_order(OrderID cancel_order_id) {
    if(!orders_.count(cancel_order_id))
        return -1;
    
    return 0;
}

/*delete all orders*/
void Orderbook::flush_orderbook() {
    Logger.log_message("Flushing orderbook",last_sim_tick);
    for(auto [order,_] : orders_) {
        cancel_order(order);
    }
}

std::size_t Orderbook::get_size(){
    return orders_.size();
}

/*accumulate total quantity of price level from specified side of the orderbook*/
uint32_t Orderbook::get_level_quantity(order_ptr_list orderbook_side) {
    Quantity side_total_quantity = 0;
    for(auto& order : orderbook_side) {
        side_total_quantity+=order->get_remaining_quantity(); /*sum the quantity from every order on the price level*/
    }
    return side_total_quantity;
}

[[nodiscard]] OrderbookLevelInfos Orderbook::get_levelInfos(){
    levelInfos bidInfos,askInfos;
    bidInfos.reserve(orders_.size());
    askInfos.reserve(orders_.size());

    for(const auto& [price,level_orders] : bids_) {
        Quantity level_quantity = get_level_quantity(level_orders);
        bidInfos.push_back(levelInfo{price,level_quantity});
    }
    for(const auto& [price,level_orders] : asks_) {
        Quantity level_quantity = get_level_quantity(level_orders);
        askInfos.push_back(levelInfo{price,level_quantity});
    }
    return OrderbookLevelInfos(bidInfos,askInfos);
}

/*print levels of the orderbook*/
void Orderbook::print_levels() {
    OrderbookLevelInfos orderbook_level_infos = get_levelInfos();
    uint32_t current_price_level_idx = 0; /*keep track of how many levels there are to organize printing*/

    /*Joe做这个*/
    /* print example:
         BID_PRICE1     BID_QUANTITY1       BID_PRICE2      BID_QUANTITY2       ASK_PRICE1      ASK_QUANTITY1       ASK_PRICE2      ASK_QUANTITY2
    1    100.51            50               98.13           48                  100.40          20                  100.68          24   
    2    101.30            25               ...
    .
    .
    */

    const levelInfos& bids = orderbook_level_infos.get_bids();
    const levelInfos& asks = orderbook_level_infos.get_asks();

    if(!bids.empty()) {
        for(auto& level : bids) {   
            std::cout << "Bid Price: " << level.price << " | Quantity: " << level.quantity << std::endl;
        }
    }
    if(!asks.empty()){
        for(auto& level : asks) {   
            std::cout << "Ask Price: " << level.price << " | Quantity: " << level.quantity << std::endl;
        }
    }

    /*
    while(!asks.empty() && !bids.empty()) {
        break;
    }
    */
} 

