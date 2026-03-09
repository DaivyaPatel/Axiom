# AXIOM — Adaptive eXchange Infrastructure with Order Microstructure

> A high-performance, lock-free order matching engine with a chaos-driven 
> market microstructure simulator and FIX 4.2 protocol gateway.
> Built to explore the engineering challenges of modern quantitative trading infrastructure.

---

## Architecture
```
┌─────────────────┐     FIX 4.2 Messages      ┌──────────────────────┐
│  FIX Gateway    │ ────────────────────────▶  │                      │
│  (fix_gateway)  │                            │   Matching Engine    │
└─────────────────┘                            │   (Lock-Free SPSC)   │
                                               │                      │
┌─────────────────┐     Synthetic Orders       │   Price-Time         │
│ Chaos Simulator │ ────────────────────────▶  │   Priority CLOB      │
│ (Lorenz/RK4)    │                            │                      │
└─────────────────┘                            └──────────┬───────────┘
                                                          │
                                               Matched Trades (JSON)
                                                          │
                                               ┌──────────▼───────────┐
                                               │  WebSocket Feed      │
                                               │  ws://localhost:8765 │
                                               └──────────────────────┘
```

---

## Benchmark Results

| Metric | Value |
|---|---|
| Orders processed | 1,000,000 |
| Trades generated | 954,990 |
| Time taken | 122 ms |
| **Orders/second** | **8,200,000+** |
| **Avg latency/order** | **122 ns** |

Benchmarked on a single thread with no external dependencies.

---

## Components

### 1. Lock-Free Matching Engine (`include/OrderBook.hpp`)
The core of AXIOM. A Central Limit Order Book (CLOB) with:
- **Custom SPSC ring buffer** — built from scratch using `std::atomic` with `memory_order_acquire/release` semantics. No mutexes, no locks.
- **Cache-line alignment** — `alignas(64)` on atomic head/tail pointers eliminates false sharing between producer and consumer threads
- **Price-time priority** — bids sorted descending, asks ascending via `std::map`. Orders at the same price level matched FIFO via `std::list`
- **Partial fills** — orders fill across multiple price levels until quantity is exhausted

### 2. Chaos-Driven Market Simulator (`simulator/`)
Synthetic order flow generated using the **Lorenz dynamical system** (σ=10, ρ=28, β=8/3) integrated with **Runge-Kutta 4th order**:
- `x` axis → price fluctuation around base price 100
- `y` axis → order arrival rate per tick  
- `z` axis → order quantity (clamped 1–100)
- **Lyapunov divergence tracking** — two trajectories initialized 0.0001 apart, divergence measured over time to characterize chaos regime
- Produces realistic fat-tailed order flow that Gaussian models cannot replicate

### 3. FIX 4.2 Protocol Gateway (`include/FIXGateway.hpp`)
Industry-standard Financial Information eXchange protocol implementation:
- Parses pipe-delimited FIX 4.2 messages (tags 8, 35, 49, 54, 38, 44, 40)
- Validates side, order type, and quantity
- Converts to internal `axiom::Order` and submits to lock-free queue
- Returns compliant **Execution Reports** (MsgType=8) with order status
- Rejects malformed messages with proper FIX reject messages

### 4. WebSocket Market Data Feed (`src/websocket_server.py`)
Live trade broadcasting after matching:
- Streams matched trades as JSON over WebSocket on `ws://localhost:8765`
- Each message contains: buy/sell order IDs, matched price, quantity, timestamp
- Tested streaming 954,990 trades to connected clients

---

## Build Instructions

**Requirements:** g++ (C++20), CMake 3.14+, MinGW on Windows
```bash
git clone https://github.com/YOURUSERNAME/AXIOM.git
cd AXIOM
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Run benchmark
.\axiom.exe

# Run chaos simulator  
.\axiom_simulator.exe

# Run FIX gateway test
.\fix_gateway_test.exe

# Stream trades via WebSocket (separate terminal)
cd ..
python src/websocket_server.py
```

---

## Technical Highlights

- **Zero external dependencies** — custom lock-free queue, no Boost required
- **Cache-line aware design** — false sharing eliminated via `alignas(64)`
- **Chaos theory applied to finance** — Lorenz attractor as market microstructure model
- **Industry protocol** — FIX 4.2, the standard used by NSE, NYSE, Binance
- **Full stack** — protocol parsing → order matching → live data feed

---

## Part of SummerFest'26 Trading Infrastructure Initiative