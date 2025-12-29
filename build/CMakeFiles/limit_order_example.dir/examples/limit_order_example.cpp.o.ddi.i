# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/examples/limit_order_example.cpp"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/examples/limit_order_example.cpp"
import std;

import simulation_engine;

using namespace sim;

class LimitOrderExampleStrategy : public IStrategy<10> {
   public:
    LimitOrderExampleStrategy()
        : IStrategy<10>(Portfolio{}),
          quotesProcessed_(0),
          ordersPlaced_(0),
          startTime_(std::chrono::high_resolution_clock::now()) {}

    void onMarketData(const Quote<10>& q) override {

        if (quotesProcessed_ == 0) {
            std::vector<Ticks> priceLevels = {
                Ticks{125 * 1'000'000'000LL},
                Ticks{124 * 1'000'000'000LL},
                Ticks{120 * 1'000'000'000LL}
            };

            for (const Ticks& limitPrice : priceLevels) {
                OrderId orderId = placeOrder(SymbolId{1},
                    OrderInstruction::Buy,
                    OrderType::Limit,
                    Quantity{10},
                    TimeInForce::Day,
                    limitPrice
                );

                ordersPlaced_++;
            }
        }
        quotesProcessed_++;
    }
    void onFill(const Fill& fill) {

    }

    void onEnd() override {

    }

   private:
    std::size_t quotesProcessed_;
    std::size_t ordersPlaced_;
    std::chrono::high_resolution_clock::time_point startTime_;
};

RunParams setRunParams() {

    RunParams params;
    params.symbols = {SymbolId{1}};
    params.depth = Depth{10};
    params.startingCash = Ticks{100'000'000'000};
    params.commissionPerShareMaker = Ticks{0};
    params.commissionPerShareTaker = Ticks{0};
    params.strategyName = "LimitOrderTest";
    params.outputFile = "limit_order_test_results.csv";


    params.fillRate = 100;
    params.fillRateStdDev = 0;
    params.partialFillProbability = 0;
    params.useRandomness = false;

    params.sendLatencyNanoseconds = 5'000'000;
    params.receiveLatencyNanoseconds = 5'000'000;
    params.interestRate = Percentage{5};

    return params;
}

int main() {

    std::string inputParquetFile = "/mnt/klmncap3/tmp_simulation_data/AAPL_2025-10-24.parquet";

    std::unordered_map<std::string, SymbolId> symbolIdMap;
    symbolIdMap["AAAPL"] = sim::SymbolId{1};


    auto dataManager = std::make_unique<SingleSymbolMarketDataParquet<10>>("AAAPL", symbolIdMap, "2025-08-14", "2025-08-15", inputParquetFile);


    RunParams params = setRunParams();


    LimitOrderExampleStrategy strat;


    Engine<10> engine(std::move(dataManager), params);


    engine.run(strat, VerbosityLevel::STANDARD);

    return 0;
}
