// SPDX-License-Identifier: MIT
// SPDX FileCopyRightText : Ashish Kumar <15678ashishk@gmail.com>

/*
* Single Header Library Pattern
* All files in this project use a simple technique
* to define the interface and implementation separately.
*
* The interface is defined in this header file, and the implementation
* is provided by `#define FILE_NAME_H_IMPL`
*/

#ifndef ORDERBOOK_API_H
#define ORDERBOOK_API_H

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

OrderBook* orderbook_create(void);
void orderbook_destroy(OrderBook* book);
bool orderbook_add_order(OrderBook* book, Order* order);
bool orderbook_cancel_order(OrderBook* book, uint64_t order_id);
double orderbook_get_best_bid(OrderBook* book);
double orderbook_get_best_ask(OrderBook* book);

#ifdef ORDERBOOK_API_H_IMPL

#include "api.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Helper to create a new price level
static PriceLevel* create_price_level(double price) {
    PriceLevel* level = (PriceLevel*)malloc(sizeof(PriceLevel));
    if (!level) return NULL;
    level->price = price;
    level->total_quantity = 0;
    level->order_head = NULL;
    level->order_tail = NULL;
    level->left = NULL;
    level->right = NULL;
    return level;
}

OrderBook* orderbook_create(void) {
    OrderBook* book = (OrderBook*)malloc(sizeof(OrderBook));
    if (!book) return NULL;
    book->bids = NULL;
    book->asks = NULL;
    book->last_traded_price = 0.0;
    return book;
}

static void free_price_level(PriceLevel* level) {
    if (!level) return;
    free_price_level(level->left);
    free_price_level(level->right);

    Order* current = level->order_head;
    while (current) {
        Order* next = current->next;
        free(current);
        current = next;
    }
    free(level);
}

void orderbook_destroy(OrderBook* book) {
    if (!book) return;
    free_price_level(book->bids);
    free_price_level(book->asks);
    free(book);
}

static void insert_price_level(PriceLevel** root, PriceLevel* new_level) {
    if (*root == NULL) {
        *root = new_level;
        return;
    }

    PriceLevel* current = *root;
    while (true) {
        if (new_level->price == current->price) return;

        if (new_level->price < current->price) {
            if (current->left == NULL) {
                current->left = new_level;
                break;
            }
            current = current->left;
        } else {
            if (current->right == NULL) {
                current->right = new_level;
                break;
            }
            current = current->right;
        }
    }
}

static PriceLevel* find_price_level(PriceLevel* root, double price) {
    PriceLevel* current = root;
    while (current) {
        if (price == current->price) return current;
        if (price < current->price) current = current->left;
        else current = current->right;
    }
    return NULL;
}

static PriceLevel* get_max_level(PriceLevel* root) {
    if (!root) return NULL;
    while (root->right) root = root->right;
    return root;
}

static PriceLevel* get_min_level(PriceLevel* root) {
    if (!root) return NULL;
    while (root->left) root = root->left;
    return root;
}

static PriceLevel* remove_price_level(PriceLevel* root, double price) {
    if (!root) return NULL;

    if (price < root->price) {
        root->left = remove_price_level(root->left, price);
    } else if (price > root->price) {
        root->right = remove_price_level(root->right, price);
    } else {
        if (!root->left) {
            PriceLevel* temp = root->right;
            free(root);
            return temp;
        } else if (!root->right) {
            PriceLevel* temp = root->left;
            free(root);
            return temp;
        }

        PriceLevel* temp = get_min_level(root->right);
        root->price = temp->price;
        root->order_head = temp->order_head;
        root->order_tail = temp->order_tail;
        root->total_quantity = temp->total_quantity;

        temp->order_head = NULL;
        temp->order_tail = NULL;

        root->right = remove_price_level(root->right, temp->price);
    }
    return root;
}

static void match_order(OrderBook* book, Order* order) {
    while (order->filled_quantity < order->quantity) {
        PriceLevel* best_opposing = NULL;
        if (order->side == SIDE_BUY) {
            best_opposing = get_min_level(book->asks);
            if (!best_opposing || (order->type == TYPE_LIMIT && best_opposing->price > order->price)) {
                break;
            }
        } else {
            best_opposing = get_max_level(book->bids);
            if (!best_opposing || (order->type == TYPE_LIMIT && best_opposing->price < order->price)) {
                break;
            }
        }

        Order* maker = best_opposing->order_head;
        if (!maker) {
            if (order->side == SIDE_BUY) {
                book->asks = remove_price_level(book->asks, best_opposing->price);
            } else {
                book->bids = remove_price_level(book->bids, best_opposing->price);
            }
            continue;
        }

        double qty_needed = order->quantity - order->filled_quantity;
        double qty_available = maker->quantity - maker->filled_quantity;
        double trade_qty = (qty_needed < qty_available) ? qty_needed : qty_available;

        order->filled_quantity += trade_qty;
        maker->filled_quantity += trade_qty;
        best_opposing->total_quantity -= trade_qty;
        book->last_traded_price = maker->price;

        if (maker->filled_quantity >= maker->quantity) {
            best_opposing->order_head = maker->next;
            if (best_opposing->order_head) {
                best_opposing->order_head->prev = NULL;
            } else {
                best_opposing->order_tail = NULL;
            }
            free(maker);
        }
    }
}

bool orderbook_add_order(OrderBook* book, Order* order) {
    if (!book || !order) return false;

    order->filled_quantity = 0;
    match_order(book, order);

    if (order->filled_quantity < order->quantity) {
        if (order->type == TYPE_MARKET) {
            free(order);
            return true;
        }

        PriceLevel** root = (order->side == SIDE_BUY) ? &book->bids : &book->asks;
        PriceLevel* level = find_price_level(*root, order->price);

        if (!level) {
            level = create_price_level(order->price);
            if (!level) return false;
            insert_price_level(root, level);
        }

        order->next = NULL;
        order->prev = level->order_tail;
        if (level->order_tail) {
            level->order_tail->next = order;
        } else {
            level->order_head = order;
        }
        level->order_tail = order;
        level->total_quantity += (order->quantity - order->filled_quantity);
    } else {
        free(order);
    }

    return true;
}

static Order* find_order_in_tree(PriceLevel* root, uint64_t order_id, PriceLevel** found_level) {
    if (!root) return NULL;

    Order* current = root->order_head;
    while (current) {
        if (current->id == order_id) {
            if (found_level) *found_level = root;
            return current;
        }
        current = current->next;
    }

    Order* found = find_order_in_tree(root->left, order_id, found_level);
    if (found) return found;

    return find_order_in_tree(root->right, order_id, found_level);
}

bool orderbook_cancel_order(OrderBook* book, uint64_t order_id) {
    if (!book) return false;

    PriceLevel* level = NULL;
    Order* target = find_order_in_tree(book->bids, order_id, &level);
    if (!target) {
        target = find_order_in_tree(book->asks, order_id, &level);
    }

    if (!target || !level) return false;

    if (target->prev) target->prev->next = target->next;
    else level->order_head = target->next;

    if (target->next) target->next->prev = target->prev;
    else level->order_tail = target->prev;

    level->total_quantity -= (target->quantity - target->filled_quantity);
    free(target);

    return true;
}

double orderbook_get_best_bid(OrderBook* book) {
    if (!book || !book->bids) return 0.0;
    PriceLevel* level = get_max_level(book->bids);
    return level ? level->price : 0.0;
}

double orderbook_get_best_ask(OrderBook* book) {
    if (!book || !book->asks) return 0.0;
    PriceLevel* level = get_min_level(book->asks);
    return level ? level->price : 0.0;
}

#endif // ORDERBOOK_API_H_IMPL


#ifdef __cplusplus
}
#endif

#endif // ORDERBOOK_API_H
