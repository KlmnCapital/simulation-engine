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
    struct RunParams {
        // Symbols, depth, and starting cash
        std::vector<SymbolId> symbols;
        Depth depth{kDefaultDepth};
        Ticks startingCash{0};

        // Latency
        std::uint64_t sendLatencyNanoseconds{30'000'000};     // 30 milliseconds
        std::uint64_t receiveLatencyNanoseconds{30'000'000};  // 30 milliseconds

        // Commissions
        Ticks commissionPerShareMaker{0};
        Ticks commissionPerShareTaker{0};

        // Order execution parameters
        std::uint8_t fillRate{95};
        std::uint8_t fillRateStdDev{5};
        std::uint8_t partialFillProbability{30};
        bool useRandomness{true};  // Toggle for testing
        std::uint32_t randomSeed{0};    // 0 means use random seed, non-zero means use specific seed

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
    };

}  // namespace sim
