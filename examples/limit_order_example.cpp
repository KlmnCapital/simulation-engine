import std;
import simulation_engine;

using namespace sim;

class LimitOrderExampleStrategy : public IStrategy<10, 1, ConstantFillDistribution> {
   public:
    LimitOrderExampleStrategy()
        : IStrategy<10, 1, ConstantFillDistribution>(Portfolio<1, ConstantFillDistribution>{}),
          quotesProcessed_(0),
          ordersPlaced_(0) {}

    void onMarketData(const MarketState<10, 1>& marketState) override {
        if (quotesProcessed_ == 0) {
            Ticks limitPrice{261 * 10'000LL};
            // Use this-> to help the compiler resolve the template base class method
            this->placeOrder(0, OrderInstruction::Buy, OrderType::Limit, Quantity{10},
                TimeInForce::Day, limitPrice);
            ordersPlaced++;
        }
        quotesProcessed++;
    }
};

// Ensure the return type is fully qualified
sim::RunParams<sim::ConstantFillDistribution> setRunParams() {
    sim::RunParams<sim::ConstantFillDistribution> params;

    params.depth = Depth{10};
    params.startingCash = Ticks{1'000'000'000};
    params.buyFillRateDistribution = ConstantFillDistribution{100};
    params.sellFillRateDistribution = ConstantFillDistribution{100};
    params.sendLatencyNanoseconds = 5'000'000;
    params.receiveLatencyNanoseconds = 5'000'000;
    params.leverageFactor = 1;
    params.interestRate = Percentage{5};
    params.strategyName = "LimitOrderTest";
    params.outputFile = "limit_order_test_results.csv";
    params.enforceTradingHours = true;
    params.allowExtendedHoursTrading = true;
    params.daylightSavings = true;
    params.verbosity = VerbosityLevel::STANDARD;
    params.statisticsUpateRateSeconds = 60;

    return params;
}

int main() {
    std::vector<std::string> filePaths = {
        "/mnt/klmncap3/tmp_simulation_data/ubigint_AAPL_2025-10-24.parquet",
        "/mnt/klmncap3/tmp_simulation_data/ubigint_AAPL_2025-10-27.parquet"};

    auto params = setRunParams();

    auto dataManager =
        std::make_unique<sim::SingleSymbolMarketDataParquet<10>>(filePaths, symbolIdMap);

    LimitOrderExampleStrategy strat;
    sim::Engine<10, 1, sim::ConstantFillDistribution> engine(std::move(dataManager), params);

    engine.run(strat, params.verbosityLevel);

    return 0;
}
