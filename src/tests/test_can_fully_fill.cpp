#include <cassert>
#include <iostream>

#include "order.hpp"
#include "orderbook.hpp"

/*
 * Friend class that accesses Orderbook private members for testing.
 * Uses direct insertion into bids_/asks_/orders_ to set up book state
 * without triggering the matching engine.
 */
class OrderbookTest {
public:
  /*helper: insert an order into the book without matching*/
  static void insert_order(Orderbook &ob, Side side, OrderID id, Price price, Quantity qty) {
    auto order = std::make_shared<Order>(side, id, price, qty);
    auto &level = (side == Side::BUY) ? ob.bids_[price] : ob.asks_[price];
    level.push_back(order);
    auto it = std::prev(level.end());
    ob.orders_.insert({id, orderEntry{order, it}});
  }

  /*--- tests ---*/

  static void test_empty_book() {
    Orderbook ob;
    /*no orders on either side, should not be fillable*/
    assert(!ob.can_fully_fill(Side::BUY, 100, 10));
    assert(!ob.can_fully_fill(Side::SELL, 100, 10));
    std::cout << "PASS: test_empty_book" << std::endl;
  }

  static void test_exact_fill_single_order() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 100, 50);
    /*buy 50 @ 100, there is a sell for exactly 50 @ 100*/
    assert(ob.can_fully_fill(Side::BUY, 100, 50));
    std::cout << "PASS: test_exact_fill_single_order" << std::endl;
  }

  static void test_partial_fill_insufficient_quantity() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 100, 20);
    /*buy 50 @ 100, only 20 available*/
    assert(!ob.can_fully_fill(Side::BUY, 100, 50));
    std::cout << "PASS: test_partial_fill_insufficient_quantity" << std::endl;
  }

  static void test_fill_across_multiple_orders_same_level() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 100, 20);
    insert_order(ob, Side::SELL, 2, 100, 20);
    insert_order(ob, Side::SELL, 3, 100, 15);
    /*buy 50 @ 100, 20+20+15 = 55 available at price 100*/
    assert(ob.can_fully_fill(Side::BUY, 100, 50));
    std::cout << "PASS: test_fill_across_multiple_orders_same_level" << std::endl;
  }

  static void test_fill_across_multiple_price_levels() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 98, 30);
    insert_order(ob, Side::SELL, 2, 100, 30);
    /*buy 50 @ 100: 30 @ 98 + 30 @ 100 = 60 available at or below 100*/
    assert(ob.can_fully_fill(Side::BUY, 100, 50));
    std::cout << "PASS: test_fill_across_multiple_price_levels" << std::endl;
  }

  static void test_price_too_low_to_match() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 110, 50);
    /*buy @ 100, but best ask is 110 — can't match*/
    assert(!ob.can_fully_fill(Side::BUY, 100, 50));
    std::cout << "PASS: test_price_too_low_to_match" << std::endl;
  }

  static void test_partial_levels_in_range() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 98, 10);
    insert_order(ob, Side::SELL, 2, 100, 10);
    insert_order(ob, Side::SELL, 3, 105, 100); /*this level is above our price*/
    /*buy 25 @ 100: only 10+10 = 20 available at or below 100*/
    assert(!ob.can_fully_fill(Side::BUY, 100, 25));
    std::cout << "PASS: test_partial_levels_in_range" << std::endl;
  }

  static void test_sell_side_exact_fill() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 100, 40);
    /*sell 40 @ 100, there is a bid for 40 @ 100*/
    assert(ob.can_fully_fill(Side::SELL, 100, 40));
    std::cout << "PASS: test_sell_side_exact_fill" << std::endl;
  }

  static void test_sell_side_across_levels() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 105, 20);
    insert_order(ob, Side::BUY, 2, 100, 20);
    /*sell 35 @ 100: 20 @ 105 + 20 @ 100 = 40 available at or above 100*/
    assert(ob.can_fully_fill(Side::SELL, 100, 35));
    std::cout << "PASS: test_sell_side_across_levels" << std::endl;
  }

  static void test_sell_side_price_too_high() {
    Orderbook ob;
    insert_order(ob, Side::BUY, 1, 90, 50);
    /*sell @ 100, but best bid is 90 — can't match*/
    assert(!ob.can_fully_fill(Side::SELL, 100, 50));
    std::cout << "PASS: test_sell_side_price_too_high" << std::endl;
  }

  static void test_quantity_one() {
    Orderbook ob;
    insert_order(ob, Side::SELL, 1, 100, 1);
    assert(ob.can_fully_fill(Side::BUY, 100, 1));
    std::cout << "PASS: test_quantity_one" << std::endl;
  }
};

int main() {
  OrderbookTest::test_empty_book();
  OrderbookTest::test_exact_fill_single_order();
  OrderbookTest::test_partial_fill_insufficient_quantity();
  OrderbookTest::test_fill_across_multiple_orders_same_level();
  OrderbookTest::test_fill_across_multiple_price_levels();
  OrderbookTest::test_price_too_low_to_match();
  OrderbookTest::test_partial_levels_in_range();
  OrderbookTest::test_sell_side_exact_fill();
  OrderbookTest::test_sell_side_across_levels();
  OrderbookTest::test_sell_side_price_too_high();
  OrderbookTest::test_quantity_one();

  std::cout << "\nAll can_fully_fill tests passed." << std::endl;
  return 0;
}
