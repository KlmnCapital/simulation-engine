import std;
import simulation_engine;

namespace sim {

class LimitOrderExampleStrategy : public IStrategy<10, 4, ConstantDistribution> {
   public:
    LimitOrderExampleStrategy(const RunParams<ConstantDistribution>& params)
        : IStrategy<10, 4, ConstantDistribution>(Portfolio<4, ConstantDistribution>{params}),
          quotesProcessed_{0} {}

    void onMarketData(const MarketState<10, 4>& marketState) override {
        if (quotesProcessed_ == 0) {
            Ticks limitPrice{261'000'000};
            this->placeOrder(0, OrderInstruction::Buy, OrderType::Limit, Quantity{1},
                TimeInForce::Day, limitPrice);
        }
        quotesProcessed_++;
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
        "/mnt/klmncap3/tmp_multisymbol_simulation_data/simulation_data_indexed/"
        "indexed_2025-07-13.parquet",
        "/mnt/klmncap3/tmp_multisymbol_simulation_data/simulation_data_indexed/"
        "indexed_2025-07-14.parquet"};

    auto params = sim::setRunParams();

    auto dataManager = std::make_unique<sim::MarketDataParquet<10, 4>>(filePaths);

    sim::LimitOrderExampleStrategy strat{params};

    sim::Engine<10, 4, sim::ConstantDistribution> engine(std::move(dataManager), params);

    engine.run(strat, std::cout);

    return 0;
}
