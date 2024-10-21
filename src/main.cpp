#include <iostream>

#include "engine/orderbook.hpp"
#include "common/orderLog.hpp"

int main() {
    Orderbook orderbook();
    OrderbookLogger logger;
    logger.flush_log_Dir();
    logger.init_Log();
    logger.log_message("logger is working!",1000);
    logger.close_Log();
    return 0;
}