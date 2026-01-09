// statistics.cpp
module simulation_engine;

import std;

namespace sim {

// Constructor
template <std::size_t depth, typename Distribution>
Statistics<depth, Distribution>::Statistics(const RunParams<Distribution>& simulationParams)
    : simulationParams_{simulationParams},
      startingMarketValue_{simulationParams.startingCash},
      sampleRateSeconds_{simulationParams.statisticsUpdateRateSeconds},
      runningStatistics{simulationParams} {}

// Output methods
template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::outputSummary(std::ostream& outFile,
    VerbosityLevel verbosity) {
    switch (verbosity) {
        case VerbosityLevel::MINIMAL:
            outputMinimal(outFile);
            break;
        case VerbosityLevel::STANDARD:
            outputStandard(outFile);
            break;
        case VerbosityLevel::DETAILED:
            outputDetailed(outFile);
            break;
    }
}

template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::outputMinimal(std::ostream& out) const {
    outputHeader(out, "Simulation Results");

    Ticks finalMarketValue = runningStatistics.previousPortfolioValue;

    // Calculate total return from current equity
    double totalReturn = 0.0;
    if (startingMarketValue_.value() != 0) {
        totalReturn =
            (static_cast<double>(finalMarketValue.value()) - startingMarketValue_.value()) / startingMarketValue_.value();
    }

    out << "Starting Equity: " << formatTicksAsDollars(startingMarketValue_) << std::endl;
    out << "Final Portfolio Value: " << formatTicksAsDollars(finalMarketValue) << std::endl;
    out << "Total Return: " << formatPercentage(totalReturn) << std::endl;
    out << "Max Drawdown: " << formatPercentage(this->calculateMaxDrawdownPercent()) << std::endl;
    out << "Volatility: " << formatPercentage(calculateVolatility()) << std::endl;
    out << "Sharpe Ratio: " << std::fixed << std::setprecision(4)
        << calculateAnnualizedSharpeRatio() << std::endl;
    out << "Interest Owed: " << formatTicksAsDollars(totalInterestOwed_) << std::endl;
    out << "Fills: " << fillsHistory.size() << std::endl;
}

template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::outputStandard(std::ostream& out) const {
    // Output the minimal output
    outputMinimal(out);

    // Additional metrics for STANDARD verbosity
    double maxDrawdown = this->calculateMaxDrawdownPercent();
    out << "Max Drawdown: " << formatPercentage(maxDrawdown) << std::endl;

    // Additionally, output orders and fills
    outputOrdersPlaced(out);
    outputFillsReceived(out);
}

template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::outputDetailed(std::ostream& out) const {
    // TODO: Detailed mode will show various stats about multiple simulation trials
    // when rerunning with different competition/liquidity hyperparameters
    // For now, detailed does nothing

    // Commented out for future implementation:
    // - Multiple trial statistics
    // - Competition/liquidity hyperparameter analysis
    // - Performance comparison across different parameter sets
    // outputOrdersPlaced(out);
    // outputFillsReceived(out);
}

template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::outputHeader(std::ostream& out,
    const std::string& title) const {
    out << "\n" << title << "\n";
    out << std::string(title.length(), '-') << "\n";
}

template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::outputSeparator(std::ostream& out) const {
    out << "\n" << std::string(50, '=') << "\n";
}

template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::outputOrdersPlaced(std::ostream& out) const {
    outputHeader(out, "Orders Placed");

    if (orderHistory.empty()) {
        out << "No orders were placed during the simulation." << std::endl;
    } else {
        out << "Total Orders Placed: " << orderHistory.size() << std::endl;
        out << std::endl;

        out << std::left << std::setw(8) << "OrderID" << std::setw(8) << "Symbol" << std::setw(6)
            << "Side" << std::setw(8) << "Type" << std::setw(12) << "Quantity" << std::setw(15)
            << "Price" << std::setw(8) << "TIF" << std::setw(30) << "Timestamp" << std::endl;
        out << std::string(95, '-') << std::endl;

        for (const auto& orderWithTimestamp : orderHistory) {
            const auto& order = orderWithTimestamp.order;
            out << std::left << std::setw(8) << order.id.value() << std::setw(8)
                << order.symbol << std::setw(6) << formatOrderInstruction(order.instruction)
                << std::setw(8) << formatOrderType(order.orderType) << std::setw(12)
                << order.quantity.value() << std::setw(15) << formatTicksAsDollars(order.price)
                << std::setw(8) << formatTimeInForce(order.timeInForce) << std::setw(30)
                << datetime::DateTime::fromEpochTime(orderWithTimestamp.timestamp.value(), true)
                << std::endl;
        }
    }
}

template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::outputFillsReceived(std::ostream& out) const {
    outputHeader(out, "Fills Received");

    if (fillsHistory.empty()) {
        out << "No fills were received during the simulation." << std::endl;
    } else {
        out << "Total Fills Received: " << fillsHistory.size() << std::endl;
        out << std::endl;

        out << std::left << std::setw(8) << "OrderID" << std::setw(8) << "Symbol" << std::setw(6)
            << "Side" << std::setw(12) << "Quantity" << std::setw(15) << "Price" << std::setw(30)
            << "Timestamp" << std::endl;
        out << std::string(79, '-') << std::endl;

        for (const auto& fill : fillsHistory) {
            out << std::left << std::setw(8) << fill.id.value() << std::setw(8) << fill.symbol
                << std::setw(6) << formatOrderInstruction(fill.instruction) << std::setw(12)
                << fill.quantity.value() << std::setw(15) << formatTicksAsDollars(fill.price)
                << std::setw(30) << datetime::DateTime::fromEpochTime(fill.timestamp.value(), true)
                << std::endl;
        }
    }
}

// Methods to format data for output
template <std::size_t depth, typename Distribution>
std::string Statistics<depth, Distribution>::formatTicksAsDollars(Ticks ticks) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << "$"
        << (static_cast<double>(ticks.value()) / 1'000'000.0);
    return oss.str();
}

template <std::size_t depth, typename Distribution>
std::string Statistics<depth, Distribution>::formatCurrency(double amount) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << "$" << amount;
    return oss.str();
}

template <std::size_t depth, typename Distribution>
std::string Statistics<depth, Distribution>::formatPercentage(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << (value * 100.0) << "%";
    return oss.str();
}

template <std::size_t depth, typename Distribution>
std::string Statistics<depth, Distribution>::formatOrderInstruction(
    OrderInstruction instruction) const {
    switch (instruction) {
        case OrderInstruction::Buy:
            return "BUY";
        case OrderInstruction::Sell:
            return "SELL";
        default:
            return "UNKNOWN";
    }
}

template <std::size_t depth, typename Distribution>
std::string Statistics<depth, Distribution>::formatOrderType(OrderType orderType) const {
    switch (orderType) {
        case OrderType::Limit:
            return "LIMIT";
        case OrderType::Market:
            return "MARKET";
        case OrderType::TrailingStop:
            return "TRAIL_STOP";
        case OrderType::StopMarket:
            return "STOP_MKT";
        case OrderType::StopLimit:
            return "STOP_LMT";
        default:
            return "UNKNOWN";
    }
}

template <std::size_t depth, typename Distribution>
std::string Statistics<depth, Distribution>::formatTimeInForce(TimeInForce timeInForce) const {
    switch (timeInForce) {
        case TimeInForce::Day:
            return "DAY";
        case TimeInForce::IOC:
            return "IOC";
        case TimeInForce::FOK:
            return "FOK";
        case TimeInForce::GTC:
            return "GTC";
        default:
            return "UNKNOWN";
    }
}

// Methods to update class members
template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::updateStatistics(Ticks portfolioLiquidationValue) {
    runningStatistics.update(portfolioLiquidationValue);
}

template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::recordOrder(const NewOrder& order, TimeStamp timestamp) {
    OrderWithTimestamp orderWithTimestamp;
    orderWithTimestamp.order = order;
    orderWithTimestamp.timestamp = timestamp;
    orderHistory.push_back(orderWithTimestamp);
}

template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::recordFill(const Fill& fill) {
    fillsHistory.push_back(fill);
}

template <std::size_t depth, typename Distribution>
void Statistics<depth, Distribution>::updateInterestOwed(Ticks interestOwed) {
    totalInterestOwed_ = interestOwed;
}

template <std::size_t depth, typename Distribution>
double Statistics<depth, Distribution>::calculateVolatility() const {
    return runningStatistics.calculateAnnualizedVolatility(sampleRateSeconds_);
}

template <std::size_t depth, typename Distribution>
double Statistics<depth, Distribution>::calculateAnnualizedSharpeRatio() const {
    // Using 0.0 as risk-free rate (can be made configurable)
    return runningStatistics.calculateAnnualizedSharpeRatio(0.0, sampleRateSeconds_);
}

template <std::size_t depth, typename Distribution>
double Statistics<depth, Distribution>::calculateMaxDrawdownPercent() const {
    Ticks minimumMarketValue = runningStatistics.minimumPortfolioValue;
    return (startingMarketValue_.value() - minimumMarketValue.value()) /
        static_cast<double>(startingMarketValue_.value());
}

// Explicit template instantiations
template class Statistics<10, ConstantDistribution>;

}  // namespace sim
