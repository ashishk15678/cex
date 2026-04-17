import asyncio
import json
import random
import time

try:
    import websockets
except ImportError:
    print("Please install websockets: pip install websockets")
    exit(1)

# Market Maker configuration
WS_URL = "ws://localhost:8080"
START_PRICE = 150.0


async def market_maker():
    current_price = START_PRICE
    order_id = 1000000

    while True:
        try:
            # We use the subprotocol that the Svelte app expects/uses
            async with websockets.connect(WS_URL, subprotocols=["cex-protocol"]) as ws:
                print(f"Market Maker connected to {WS_URL}")

                while True:
                    # Random walk for the theoretical mid price
                    price_change = random.uniform(-0.5, 0.5)
                    current_price = max(1.0, current_price + price_change)

                    # Generate maker orders (Limit) to populate the order book
                    spread = random.uniform(0.1, 0.5)
                    bid_price = round(current_price - spread, 2)
                    ask_price = round(current_price + spread, 2)

                    qty_bid = round(random.uniform(0.5, 5.0), 2)
                    qty_ask = round(random.uniform(0.5, 5.0), 2)

                    # Send Bid (Maker)
                    bid_order = {
                        "action": "submit_order",
                        "order": {
                            "id": order_id,
                            "user_id": 999,  # Market Maker ID
                            "side": "BUY",
                            "type": "LIMIT",
                            "price": bid_price,
                            "quantity": qty_bid,
                        },
                    }
                    await ws.send(json.dumps(bid_order))
                    order_id += 1

                    # Send Ask (Maker)
                    ask_order = {
                        "action": "submit_order",
                        "order": {
                            "id": order_id,
                            "user_id": 999,
                            "side": "SELL",
                            "type": "LIMIT",
                            "price": ask_price,
                            "quantity": qty_ask,
                        },
                    }
                    await ws.send(json.dumps(ask_order))
                    order_id += 1

                    # Occasional Taker Order (Market) to cause trades/slippage and consume liquidity
                    if random.random() < 0.2:
                        side = random.choice(["BUY", "SELL"])
                        market_order = {
                            "action": "submit_order",
                            "order": {
                                "id": order_id,
                                "user_id": 998,  # Taker Bot ID
                                "side": side,
                                "type": "MARKET",
                                "price": 0.0,  # Market orders don't need a price
                                "quantity": round(random.uniform(0.1, 2.0), 2),
                            },
                        }
                        await ws.send(json.dumps(market_order))
                        print(
                            f"Sent MARKET {side} order (Qty: {market_order['order']['quantity']})"
                        )
                        order_id += 1

                    print(
                        f"Market Maker update: Mid={current_price:.2f}, Bid={bid_price}, Ask={ask_price}"
                    )

                    # Wait a bit before next market update
                    await asyncio.sleep(random.uniform(1.0, 3.0))

        except websockets.exceptions.ConnectionClosed:
            print("Connection lost. Retrying in 3 seconds...")
            await asyncio.sleep(3)
        except ConnectionRefusedError:
            print(
                f"Connection refused. Make sure the CEX websocket engine is running on {WS_URL}."
            )
            await asyncio.sleep(3)
        except Exception as e:
            print(f"Error: {e}")
            await asyncio.sleep(3)


if __name__ == "__main__":
    print("Starting Market Maker bot...")
    asyncio.run(market_maker())
