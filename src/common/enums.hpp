#ifndef YINHE_SRC_COMMON_ENUMS_H
#define YINHE_SRC_COMMON_ENUMS_H

enum side {
    BUY,
    SELL
};

enum orderType {
    GOODTOCANCEL,
    FILLORKILL,
    MARKET,
    GOODFORDAY,
    FILLANDKILL
};

#endif