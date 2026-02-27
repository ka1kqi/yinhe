#include <cassert>
#include <iostream>

#include "order.hpp"
#include "orderbook.hpp"

class OrderbookTest {
public:
  /*helper: insert an order into the book without triggering match*/
  static void insert_order(Orderbook &ob, Side side, OrderID id, Price price,
                           Quantity qty,
                           orderType type = orderType::GOODTOCANCEL) {
    auto &level = (side == Side::BUY) ? ob.bids_[price] : ob.asks_[price];
    level.push_back(Order(side, id, price, qty, type));
    auto it = std::prev(level.end());
    ob.orders_.insert({id, orderEntry{it}});
  }

  /*helper: call match() directly*/
  static Trades call_match(Orderbook &ob) { return ob.match(); }

  /*helper: call can_match() directly*/
  static bool call_can_match(Orderbook &ob, Side side, Price price) {
    return ob.can_match(side, price);
  }

  /*helper: call can_fully_fill() directly*/
  static bool call_can_fully_fill(Orderbook &ob, Side side, Price price,
                                  Quantity qty) {
    return ob.can_fully_fill(side, price, qty);
  }

  /* ==================== match() tests ==================== */

  static void test_match_empty_book() {
    Orderbook ob;
    Trades trades = call_match(ob);
    assert(trades.empty());
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_match_empty_book" << std::endl;
  }

  static void test_match_no_crossing() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 90, 10);
    insert_order(ob, Side::SELL, 2, 100, 10);
    Trades trades = call_match(ob);
    assert(trades.empty());
    assert(ob.get_size() == 2);
    std::cout << "PASS: test_match_no_crossing" << std::endl;
  }

  static void test_match_exact() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 50);
    insert_order(ob, Side::SELL, 2, 100, 50);
    Trades trades = call_match(ob);
    assert(trades.size() == 1);
    assert(trades[0].get_bid_info().quantity_ == 50);
    assert(trades[0].get_ask_info().price_ == 100);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_match_exact" << std::endl;
  }

  static void test_match_bid_larger() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 80);
    insert_order(ob, Side::SELL, 2, 100, 30);
    Trades trades = call_match(ob);
    assert(trades.size() == 1);
    assert(trades[0].get_bid_info().quantity_ == 30);
    /*bid should remain with 50 left*/
    assert(ob.get_size() == 1);
    std::cout << "PASS: test_match_bid_larger" << std::endl;
  }

  static void test_match_ask_larger() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 20);
    insert_order(ob, Side::SELL, 2, 100, 50);
    Trades trades = call_match(ob);
    assert(trades.size() == 1);
    assert(trades[0].get_bid_info().quantity_ == 20);
    assert(ob.get_size() == 1);
    std::cout << "PASS: test_match_ask_larger" << std::endl;
  }

  static void test_match_multiple_trades_same_level() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 30);
    insert_order(ob, Side::BUY, 2, 100, 30);
    insert_order(ob, Side::SELL, 3, 100, 50);
    Trades trades = call_match(ob);
    /*first trade: bid 1 (30) vs ask 3 (50) -> trade 30, ask has 20 left*/
    /*second trade: bid 2 (30) vs ask 3 (20) -> trade 20, bid has 10 left*/
    assert(trades.size() == 2);
    assert(trades[0].get_bid_info().quantity_ == 30);
    assert(trades[1].get_bid_info().quantity_ == 20);
    assert(ob.get_size() == 1); /*bid 2 remains with 10*/
    std::cout << "PASS: test_match_multiple_trades_same_level" << std::endl;
  }

  static void test_match_across_price_levels() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 105, 40);
    insert_order(ob, Side::SELL, 2, 100, 20);
    insert_order(ob, Side::SELL, 3, 103, 20);
    Trades trades = call_match(ob);
    /*bid @ 105 crosses ask @ 100 and ask @ 103*/
    /*trade 1: bid 1 (40) vs ask 2 @ 100 (20) -> trade 20 @ 100*/
    /*trade 2: bid 1 (20 remaining) vs ask 3 @ 103 (20) -> trade 20 @ 103*/
    assert(trades.size() == 2);
    assert(trades[0].get_bid_info().quantity_ == 20);
    assert(trades[0].get_ask_info().price_ == 100);
    assert(trades[1].get_bid_info().quantity_ == 20);
    assert(trades[1].get_ask_info().price_ == 103);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_match_across_price_levels" << std::endl;
  }

  static void test_match_settles_at_ask_price() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 110, 10);
    insert_order(ob, Side::SELL, 2, 100, 10);
    Trades trades = call_match(ob);
    assert(trades.size() == 1);
    /*trade settles at ask price*/
    assert(trades[0].get_bid_info().price_ == 100);
    assert(trades[0].get_ask_info().price_ == 100);
    std::cout << "PASS: test_match_settles_at_ask_price" << std::endl;
  }

  /* ==================== FOK in match() tests ==================== */

  static void test_match_fok_bid_rejected() {
    Orderbook ob;
    /*FOK buy for 100 qty, but only 30 available on sell side*/
    insert_order(ob, Side::BUY, 1, 100, 100, orderType::FILLORKILL);
    insert_order(ob, Side::SELL, 2, 100, 30);
    Trades trades = call_match(ob);
    assert(trades.empty());
    /*FOK bid should be removed, ask remains*/
    assert(ob.get_size() == 1);
    std::cout << "PASS: test_match_fok_bid_rejected" << std::endl;
  }

  static void test_match_fok_ask_rejected() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 20);
    insert_order(ob, Side::SELL, 2, 100, 50, orderType::FILLORKILL);
    Trades trades = call_match(ob);
    assert(trades.empty());
    assert(ob.get_size() == 1);
    std::cout << "PASS: test_match_fok_ask_rejected" << std::endl;
  }

  static void test_match_fok_bid_accepted() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 30, orderType::FILLORKILL);
    insert_order(ob, Side::SELL, 2, 100, 30);
    Trades trades = call_match(ob);
    assert(trades.size() == 1);
    assert(trades[0].get_bid_info().quantity_ == 30);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_match_fok_bid_accepted" << std::endl;
  }

  static void test_match_fok_bid_rejected_then_normal_matches() {
    Orderbook ob;
    /*FOK bid can't be filled, but a normal bid behind it can*/
    insert_order(ob, Side::BUY, 1, 100, 100, orderType::FILLORKILL);
    insert_order(ob, Side::BUY, 2, 100, 20);
    insert_order(ob, Side::SELL, 3, 100, 20);
    Trades trades = call_match(ob);
    /*FOK rejected, then bid 2 matches ask 3*/
    assert(trades.size() == 1);
    assert(trades[0].get_bid_info().orderID_ == 2);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_match_fok_bid_rejected_then_normal_matches"
              << std::endl;
  }

  /* ==================== can_match() tests ==================== */

  static void test_can_match_empty_book() {
    Orderbook ob;
    assert(!call_can_match(ob, Side::BUY, 100));
    assert(!call_can_match(ob, Side::SELL, 100));
    std::cout << "PASS: test_can_match_empty_book" << std::endl;
  }

  static void test_can_match_buy_yes() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 100, 10);
    assert(call_can_match(ob, Side::BUY, 100));
    assert(call_can_match(ob, Side::BUY, 110));
    std::cout << "PASS: test_can_match_buy_yes" << std::endl;
  }

  static void test_can_match_buy_no() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 100, 10);
    assert(!call_can_match(ob, Side::BUY, 90));
    std::cout << "PASS: test_can_match_buy_no" << std::endl;
  }

  static void test_can_match_sell_yes() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 10);
    assert(call_can_match(ob, Side::SELL, 100));
    assert(call_can_match(ob, Side::SELL, 90));
    std::cout << "PASS: test_can_match_sell_yes" << std::endl;
  }

  static void test_can_match_sell_no() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 10);
    assert(!call_can_match(ob, Side::SELL, 110));
    std::cout << "PASS: test_can_match_sell_no" << std::endl;
  }

  /* ==================== cancel_order() tests ==================== */

  static void test_cancel_nonexistent() {
    Orderbook ob;
    assert(ob.cancel_order(999) == -1);
    std::cout << "PASS: test_cancel_nonexistent" << std::endl;
  }

  static void test_cancel_bid() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 50);
    assert(ob.get_size() == 1);
    assert(ob.cancel_order(1) == 0);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_cancel_bid" << std::endl;
  }

  static void test_cancel_ask() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 100, 50);
    assert(ob.get_size() == 1);
    assert(ob.cancel_order(1) == 0);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_cancel_ask" << std::endl;
  }

  static void test_cancel_one_of_many_at_level() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 10);
    insert_order(ob, Side::BUY, 2, 100, 20);
    insert_order(ob, Side::BUY, 3, 100, 30);
    assert(ob.get_size() == 3);
    assert(ob.cancel_order(2) == 0);
    assert(ob.get_size() == 2);
    /*level should still exist with orders 1 and 3*/
    auto infos = ob.get_levelInfos();
    auto bids = infos.get_bids();
    assert(bids.size() == 1);
    assert(bids[0].quantity == 40); /*10 + 30*/
    std::cout << "PASS: test_cancel_one_of_many_at_level" << std::endl;
  }

  static void test_cancel_removes_empty_level() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 100, 10);
    insert_order(ob, Side::SELL, 2, 200, 20);
    assert(ob.cancel_order(1) == 0);
    auto infos = ob.get_levelInfos();
    auto asks = infos.get_asks();
    assert(asks.size() == 1);
    assert(asks[0].price == 200);
    std::cout << "PASS: test_cancel_removes_empty_level" << std::endl;
  }

  /* ==================== add_order() + matching tests ==================== */

  static void test_add_order_no_match() {
    Orderbook ob;
    Trades trades = ob.add_order(Side::BUY, 100, 10, orderType::GOODTOCANCEL);
    assert(trades.empty());
    assert(ob.get_size() == 1);
    std::cout << "PASS: test_add_order_no_match" << std::endl;
  }

  static void test_add_order_immediate_match() {
    Orderbook ob;
    (void)ob.add_order(Side::SELL, 100, 20, orderType::GOODTOCANCEL);
    Trades trades = ob.add_order(Side::BUY, 100, 20, orderType::GOODTOCANCEL);
    assert(trades.size() == 1);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_add_order_immediate_match" << std::endl;
  }

  /* ==================== get_size() tests ==================== */

  static void test_get_size_empty() {
    Orderbook ob;
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_get_size_empty" << std::endl;
  }

  static void test_get_size_after_inserts() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 10);
    insert_order(ob, Side::SELL, 2, 200, 20);
    assert(ob.get_size() == 2);
    std::cout << "PASS: test_get_size_after_inserts" << std::endl;
  }

  /* ==================== get_levelInfos() tests ==================== */

  static void test_level_infos_empty() {
    Orderbook ob;
    auto infos = ob.get_levelInfos();
    assert(infos.get_bids().empty());
    assert(infos.get_asks().empty());
    std::cout << "PASS: test_level_infos_empty" << std::endl;
  }

  static void test_level_infos_aggregates_quantity() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 10);
    insert_order(ob, Side::BUY, 2, 100, 25);
    insert_order(ob, Side::SELL, 3, 200, 15);
    auto infos = ob.get_levelInfos();
    auto bids = infos.get_bids();
    auto asks = infos.get_asks();
    assert(bids.size() == 1);
    assert(bids[0].price == 100);
    assert(bids[0].quantity == 35);
    assert(asks.size() == 1);
    assert(asks[0].price == 200);
    assert(asks[0].quantity == 15);
    std::cout << "PASS: test_level_infos_aggregates_quantity" << std::endl;
  }

  static void test_level_infos_multiple_levels() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 10);
    insert_order(ob, Side::BUY, 2, 105, 20);
    insert_order(ob, Side::SELL, 3, 200, 5);
    insert_order(ob, Side::SELL, 4, 210, 15);
    auto infos = ob.get_levelInfos();
    auto bids = infos.get_bids();
    auto asks = infos.get_asks();
    /*bids sorted descending*/
    assert(bids.size() == 2);
    assert(bids[0].price == 105);
    assert(bids[1].price == 100);
    /*asks sorted ascending*/
    assert(asks.size() == 2);
    assert(asks[0].price == 200);
    assert(asks[1].price == 210);
    std::cout << "PASS: test_level_infos_multiple_levels" << std::endl;
  }

  /* ==================== can_fully_fill() tests ==================== */

  static void test_can_fully_fill_empty() {
    Orderbook ob;
    assert(!call_can_fully_fill(ob, Side::BUY, 100, 10));
    assert(!call_can_fully_fill(ob, Side::SELL, 100, 10));
    std::cout << "PASS: test_can_fully_fill_empty" << std::endl;
  }

  static void test_can_fully_fill_exact() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 100, 50);
    assert(call_can_fully_fill(ob, Side::BUY, 100, 50));
    std::cout << "PASS: test_can_fully_fill_exact" << std::endl;
  }

  static void test_can_fully_fill_insufficient() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 100, 20);
    assert(!call_can_fully_fill(ob, Side::BUY, 100, 50));
    std::cout << "PASS: test_can_fully_fill_insufficient" << std::endl;
  }

  static void test_can_fully_fill_across_levels() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 98, 30);
    insert_order(ob, Side::SELL, 2, 100, 30);
    assert(call_can_fully_fill(ob, Side::BUY, 100, 50));
    std::cout << "PASS: test_can_fully_fill_across_levels" << std::endl;
  }
};

int main() {
  std::cout << "\n=== match() ===" << std::endl;
  OrderbookTest::test_match_empty_book();
  OrderbookTest::test_match_no_crossing();
  OrderbookTest::test_match_exact();
  OrderbookTest::test_match_bid_larger();
  OrderbookTest::test_match_ask_larger();
  OrderbookTest::test_match_multiple_trades_same_level();
  OrderbookTest::test_match_across_price_levels();
  OrderbookTest::test_match_settles_at_ask_price();

  std::cout << "\n=== FOK in match() ===" << std::endl;
  OrderbookTest::test_match_fok_bid_rejected();
  OrderbookTest::test_match_fok_ask_rejected();
  OrderbookTest::test_match_fok_bid_accepted();
  OrderbookTest::test_match_fok_bid_rejected_then_normal_matches();

  std::cout << "\n=== can_match() ===" << std::endl;
  OrderbookTest::test_can_match_empty_book();
  OrderbookTest::test_can_match_buy_yes();
  OrderbookTest::test_can_match_buy_no();
  OrderbookTest::test_can_match_sell_yes();
  OrderbookTest::test_can_match_sell_no();

  std::cout << "\n=== cancel_order() ===" << std::endl;
  OrderbookTest::test_cancel_nonexistent();
  OrderbookTest::test_cancel_bid();
  OrderbookTest::test_cancel_ask();
  OrderbookTest::test_cancel_one_of_many_at_level();
  OrderbookTest::test_cancel_removes_empty_level();

  std::cout << "\n=== add_order() ===" << std::endl;
  OrderbookTest::test_add_order_no_match();
  OrderbookTest::test_add_order_immediate_match();

  std::cout << "\n=== get_size() ===" << std::endl;
  OrderbookTest::test_get_size_empty();
  OrderbookTest::test_get_size_after_inserts();

  std::cout << "\n=== get_levelInfos() ===" << std::endl;
  OrderbookTest::test_level_infos_empty();
  OrderbookTest::test_level_infos_aggregates_quantity();
  OrderbookTest::test_level_infos_multiple_levels();

  std::cout << "\n=== can_fully_fill() ===" << std::endl;
  OrderbookTest::test_can_fully_fill_empty();
  OrderbookTest::test_can_fully_fill_exact();
  OrderbookTest::test_can_fully_fill_insufficient();
  OrderbookTest::test_can_fully_fill_across_levels();

  std::cout << "\n*** All orderbook tests passed. ***" << std::endl;
  return 0;
}
