#ifndef YINHE_SRC_COMMON_ENUMS_H
#define YINHE_SRC_COMMON_ENUMS_H

enum Side { BUY, SELL };

enum orderType {
  GOODTOCANCEL, // remains on the book until fully cancelled
  FILLORKILL,   // must be fully filled immediately or cancelled entirely
  MARKET,       // execute at best available price
  GOODFORDAY,   // remains on the book until end of day
  FILLANDKILL   // fill as much as possible immediately, cancel the rest
};

#endif