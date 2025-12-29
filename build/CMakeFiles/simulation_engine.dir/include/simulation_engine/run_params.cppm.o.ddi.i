# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/run_params.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/run_params.cppm"
export module simulation_engine:run_params;

import :types;

import std;

export namespace sim {







    struct RunParams {

        std::vector<SymbolId> symbols;
        Depth depth{kDefaultDepth};
        Ticks startingCash{0};


        std::uint64_t sendLatencyNanoseconds{30'000'000};
        std::uint64_t receiveLatencyNanoseconds{30'000'000};


        Ticks commissionPerShareMaker{0};
        Ticks commissionPerShareTaker{0};


        std::uint8_t fillRate{95};
        std::uint8_t fillRateStdDev{5};
        std::uint8_t partialFillProbability{30};
        bool useRandomness{true};
        std::uint32_t randomSeed{0};


        std::string strategyName{"default"};
        std::string outputFile{"sim_results.csv"};


        std::uint8_t leverageFactor;
        Percentage interestRate;
    };

}
