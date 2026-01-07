export module simulation_engine:run_params;

import :types;

import std;

export namespace sim {

/**
* @brief Configuration parameters for simulation runs
* @details Contains all configurable parameters for simulation execution including
* time ranges, capital limits, execution parameters, and strategy configuration.
* These parameters control the behavior of the simulation engine and risk management.
*/
template<typename Distribution>
struct RunParams {
    // Symbols, depth, and starting cash
    Depth depth{kDefaultDepth};
    Ticks startingCash{0};

    // Latency
    std::uint64_t sendLatencyNanoseconds{30'000'000};     // 30 milliseconds
    std::uint64_t receiveLatencyNanoseconds{30'000'000};  // 30 milliseconds

    // Commissions
    // Ticks commissionPerShareMaker{0};
    // Ticks commissionPerShareTaker{0};

    // Market competition paramaters
    Distribution buyFillRateDistribution;
    Distribution sellFillRateDistribution;

    // Strategy configuration
    std::string strategyName{"default"};
    std::string outputFile{"sim_results.csv"};

    // Margin params
    std::uint8_t leverageFactor;
    Percentage interestRate;

    // Trading hours
    bool enforceTradingHours;
    bool allowExtendedHoursTrading;
    bool daylightSavings;

    // Verbosity settings
    VerbosityLevel verbosityLevel;

    // Statistics settings
    int statisticsUpateRateSeconds;
};

}  // namespace sim
