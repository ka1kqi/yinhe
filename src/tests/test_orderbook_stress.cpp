#include <cassert>
#include <iostream>

#include "order.hpp"
#include "orderbook.hpp"

class OrderbookTest {
public:
  static void insert_order(Orderbook &ob, Side side, OrderID id, Price price,
                           Quantity qty,
                           orderType type = orderType::GOODTOCANCEL) {
    auto &level = (side == Side::BUY) ? ob.bids_[price] : ob.asks_[price];
    level.push_back(Order(side, id, price, qty, type));
    auto it = std::prev(level.end());
    ob.orders_.insert({id, orderEntry{it}});
  }

  static Trades call_match(Orderbook &ob) { return ob.match(); }

  static bool call_can_match(Orderbook &ob, Side side, Price price) {
    return ob.can_match(side, price);
  }

  static bool call_can_fully_fill(Orderbook &ob, Side side, Price price,
                                  Quantity qty) {
    return ob.can_fully_fill(side, price, qty);
  }

  /* ==================== 1. Scale Tests ==================== */

  static void test_many_orders_single_level() {
    Orderbook ob;
    for (OrderID i = 1; i <= 1000; ++i)
      insert_order(ob, Side::BUY, i, 100, 10);
    for (OrderID i = 1001; i <= 2000; ++i)
      insert_order(ob, Side::SELL, i, 100, 10);
    Trades trades = call_match(ob);
    assert(trades.size() == 1000);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_many_orders_single_level" << std::endl;
  }

  static void test_many_price_levels() {
    Orderbook ob;
    /*500 bid levels: price 1..500, 1 order each qty=1*/
    for (Price p = 1; p <= 500; ++p)
      insert_order(ob, Side::BUY, p, p, 1);
    /*500 ask levels: price 501..1000, 1 order each qty=1*/
    for (Price p = 501; p <= 1000; ++p)
      insert_order(ob, Side::SELL, p + 500, p, 1);
    assert(ob.get_size() == 1000);

    /*aggressive bid that sweeps all 500 ask levels*/
    insert_order(ob, Side::BUY, 2000, 1000, 500);
    Trades trades = call_match(ob);
    assert(trades.size() == 500);
    /*trades settle at ask price; first trade at 501, last at 1000*/
    assert(trades[0].get_ask_info().price_ == 501);
    assert(trades[499].get_ask_info().price_ == 1000);
    /*500 original bids remain (no crossing), aggressive bid fully filled*/
    assert(ob.get_size() == 500);
    std::cout << "PASS: test_many_price_levels" << std::endl;
  }

  static void test_many_cancels() {
    Orderbook ob;
    for (OrderID i = 1; i <= 1000; ++i)
      insert_order(ob, Side::BUY, i, 100 + (i % 10), 10);
    assert(ob.get_size() == 1000);
    for (OrderID i = 1; i <= 1000; ++i)
      assert(ob.cancel_order(i) == 0);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_many_cancels" << std::endl;
  }

  /* ==================== 2. FIFO / Price Priority ==================== */

  static void test_fifo_within_level() {
    Orderbook ob;
    /*5 bids at same price, IDs 10..14*/
    for (OrderID i = 10; i <= 14; ++i)
      insert_order(ob, Side::BUY, i, 100, 10);
    /*single ask that matches all 5 bids*/
    insert_order(ob, Side::SELL, 20, 100, 50);
    Trades trades = call_match(ob);
    assert(trades.size() == 5);
    /*verify FIFO: earliest bid fills first*/
    for (int i = 0; i < 5; ++i)
      assert(trades[i].get_bid_info().orderID_ == (OrderID)(10 + i));
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_fifo_within_level" << std::endl;
  }

  static void test_price_priority_bids() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 10);
    insert_order(ob, Side::BUY, 2, 102, 10);
    insert_order(ob, Side::BUY, 3, 101, 10);
    /*ask at 99 crosses all bids*/
    insert_order(ob, Side::SELL, 4, 99, 30);
    Trades trades = call_match(ob);
    assert(trades.size() == 3);
    /*best bid (102) fills first, then 101, then 100*/
    assert(trades[0].get_bid_info().orderID_ == 2);
    assert(trades[1].get_bid_info().orderID_ == 3);
    assert(trades[2].get_bid_info().orderID_ == 1);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_price_priority_bids" << std::endl;
  }

  static void test_price_priority_asks() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 102, 10);
    insert_order(ob, Side::SELL, 2, 100, 10);
    insert_order(ob, Side::SELL, 3, 101, 10);
    /*bid at 103 crosses all asks*/
    insert_order(ob, Side::BUY, 4, 103, 30);
    Trades trades = call_match(ob);
    assert(trades.size() == 3);
    /*best ask (100) fills first, then 101, then 102*/
    assert(trades[0].get_ask_info().orderID_ == 2);
    assert(trades[1].get_ask_info().orderID_ == 3);
    assert(trades[2].get_ask_info().orderID_ == 1);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_price_priority_asks" << std::endl;
  }

  /* ==================== 3. Interleaved Operations ==================== */

  static void test_add_cancel_add_match() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 10);
    assert(ob.cancel_order(1) == 0);
    insert_order(ob, Side::BUY, 2, 105, 10);
    insert_order(ob, Side::SELL, 3, 105, 10);
    Trades trades = call_match(ob);
    assert(trades.size() == 1);
    assert(trades[0].get_bid_info().orderID_ == 2);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_add_cancel_add_match" << std::endl;
  }

  static void test_cancel_then_reinsert_same_level() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 10); /*first*/
    insert_order(ob, Side::BUY, 2, 100, 10); /*middle - will cancel*/
    insert_order(ob, Side::BUY, 3, 100, 10); /*third*/
    assert(ob.cancel_order(2) == 0);
    insert_order(ob, Side::BUY, 4, 100, 10); /*new - appended at end*/
    /*match all 3 remaining bids*/
    insert_order(ob, Side::SELL, 5, 100, 30);
    Trades trades = call_match(ob);
    assert(trades.size() == 3);
    /*FIFO order: first(1), third(3), new(4)*/
    assert(trades[0].get_bid_info().orderID_ == 1);
    assert(trades[1].get_bid_info().orderID_ == 3);
    assert(trades[2].get_bid_info().orderID_ == 4);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_cancel_then_reinsert_same_level" << std::endl;
  }

  static void test_partial_fill_then_cancel_remainder() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 50);
    insert_order(ob, Side::SELL, 2, 100, 20);
    Trades trades = call_match(ob);
    assert(trades.size() == 1);
    assert(trades[0].get_bid_info().quantity_ == 20);
    /*bid 1 should remain with 30 left*/
    assert(ob.get_size() == 1);
    assert(ob.cancel_order(1) == 0);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_partial_fill_then_cancel_remainder" << std::endl;
  }

  /* ==================== 4. FOK Edge Cases ==================== */

  static void test_fok_exact_fill_across_many_levels() {
    Orderbook ob;
    /*10 asks of 10 qty each at prices 100..109*/
    for (int i = 0; i < 10; ++i)
      insert_order(ob, Side::SELL, i + 1, 100 + i, 10);
    /*FOK buy for exactly 100 qty at price 109 (covers all levels)*/
    insert_order(ob, Side::BUY, 100, 109, 100, orderType::FILLORKILL);
    Trades trades = call_match(ob);
    assert(trades.size() == 10);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_fok_exact_fill_across_many_levels" << std::endl;
  }

  static void test_fok_one_short() {
    Orderbook ob;
    /*100 qty available across 10 asks*/
    for (int i = 0; i < 10; ++i)
      insert_order(ob, Side::SELL, i + 1, 100 + i, 10);
    /*FOK buy for 101 qty - one short*/
    insert_order(ob, Side::BUY, 100, 109, 101, orderType::FILLORKILL);
    Trades trades = call_match(ob);
    assert(trades.empty());
    /*FOK bid removed, all 10 asks remain*/
    assert(ob.get_size() == 10);
    std::cout << "PASS: test_fok_one_short" << std::endl;
  }

  static void test_fok_via_add_order_rejected() {
    Orderbook ob;
    /*no liquidity on sell side*/
    Trades trades =
        ob.add_order(Side::BUY, 100, 50, orderType::FILLORKILL);
    assert(trades.empty());
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_fok_via_add_order_rejected" << std::endl;
  }

  static void test_fok_via_add_order_accepted() {
    Orderbook ob;
    /*provide liquidity*/
    insert_order(ob, Side::SELL, 1, 100, 50);
    Trades trades =
        ob.add_order(Side::BUY, 100, 50, orderType::FILLORKILL);
    assert(trades.size() == 1);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_fok_via_add_order_accepted" << std::endl;
  }

  /* ==================== 5. Sweep / Deep Book ==================== */

  static void test_aggressive_bid_sweeps_all_asks() {
    Orderbook ob;
    /*10 ask levels at prices 100..109, qty 10 each*/
    for (int i = 0; i < 10; ++i)
      insert_order(ob, Side::SELL, i + 1, 100 + i, 10);
    /*aggressive bid sweeps all*/
    insert_order(ob, Side::BUY, 50, 109, 100);
    Trades trades = call_match(ob);
    assert(trades.size() == 10);
    /*prices ascending (best ask first)*/
    for (int i = 0; i < 10; ++i)
      assert(trades[i].get_ask_info().price_ == (Price)(100 + i));
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_aggressive_bid_sweeps_all_asks" << std::endl;
  }

  static void test_partial_sweep_leaves_residual() {
    Orderbook ob;
    /*3 ask levels: 100(10), 101(10), 102(10)*/
    insert_order(ob, Side::SELL, 1, 100, 10);
    insert_order(ob, Side::SELL, 2, 101, 10);
    insert_order(ob, Side::SELL, 3, 102, 10);
    /*bid sweeps 25 qty - fully fills 100 and 101, partially fills 102*/
    insert_order(ob, Side::BUY, 10, 102, 25);
    Trades trades = call_match(ob);
    assert(trades.size() == 3);
    assert(trades[0].get_ask_info().quantity_ == 10);
    assert(trades[1].get_ask_info().quantity_ == 10);
    assert(trades[2].get_ask_info().quantity_ == 5);
    /*ask 3 remains with 5 qty left*/
    assert(ob.get_size() == 1);
    auto infos = ob.get_levelInfos();
    auto asks = infos.get_asks();
    assert(asks.size() == 1);
    assert(asks[0].price == 102);
    assert(asks[0].quantity == 5);
    std::cout << "PASS: test_partial_sweep_leaves_residual" << std::endl;
  }

  /* ==================== 6. Edge Cases ==================== */

  static void test_quantity_one_orders() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 1);
    insert_order(ob, Side::SELL, 2, 100, 1);
    Trades trades = call_match(ob);
    assert(trades.size() == 1);
    assert(trades[0].get_bid_info().quantity_ == 1);
    assert(ob.get_size() == 0);
    std::cout << "PASS: test_quantity_one_orders" << std::endl;
  }

  static void test_cancel_already_filled_order() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 10);
    insert_order(ob, Side::SELL, 2, 100, 10);
    Trades trades = call_match(ob);
    assert(trades.size() == 1);
    assert(ob.get_size() == 0);
    /*both orders are gone after match, cancel should return -1*/
    assert(ob.cancel_order(1) == -1);
    std::cout << "PASS: test_cancel_already_filled_order" << std::endl;
  }

  static void test_double_cancel() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 10);
    assert(ob.cancel_order(1) == 0);
    assert(ob.cancel_order(1) == -1);
    std::cout << "PASS: test_double_cancel" << std::endl;
  }

  static void test_levelinfos_after_partial_match() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 50);
    insert_order(ob, Side::SELL, 2, 100, 20);
    call_match(ob);
    /*bid 1 should have 30 remaining*/
    auto infos = ob.get_levelInfos();
    auto bids = infos.get_bids();
    assert(bids.size() == 1);
    assert(bids[0].price == 100);
    assert(bids[0].quantity == 30);
    assert(infos.get_asks().empty());
    std::cout << "PASS: test_levelinfos_after_partial_match" << std::endl;
  }
};

int main() {
  std::cout << "\n=== Scale Tests ===" << std::endl;
  OrderbookTest::test_many_orders_single_level();
  OrderbookTest::test_many_price_levels();
  OrderbookTest::test_many_cancels();

  std::cout << "\n=== FIFO / Price Priority ===" << std::endl;
  OrderbookTest::test_fifo_within_level();
  OrderbookTest::test_price_priority_bids();
  OrderbookTest::test_price_priority_asks();

  std::cout << "\n=== Interleaved Operations ===" << std::endl;
  OrderbookTest::test_add_cancel_add_match();
  OrderbookTest::test_cancel_then_reinsert_same_level();
  OrderbookTest::test_partial_fill_then_cancel_remainder();

  std::cout << "\n=== FOK Edge Cases ===" << std::endl;
  OrderbookTest::test_fok_exact_fill_across_many_levels();
  OrderbookTest::test_fok_one_short();
  OrderbookTest::test_fok_via_add_order_rejected();
  OrderbookTest::test_fok_via_add_order_accepted();

  std::cout << "\n=== Sweep / Deep Book ===" << std::endl;
  OrderbookTest::test_aggressive_bid_sweeps_all_asks();
  OrderbookTest::test_partial_sweep_leaves_residual();

  std::cout << "\n=== Edge Cases ===" << std::endl;
  OrderbookTest::test_quantity_one_orders();
  OrderbookTest::test_cancel_already_filled_order();
  OrderbookTest::test_double_cancel();
  OrderbookTest::test_levelinfos_after_partial_match();

  std::cout << "\n*** All stress tests passed (20 tests). ***" << std::endl;
  return 0;
}
