// statistics.cppm
export module simulation_engine:statistics;

import std;

import :order_placement;
import :portfolio;
import :run_params;
import :types;

export namespace sim {

// Define enum classes for safer input parameters.
enum class VerbosityLevel : std::uint8_t { 
    MINIMAL = 0,
    STANDARD = 1, 
    DETAILED = 2
};

enum class Metric : std::uint8_t { 
    Dollars = 0, 
    Percent = 1, 
    Quantity = 2 
};

struct RunningSimulationStatistics {
    uint64_t totalSamples = 0;
    double averageReturn = 0.0;
    double sumOfSquaredDifferences = 0.0; // M2 in Welford's
    double minimumPortfolioValue = std::numeric_limits<double>::max();
    double previousPortfolioValue = 0.0;

    /**
     * @brief Updates statistics using the current portfolio value.
     * @param currentPortfolioValue The current total value of the portfolio.
     */
    void update(double currentPortfolioValue) {
        // Track the lowest value reached (useful for Drawdown)
        minimumPortfolioValue = std::min(minimumPortfolioValue, currentPortfolioValue);

        // We need at least two points to calculate a "return"
        if (previousPortfolioValue > 0.0) {
            totalSamples++;

            // Calculate the percentage return for this period
            double currentReturn = (currentPortfolioValue - previousPortfolioValue) / previousPortfolioValue;

            // Welford's Algorithm for running mean and variance of returns
            double delta = currentReturn - averageReturn;
            averageReturn += delta / totalSamples;
            double delta2 = currentReturn - averageReturn;
            sumOfSquaredDifferences += delta * delta2;
        }

        previousPortfolioValue = currentPortfolioValue;
    }

    double getVariance() const {
        return (totalSamples > 1) ? sumOfSquaredDifferences / (totalSamples - 1) : 0.0;
    }

    double getStandardDeviation() const {
        return std::sqrt(getReturnVariance());
    }

    /**
     * @brief Annualized Volatility
     * @param samplesPerYear How many update() calls happen in a year (e.g., 252 for daily, 98280 for minutes)
     */
    double calculateAnnualizedVolatility(double samplesPerYear) const {
        return getReturnStandardDeviation() * std::sqrt(samplesPerYear);
    }

    /**
     * @brief Annualized Sharpe Ratio
     * @param annualizedRiskFreeRate e.g., 0.04 for 4%
     * @param samplesPerYear The frequency of samples per year
     */
    double calculateAnnualizedSharpeRatio(double annualizedRiskFreeRate, double samplesPerYear) const {
        double stdev = getReturnStandardDeviation();
        if (stdev == 0.0) return 0.0;

        // Convert annual risk-free rate to the rate for our specific sampling period
        double riskFreeRatePerPeriod = annualizedRiskFreeRate / samplesPerYear;
        double averageExcessReturn = averageReturn - riskFreeRatePerPeriod;

        // Annualize: (Mean Excess / StdDev) * sqrt(SamplesPerYear)
        return (averageExcessReturn / stdev) * std::sqrt(samplesPerYear);
    }
};

template <std::size_t depth>
class Statistics {
public:
    Statistics(const RunParams& simulationParams);

    void outputSummary(std::ostream& outFile, VerbosityLevel verbosity);

    void recordOrder(const NewOrder& order, TimeStamp timestamp);
    void recordFill(const Fill& fill);
    void updateStatistics(double portfolioLiquidationValue);

    void updateInterestOwed(Ticks interestOwed);

private:
    const RunParams& simulationParams_;

    Ticks startingMarketValue_;
    Ticks totalInterestOwed_{0};

    RunningStatistics runningStatistics;
    int sampleRateSeconds_;

    // Structure to track orders with timestamps
    struct OrderWithTimestamp {
        NewOrder order;
        TimeStamp timestamp;
    };

    // Members to track the history of the portfolio for later use in calculations
    std::vector<OrderWithTimestamp> orderHistory;
    std::vector<Fill> fillsHistory;

    // Output methods
    void outputMinimal(std::ostream& out = std::cout) const;
    void outputStandard(std::ostream& out = std::cout) const;
    void outputDetailed(std::ostream& out = std::cout) const;

    void outputHeader(std::ostream& out, const std::string& title) const;
    void outputSeparator(std::ostream& out) const;

    // Helper output methods
    void outputOrdersPlaced(std::ostream& out) const;
    void outputFillsReceived(std::ostream& out) const;
    std::string formatOrderInstruction(OrderInstruction instruction) const;
    std::string formatOrderType(OrderType orderType) const;
    std::string formatTimeInForce(TimeInForce timeInForce) const;
    std::string formatTicksAsDollars(Ticks ticks) const;
    std::string formatCurrency(double amount) const;
    std::string formatPercentage(double value) const;

    // Private methods for real-time metric updates
    double calculateVolatility();
    double calculateSharpe();
    Ticks calcualteMaxDrawdown();

}  // namespace sim
