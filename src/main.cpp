#include <iostream>

#include "engine/orderbook.hpp"

int main() {
  Orderbook orderbook;

  std::cout << "\n=== 1. Add asks (sell orders) ===" << std::endl;
  (void)orderbook.add_order(Side::SELL, 100, 50, orderType::LIMIT);
  (void)orderbook.add_order(Side::SELL, 101, 30, orderType::LIMIT);
  (void)orderbook.add_order(Side::SELL, 102, 20, orderType::LIMIT);
  std::cout << "Book size: " << orderbook.get_size() << std::endl;
  orderbook.print_levels();

  std::cout << "\n=== 2. Add bids (no match, below best ask) ===" << std::endl;
  (void)orderbook.add_order(Side::BUY, 98, 40, orderType::LIMIT);
  (void)orderbook.add_order(Side::BUY, 97, 25, orderType::LIMIT);
  std::cout << "Book size: " << orderbook.get_size() << std::endl;
  orderbook.print_levels();

  std::cout << "\n=== 3. Add crossing bid (matches ask @ 100) ===" << std::endl;
  Trades trades1 = orderbook.add_order(Side::BUY, 100, 30, orderType::LIMIT);
  std::cout << "Trades executed: " << trades1.size() << std::endl;
  for (auto &t : trades1) {
    auto bi = t.get_bid_info();
    auto ai = t.get_ask_info();
    std::cout << "  Trade: BidID=" << bi.orderID_ << " AskID=" << ai.orderID_
              << " Price=" << ai.price_ << " Qty=" << ai.quantity_ << std::endl;
  }
  std::cout << "Book size: " << orderbook.get_size() << std::endl;
  orderbook.print_levels();

  std::cout << "\n=== 4. Add aggressive bid (sweeps multiple ask levels) ==="
            << std::endl;
  Trades trades2 = orderbook.add_order(Side::BUY, 105, 60, orderType::LIMIT);
  std::cout << "Trades executed: " << trades2.size() << std::endl;
  for (auto &t : trades2) {
    auto bi = t.get_bid_info();
    auto ai = t.get_ask_info();
    std::cout << "  Trade: BidID=" << bi.orderID_ << " AskID=" << ai.orderID_
              << " Price=" << ai.price_ << " Qty=" << ai.quantity_ << std::endl;
  }
  std::cout << "Book size: " << orderbook.get_size() << std::endl;
  orderbook.print_levels();

  std::cout << "\n=== 5. FOK buy (rejected: needs 500, only 10 available) ==="
            << std::endl;
  std::cout << "Book before FOK:" << std::endl;
  orderbook.print_levels();
  /*add a sell that crosses existing bids so there's something to match against*/
  (void)orderbook.add_order(Side::SELL, 100, 10, orderType::LIMIT);
  std::cout << "Added ask @ 100 qty 10" << std::endl;
  Trades trades3 =
      orderbook.add_order(Side::BUY, 100, 500, orderType::FILLORKILL);
  std::cout << "FOK trades executed: " << trades3.size() << std::endl;
  std::cout << "Book size: " << orderbook.get_size() << std::endl;
  orderbook.print_levels();

  std::cout << "\n=== 6. FOK buy (accepted: exact fill) ===" << std::endl;
  (void)orderbook.add_order(Side::SELL, 100, 25, orderType::LIMIT);
  std::cout << "Added ask @ 100 qty 25" << std::endl;
  Trades trades4 =
      orderbook.add_order(Side::BUY, 100, 25, orderType::FILLORKILL);
  std::cout << "FOK trades executed: " << trades4.size() << std::endl;
  for (auto &t : trades4) {
    auto bi = t.get_bid_info();
    auto ai = t.get_ask_info();
    std::cout << "  Trade: BidID=" << bi.orderID_ << " AskID=" << ai.orderID_
              << " Price=" << ai.price_ << " Qty=" << ai.quantity_ << std::endl;
  }
  std::cout << "Book size: " << orderbook.get_size() << std::endl;
  orderbook.print_levels();

  std::cout << "\n=== 7. Cancel an order ===" << std::endl;
  (void)orderbook.add_order(Side::SELL, 110, 100, orderType::LIMIT);
  std::cout << "Book size: " << orderbook.get_size() << std::endl;
  orderbook.print_levels();

  std::cout << "\n=== Final book state ===" << std::endl;
  std::cout << "Book size: " << orderbook.get_size() << std::endl;
  orderbook.print_levels();

  std::cout << "\n=== Done. Check logs/ directory for trade log. ===" << std::endl;
  return 0;
}
