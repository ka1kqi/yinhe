#ifndef YINHE_SRC_COMMON_ORDERLOG_H
#define YINHE_SRC_COMMON_ORDERLOG_H

/*
 * logging for operations
 * We save our log as a text file
 * Keeps track of the tick of the simulation as well, incremented by 1000
 * 
*/
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "order.hpp"
#include "tradeUtils/trade.hpp"

using Log = std::ofstream;

class OrderbookLogger {
public:
    /*TAKES A DIRECTORY*/
    void set_logfile_save_location(std::string filepath) {
        /*eventually add checking to ensure that filepath is a valid directory*/
        if(!std::filesystem::is_directory(filepath)) {
            std::cerr << ("Invalid filepath: provided filepath is not a directory\n");
            std::exit(1); /*fatal exception if logfile location is wrong*/
        }
        CUSTOM_LOGFILE_SAVE_LOCATION = filepath;
    }
    void init_Log(){
        std::string logfile_location = CUSTOM_LOGFILE_SAVE_LOCATION.empty() ? DEFAULT_LOGFILE_SAVE_LOCATION : CUSTOM_LOGFILE_SAVE_LOCATION;
        logFile = std::ofstream(logfile_location,std::ios::app);
        curLogTick = 0;
        std::cout << "Log file opened " << std::endl;
        std::cout << "Tick: " << std::to_string(curLogTick) << std::endl;
        logFile << "Initial log" << std::endl;
    }

    /*TAKES TRADE POINTER*/
    void log_Trade(Trade *trade_,SimTick simulation_tick_time){
        const auto& bid_trade = trade_->get_bid_info();
        const auto& ask_trade = trade_->get_ask_info();
        logFile << std::to_string(simulation_tick_time) << " | " << std::to_string(ask_trade.orderID_) << " | " << 
                    std::to_string(bid_trade.orderID_) << " | " << std::to_string(ask_trade.price_) << 
                    " | " << std::to_string(ask_trade.quantity_) << std::endl;
    }

    void log_order_Error(OrderID err_order_id){
        logFile << "Error with order: " << std::to_string(err_order_id);
    }

    void close_Log() {
        logFile.close();
    }

private:
    const std::string DEFAULT_LOGFILE_SAVE_LOCATION = "yinhe/docs/logs";
    std::string CUSTOM_LOGFILE_SAVE_LOCATION;    
    static Log logFile;
    static SimTick curLogTick;
};

/*need to add mutex support eventually to prevent concurrent edits*/
#endif