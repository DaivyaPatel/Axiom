import os
import json
import asyncio
import websockets
import websockets.exceptions

TRADES_FILE = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'build', 'trades.json')

async def stream_trades(websocket):
    print(f"Client connected. Streaming from: {TRADES_FILE}")

    if not os.path.exists(TRADES_FILE):
        await websocket.send(json.dumps({"error": "trades.json not found. Run AXIOM engine first!"}))
        return

    try:
        with open(TRADES_FILE, 'r') as f:
            trades = json.load(f)

        print(f"Loaded {len(trades)} trades to stream.")

        for trade in trades:
            await websocket.send(json.dumps(trade))
            await asyncio.sleep(0.001)  # 1ms between trades

        await websocket.send(json.dumps({"status": "Stream complete"}))
        print("Stream complete.")

    except websockets.exceptions.ConnectionClosedOK:
        print("Client disconnected cleanly.")
    except Exception as e:
        print(f"Error: {e}")

async def main():
    port = 8765
    print(f"AXIOM WebSocket Server starting on ws://localhost:{port}")
    async with websockets.serve(stream_trades, "localhost", port):
        await asyncio.Future()  # run forever

if __name__ == "__main__":
    asyncio.run(main())