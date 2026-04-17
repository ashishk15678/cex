#ifndef CEX_ORDERBOOK_API_H
#define CEX_ORDERBOOK_API_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SIDE_BUY = 0,
    SIDE_SELL = 1
} OrderSide;

typedef enum {
    TYPE_LIMIT = 0,
    TYPE_MARKET = 1
} OrderType;

typedef struct Order {
    uint64_t id;
    uint64_t user_id;
    OrderSide side;
    OrderType type;
    double price;
    double quantity;
    double filled_quantity;
    uint64_t timestamp;
    struct Order* next;
    struct Order* prev;
} Order;

typedef struct Trade {
    uint64_t maker_order_id;
    uint64_t taker_order_id;
    double price;
    double quantity;
    uint64_t timestamp;
} Trade;

typedef struct PriceLevel {
    double price;
    double total_quantity;
    Order* order_head;
    Order* order_tail;
    struct PriceLevel* left;
    struct PriceLevel* right;
} PriceLevel;

typedef struct {
    PriceLevel* bids; // Bids tree (buy orders, highest price first)
    PriceLevel* asks; // Asks tree (sell orders, lowest price first)
    double last_traded_price;
} OrderBook;

// Initialize an empty order book
OrderBook* orderbook_create(void);

// Destroy an order book and free all associated memory
void orderbook_destroy(OrderBook* book);

// Add an order to the order book.
// If the order crosses the spread, it may result in immediate trades (matching).
bool orderbook_add_order(OrderBook* book, Order* order);

// Cancel an existing order by ID
bool orderbook_cancel_order(OrderBook* book, uint64_t order_id);

// Get the best bid price
double orderbook_get_best_bid(OrderBook* book);

// Get the best ask price
double orderbook_get_best_ask(OrderBook* book);

#ifdef __cplusplus
}
#endif

#endif // CEX_ORDERBOOK_API_H