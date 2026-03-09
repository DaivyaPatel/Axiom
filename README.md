# AXIOM

AXIOM is a lock-free order matching engine featuring a chaos-driven market microstructure simulator. It is designed for quantitative trading infrastructure, prioritizing extremely low latency and high throughput.

## Project Structure

* `src/`: Core engine implementation.
* `include/`: Header files, including the lock-free data structures (`OrderBook.hpp`).
* `simulator/`: Chaos-driven market microstructure simulator for stress-testing the matching engine under extreme market conditions.
* `tests/`: Benchmarks and unit tests (using Google Benchmark).

## Dependencies

* C++20
* Boost (Lockfree)
* Google Benchmark

## Build Instructions

```sh
mkdir build
cd build
cmake ..
cmake --build .
```
