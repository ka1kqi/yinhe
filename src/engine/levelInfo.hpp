#ifndef YINHE_SRC_ENGINE_LEVELINFO_H
#define YINHE_SRC_ENGINE_LEVELINFO_H

#include <cstdint>

/*
 * level of a side, eg. if there are 3 buy order at $100 for quantity 3, 10, 15
 * levelInfo price = 100, quantity = 28
*/
struct levelInfo {
    Price price;
    Quantity quantity;
};
#endif