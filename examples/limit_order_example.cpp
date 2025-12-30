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
        // Place limit orders at specific price levels
        if (quotesProcessed_ == 0) {
            std::vector<Ticks> priceLevels = {
                Ticks{261 * 10'000LL},  // $263.00
            };

            for (const Ticks& limitPrice : priceLevels) {
                OrderId orderId = placeOrder(SymbolId{1},  // Symbol 1
                    OrderInstruction::Buy,                 // Buy order
                    OrderType::Limit,                      // Limit order
                    Quantity{10},                          // 10 shares
                    TimeInForce::Day,                      // Day order
                    limitPrice                             // Limit price
                );
                // Order placed silently
                ordersPlaced_++;
            }
        }
        quotesProcessed_++;
    }
    void onFill(const Fill& fill) {
        // Fill received - processed silently
    }

    void onEnd() override {
        // Simulation completed - analysis removed for cleaner output
    }

   private:
    std::size_t quotesProcessed_;
    std::size_t ordersPlaced_;
    std::chrono::high_resolution_clock::time_point startTime_;
};

RunParams setRunParams() {
    // Create run parameters
    RunParams params;
    params.symbols = {SymbolId{1}};                // Simulate symbol 1
    params.depth = Depth{10};                      // Use 10 price levels
    params.startingCash = Ticks{1000'000'000};  // Start with $1,000
    params.commissionPerShareMaker = Ticks{0};     // No maker fees for testing
    params.commissionPerShareTaker = Ticks{0};     // No taker fees for testing
    params.strategyName = "LimitOrderTest";
    params.outputFile = "limit_order_test_results.csv";

    // Set realistic fill rate - orders should only fill when price is favorable
    params.fillRate = 100;              // 100% fill rate when price is favorable
    params.fillRateStdDev = 0;          // No variance
    params.partialFillProbability = 0;  // No partial fills
    params.useRandomness = false;       // Disable randomness for testing

    params.sendLatencyNanoseconds = 5'000'000;     // 5 seconds
    params.receiveLatencyNanoseconds = 5'000'000;  // 5 seconds (total: 10 seconds)
    params.interestRate = Percentage{5};           // 5% annual interest rate

    params.enforceTradingHours = true;
    params.allowExtendedHoursTrading = true;
    params.daylightSavings = true;

    return params;
}

int main() {
    // Define the path to the parquet file with the data.
    std::string inputParquetFile = "/mnt/klmncap3/tmp_simulation_data/ubigint_AAPL_2025-10-24.parquet";

    std::unordered_map<std::string, SymbolId> symbolIdMap;
    symbolIdMap["AAAPL"] = sim::SymbolId{1};

    // Create the market data class with the input file.
    auto dataManager = std::make_unique<SingleSymbolMarketDataParquet<10>>("AAAPL", symbolIdMap, "2025-08-14", "2025-08-15", inputParquetFile);

    // Define the parameters for the simulation.
    RunParams params = setRunParams();

    // Instantiate the strategy.
    LimitOrderExampleStrategy strat;

    // Instantiate the simulation engine with 10 price levels.
    Engine<10> engine(std::move(dataManager), params);

    // Run the simulation.
    engine.run(strat, VerbosityLevel::STANDARD);

    return 0;
}

