import std;

import datetime;
import simulation_engine;

namespace sim {

class MarketOrderExampleStrategy : public IStrategy<10, 1, ConstantDistribution> {
   public:
    MarketOrderExampleStrategy(const RunParams<ConstantDistribution>& params)
        : IStrategy<10, 1, ConstantDistribution>(Portfolio<1, ConstantDistribution>{params}),
          quotesProcessed_{0} {}

    void onMarketData(const MarketState<10, 1>& marketState) override {
        if (quotesProcessed_ == 0) {
            OrderId orderId = this->placeOrder(
                0, OrderInstruction::Buy, OrderType::Market, Quantity{1}, TimeInForce::Day);
        }
        ++quotesProcessed_;
    }

   private:
    std::size_t quotesProcessed_;
};

RunParams<ConstantDistribution> setRunParams() {
    RunParams<ConstantDistribution> params;
    params.depth = Depth{10};
    params.startingCash = Ticks{1'000'000'000};
    params.buyFillRateDistribution = ConstantDistribution{100.0};
    params.sellFillRateDistribution = ConstantDistribution{100.0};
    params.sendLatencyNanoseconds = 5'000'000;
    params.receiveLatencyNanoseconds = 5'000'000;
    params.leverageFactor = 1;
    params.interestRate = Percentage{5};
    params.strategyName = "LimitOrderTest";
    params.outputFile = "limit_order_test_results.csv";
    params.enforceTradingHours = true;
    params.allowExtendedHoursTrading = true;
    params.daylightSavings = true;
    params.verbosityLevel = VerbosityLevel::STANDARD;
    params.statisticsUpdateRateSeconds = 60;
    return params;
}

}  // namespace sim

int main() {
    std::vector<std::string> filePaths = {
        "/mnt/klmncap3/tmp_simulation_data_indexed/ubigint_AAPL_2025-10-24.parquet",
        "/mnt/klmncap3/tmp_simulation_data_indexed/ubigint_AAPL_2025-10-27.parquet"};

    auto params = sim::setRunParams();

    auto dataManager = std::make_unique<sim::MarketDataParquet<10, 1>>(filePaths);

    // std::unique_ptr<sim::IMarketData<10, 1>> baseDataManager = std::move(dataManager);

    sim::MarketOrderExampleStrategy strat{params};

    sim::Engine<10, 1, sim::ConstantDistribution> engine(std::move(dataManager), params);

    engine.run(strat, std::cout);

    return 0;
}
