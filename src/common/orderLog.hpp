#ifndef YINHE_SRC_COMMON_ORDERLOG_H
#define YINHE_SRC_COMMON_ORDERLOG_H

/*
 * logging for operations
 * We save our log as a text file
 * Keeps track of the tick of the simulation as well, incremented by 1000
 * 
*/
#include <iostream>
#include <fstream>
#include "types.hpp"
#include "enums.hpp"
#include "tradeUtils/trade.hpp"

using Log = std::ofstream;

Log logFile;
SimTick curLogTick;

void init_log(){
    logFile = std::ofstream("log.txt",std::ios::app);
    curLogTick = 0;
    std::cout << "Log file opened " << std::endl;
    std::cout << "Tick: " << std::to_string(curLogTick) << std::endl;
}

void log_Trade(Trade *trade_,SimTick tick){
    
}

#endif