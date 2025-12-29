# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/statistics.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/statistics.cppm"

export module simulation_engine:statistics;

import std;

import :order_placement;
import :portfolio;
import :run_params;
import :types;

export namespace sim {


    enum class VerbosityLevel : std::uint8_t { MINIMAL = 0, STANDARD = 1, DETAILED = 2 };
    enum class Metric : std::uint8_t { Dollars = 0, Percent = 1, Quantity = 2 };

    template <std::size_t depth>
    class Statistics {
    public:
        Statistics(const RunParams& simulationParams);

        void outputSummary(std::ostream& outFile, VerbosityLevel verbosity);


        void recordOrderPlaced(const NewOrder& order, TimeStamp timestamp);
        void recordFillReceived(const Fill& fill);
        void updateCurrentEquity(Ticks equity);
        void updateStatsOnFill(Ticks cash,
            Quantity quantity,
            Ticks costBasis,
            VerbosityLevel verbosity = VerbosityLevel::MINIMAL);
        void updateInterestOwed(Ticks interestOwed);

    private:
        void outputMinimal(std::ostream& out = std::cout) const;
        void outputStandard(std::ostream& out = std::cout) const;
        void outputDetailed(std::ostream& out = std::cout) const;
        void outputHeader(std::ostream& out, const std::string& title) const;
        void outputSeparator(std::ostream& out) const;


        void outputOrdersPlaced(std::ostream& out) const;
        void outputFillsReceived(std::ostream& out) const;
        std::string formatOrderInstruction(OrderInstruction instruction) const;
        std::string formatOrderType(OrderType orderType) const;
        std::string formatTimeInForce(TimeInForce timeInForce) const;

        std::string formatTicksAsDollars(Ticks ticks) const;
        std::string formatCurrency(double amount) const;
        std::string formatPercentage(double value) const;


        void updateVolatility();
        void updateSharpe();
        void updateMaxDrawdown();
        void updateMaxPosition(Quantity currentQuantity);
        void updateMaxPositionValueHeld(Ticks positionValue);
        void updateMinCash(Ticks currentCash);
        void updateAveragePosition(Quantity currentQuantity);

        const RunParams& simulationParams_;

        std::size_t numSamplesProcessed_{0};
        Ticks startingEquity_;
        Ticks currentEquity_;


        double totalReturn_{0.0};
        double maxDrawdown_{0.0};
        double volatility_{0.0};
        double sharpeRatio_{0.0};


        Ticks maxEquity_{0};
        Ticks minCash_{0};
        Quantity maxPosition_{0};
        Ticks maxPositionValueHeld_{0};
        double totalPositionValue_{0.0};
        std::size_t positionUpdates_{0};
        Ticks interestOwed_{0};


        std::vector<double> returns_;
        Ticks previousEquity_{0};


        struct OrderWithTimestamp {
            NewOrder order;
            TimeStamp timestamp;
        };


        std::vector<OrderWithTimestamp> ordersPlaced_;
        std::vector<Fill> fillsReceived_;
    };

}
