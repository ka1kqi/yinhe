#ifndef YINHE_SRC_COMMON_TYPES_H
#define YINHE_SRC_COMMON_TYPES_H

#include <cstdint>

/**************************************
 * namespace type aliases for price, quantity, and OrderID
 * Price: 32-bit unsigned integer
 * Quantity: 32-bit unsigned integer
 * OrderID: 64-bit unsigned integer
 **************************************/
using Price = std::uint32_t;
using Quantity = std::uint32_t;
using OrderID = std::uint64_t;

/*log ticks in millisecond*/
using SimTick = std::uint64_t;

#endif