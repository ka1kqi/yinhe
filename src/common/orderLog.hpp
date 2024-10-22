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
#include <exception>
#include <ctime>
#include "order.hpp"
#include "trade.hpp"

using Log = std::ofstream;
namespace fs = std::filesystem;

class OrderbookLogger {
public:
    /*TAKES A DIRECTORY*/
    /*provided if you want to specify a new directory to store logs*/
    void set_logfile_save_location(std::string filepath) {
        if(!std::filesystem::is_directory(filepath)) { 
            std::cerr << ("Invalid filepath: provided filepath is not a directory\n");
            std::exit(1); /*fatal error if logfile location is invalid*/
        }
        CUSTOM_LOGFILE_SAVE_LOCATION = filepath;
    }

    void init_Log() {
        logfile_location = CUSTOM_LOGFILE_SAVE_LOCATION.empty() ? DEFAULT_LOGFILE_SAVE_LOCATION + logfile_name : 
                                                                  CUSTOM_LOGFILE_SAVE_LOCATION + logfile_name;
        logFile = std::ofstream(logfile_location,std::ios::app);
        if(!logFile.is_open()) {
            std::cerr << "Error opening logger file, exiting program" << std::endl;
            std::exit(1); /*fatal error*/
        }
        logFile << "Log opened" << std::endl;
        lastLogTick = 0;
        std::cout << "Opened: " << logfile_name << std::endl;
        logFile.close();
    }

    void log_message(std::string message, SimTick simulation_tick_time) {
        logFile = std::ofstream(logfile_location,std::ios::app);
        logFile << "\n-----------------------------------------------------------" << std::endl;
        logFile << std::to_string(simulation_tick_time) << " | MESSAGE:\n" << message << std::endl;
        logFile << "-----------------------------------------------------------\n" << std::endl;
        logFile.close();

    }

    /*TAKES TRADE POINTER*/
    /*log trade info into logfile*/
    void log_Trade(Trade *trade_,SimTick simulation_tick_time){
        const auto& bid_trade = trade_->get_bid_info();
        const auto& ask_trade = trade_->get_ask_info();
        logFile << std::to_string(simulation_tick_time) << " | " << std::to_string(ask_trade.orderID_) << " | " << 
                    std::to_string(bid_trade.orderID_) << " | " << std::to_string(ask_trade.price_) << 
                    " | " << std::to_string(ask_trade.quantity_) << std::endl;
    }

    /*TODO: add exceptions to specify the error that occurs*/
    void log_order_Error(OrderID err_order_id){
        logFile << "Error with order: " << std::to_string(err_order_id);
    }

    void close_Log() {
        logFile << "End logger" << std::endl;
        logFile << "Tick: " << std::to_string(lastLogTick) << std::endl;
        logFile.close();
        std::cout << "Closed logger" << std::endl;
    }

    /*flushes all files in log folder*/
    void flush_log_Dir() {
        auto LOG_DIRECTORY = CUSTOM_LOGFILE_SAVE_LOCATION.empty() ? DEFAULT_LOGFILE_SAVE_LOCATION : CUSTOM_LOGFILE_SAVE_LOCATION;
        try {
            for(const auto& entry : fs::directory_iterator(LOG_DIRECTORY)) {
                if(entry.is_regular_file()) {
                    fs::remove(entry.path());
                }
            }
            std::cout << "Removed all files in directory \"" << LOG_DIRECTORY << "\"" << std::endl;
        }   
        catch(const fs::filesystem_error& e) {
            std::cerr << "Error: " << e.what() << " while flushing log files" << std::endl;
        }
    }

    std::string get_logfile_location() {
        return (logfile_location.empty()) ? NULL : logfile_location;
    }

private:
    const std::string DEFAULT_LOGFILE_SAVE_LOCATION = "logs/";
    std::string CUSTOM_LOGFILE_SAVE_LOCATION; 
    const std::string logfile_name = generate_logfile_name();   
    std::string logfile_location;
    Log logFile;
    //most recent tick time that was updated
    SimTick lastLogTick;

    /*generate a logfile name based on current system time*/
    const std::string generate_logfile_name() const {
        std::time_t t = std::time(0);
        std::tm* tm_now = std::localtime(&t);
        return "log" + std::to_string(tm_now->tm_year) + std::to_string(tm_now->tm_mon) + std::to_string(tm_now->tm_mday)
                                + std::to_string(tm_now->tm_hour) + std::to_string(tm_now->tm_min) + std::to_string(tm_now->tm_sec) + ".log";
        /*precision to the nearest second*/
    }
};

/*need to add mutex support eventually to prevent concurrent edits*/
#endif