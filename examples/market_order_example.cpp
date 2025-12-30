import std;

import datetime;
import simulation_engine;
using namespace sim;

class MarketOrderExampleStrategy : public IStrategy<10> {
   public:
    MarketOrderExampleStrategy()
        : IStrategy<10>(Portfolio{}),
          quotesProcessed_(0),
          startTime_(std::chrono::high_resolution_clock::now()) {}

    void onMarketData(const Quote<10>& q) override {
        if (quotesProcessed_ % 100000 == 0) {
            std::cout << "Current quote: " << q.timestamp << " "
                      << datetime::DateTime::fromEpochTime(q.timestamp.value(), true) << std::endl;

            // Place a market order for 10 shares
            OrderId orderId = placeOrder(SymbolId{1},  // Symbol 1
                OrderInstruction::Buy,                 // Buy order
                OrderType::Market,                     // Market order
                Quantity{10},                          // 10 shares
                TimeInForce::Day                       // Day order
            );
            std::cout << "Placed market order: " << orderId.value() << std::endl;
        }
        quotesProcessed_++;
    }

    void onFill(const Fill& fill) {
        std::cout << "Fill!:  " << "DateTime: "
                  << datetime::DateTime::fromEpochTime(fill.timestamp.value(), true)
                  << " Price: " << fill.price.value() << " Quantity: " << fill.quantity.value()
                  << std::endl;
    }

    void onEnd() override {
        std::cout << "Last timestamp: " << datetime::DateTime::fromEpochTime(quotesProcessed_, false)
                  << std::endl;
    }

   private:
    std::size_t quotesProcessed_;
    std::chrono::high_resolution_clock::time_point startTime_;
};

RunParams setRunParams() {
    // Create run parameters
    RunParams params;
    params.symbols = {SymbolId{1}};              // Simulate symbol 1
    params.depth = Depth{10};                    // Use 10 price levels
    params.startingCash = Ticks{1'000'000'000};  // Start with $100
    params.commissionPerShareMaker = Ticks{0};   // No maker fees for testing
    params.commissionPerShareTaker = Ticks{0};   // No taker fees for testing
    params.strategyName = "DataCompressionTest";
    params.outputFile = "data_compression_test_results.csv";

    // Set high fill rate to ensure orders get executed
    params.fillRate = 100;              // 100% fill rate
    params.fillRateStdDev = 0;          // No variance
    params.partialFillProbability = 0;  // No partial fills
    params.useRandomness = false;       // Disable randomness for testing

    params.sendLatencyNanoseconds = 5'000'000'000;     // 5 seconds
    params.receiveLatencyNanoseconds = 5'000'000'000;  // 5 seconds (total: 10 seconds)
    params.interestRate = Percentage{5};               // 5% annual interest rate

    return params;
}

int main() {
    std::string inputParquetFile = "/mnt/klmncap3/tmp_simulation_data/ubigint_AAPL_2025-10-24.parquet";

    std::unordered_map<std::string, SymbolId> symbolIdMap;
    symbolIdMap["AAPL"] = sim::SymbolId{1};

    // Create the market data class with the input file.
    auto dataManager = std::make_unique<SingleSymbolMarketDataParquet<10>>("AAPL", symbolIdMap, "2025-08-14", "2025-08-15", inputParquetFile);

    RunParams params = setRunParams();

    MarketOrderExampleStrategy strat;

    Engine<10> engine(std::move(dataManager), params);

    engine.run(strat, VerbosityLevel::STANDARD);

    return 0;
}
