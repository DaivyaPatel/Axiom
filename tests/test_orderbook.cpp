#include <benchmark/benchmark.h>
#include "OrderBook.hpp"

static void BM_OrderSubmit(benchmark::State& state) {
    axiom::OrderBook book;
    axiom::Order order{1, 100, 10, axiom::Side::BUY, axiom::OrderType::LIMIT};
    for (auto _ : state) {
        book.submit_order(order);
    }
}
BENCHMARK(BM_OrderSubmit);

BENCHMARK_MAIN();
