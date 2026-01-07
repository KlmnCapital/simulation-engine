// statistics.cpp
module simulation_engine;

import std;

namespace sim {

// Constructor
template <std::size_t depth>
Statistics<depth>::Statistics(const RunParams& simulationParams)
    : simulationParams_{simulationParams},
    startingValue_{simulationParams.startingCash},
    currentValue_{simulationParams.startingCash},
    sampleRateSeconds_{simulationParams.statisticsUpateRateSeconds} {
    // Initialize metrics
    totalReturn_ = 0.0;
    maxDrawdown_ = 0.0;
    volatility_ = 0.0;
    sharpeRatio_ = 0.0;

    // Initialize tracking variables
    maxValue_ = startingValue_;
    minValue_ = startingValue_;
    maxPosition_ = Quantity{0};
    totalPositionValue_ = 0.0;
    positionUpdates_ = 0;
    previousEquity_ = Ticks{0};
}

// Output methods
template <std::size_t depth>
void Statistics<depth>::outputSummary(std::ostream& outFile, VerbosityLevel verbosity) {
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

template <std::size_t depth>
void Statistics<depth>::outputMinimal(std::ostream& out) const {
    outputHeader(out, "Simulation Results");

    double finalMarketValue = runningStatistics.currentMarketValue;

    // Calculate total return from current equity
    double totalReturn = 0.0;
    if (startingValue_.value() != 0) {
        totalReturn = (finalMarketValue - startingValue_.value()) / startingValue_.value();
    }

    out << "Starting Equity: " << formatTicksAsDollars(startingValue_) << std::endl;
    out << "Final Portfolio Value: " << formatTicksAsDollars(finalMarketValue) << std::endl;
    out << "Total Return: " << formatPercentage(totalReturn) << std::endl;
    out << "Max Drawdown: " << formatPercentage(calculateMaxDrawdownPercent()) << std::endl;
    out << "Volatility: " << formatPercentage(calculateVolatility()) << std::endl;
    out << "Sharpe Ratio: " << std::fixed << std::setprecision(4) << calculateSharpe() << std::endl;
    out << "Interest Owed: " << formatTicksAsDollars(interestOwed_) << std::endl;
    out << "Fills: " << fillsReceived_.size() << std::endl;
}

template <std::size_t depth>
void Statistics<depth>::outputStandard(std::ostream& out) const {
    // Output the minimal output
    outputMinimal(out);

    // Additional metrics for STANDARD verbosity
    double maxDrawdown = calculateMaxDrawdownPercent();
    out << "Max Drawdown: " << formatPercentage(maxDrawdown) << std::endl;

    // Additionally, output orders and fills
    outputOrdersPlaced(out);
    outputFillsReceived(out);
}

template <std::size_t depth>
void Statistics<depth>::outputDetailed(std::ostream& out) const {
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

template <std::size_t depth>
void Statistics<depth>::outputHeader(std::ostream& out, const std::string& title) const {
    out << "\n" << title << "\n";
    out << std::string(title.length(), '-') << "\n";
}

template <std::size_t depth>
void Statistics<depth>::outputSeparator(std::ostream& out) const {
    out << "\n" << std::string(50, '=') << "\n";
}

template <std::size_t depth>
void Statistics<depth>::updateInterestOwed(Ticks interestOwed) {
    interestOwed_ = interestOwed;
}

template <std::size_t depth>
void Statistics<depth>::outputOrdersPlaced(std::ostream& out) const {
    outputHeader(out, "Orders Placed");

    if (ordersPlaced_.empty()) {
        out << "No orders were placed during the simulation." << std::endl;
    } else {
        out << "Total Orders Placed: " << ordersPlaced_.size() << std::endl;
        out << std::endl;

        out << std::left << std::setw(8) << "OrderID" << std::setw(8) << "Symbol" << std::setw(6)
            << "Side" << std::setw(8) << "Type" << std::setw(12) << "Quantity" << std::setw(15)
            << "Price" << std::setw(8) << "TIF" << std::setw(30) << "Timestamp" << std::endl;
        out << std::string(95, '-') << std::endl;

        for (const auto& orderWithTimestamp : ordersPlaced_) {
            const auto& order = orderWithTimestamp.order;
            out << std::left << std::setw(8) << order.id.value() << std::setw(8)
                << order.symbol.value() << std::setw(6) << formatOrderInstruction(order.instruction)
                << std::setw(8) << formatOrderType(order.orderType) << std::setw(12)
                << order.quantity.value() << std::setw(15) << formatTicksAsDollars(order.price)
                << std::setw(8) << formatTimeInForce(order.timeInForce) << std::setw(30)
                << datetime::DateTime::fromEpochTime(orderWithTimestamp.timestamp.value(), true) << std::endl;
        }
    }
}

template <std::size_t depth>
void Statistics<depth>::outputFillsReceived(std::ostream& out) const {
    outputHeader(out, "Fills Received");

    if (fillsReceived_.empty()) {
        out << "No fills were received during the simulation." << std::endl;
    } else {
        out << "Total Fills Received: " << fillsReceived_.size() << std::endl;
        out << std::endl;

        out << std::left << std::setw(8) << "OrderID" << std::setw(8) << "Symbol" << std::setw(6)
            << "Side" << std::setw(12) << "Quantity" << std::setw(15) << "Price" << std::setw(30)
            << "Timestamp" << std::endl;
        out << std::string(79, '-') << std::endl;

        for (const auto& fill : fillsReceived_) {
            out << std::left << std::setw(8) << fill.id.value() << std::setw(8)
                << fill.symbol.value() << std::setw(6) << formatOrderInstruction(fill.instruction)
                << std::setw(12) << fill.quantity.value() << std::setw(15)
                << formatTicksAsDollars(fill.price) << std::setw(30)
                << datetime::DateTime::fromEpochTime(fill.timestamp.value(), true) << std::endl;
        }
    }
}


//Methods to format data for output
template <std::size_t depth>
std::string Statistics<depth>::formatTicksAsDollars(Ticks ticks) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << "$"
        << (static_cast<double>(ticks.value()) / 1'000'000.0);
    return oss.str();
}

template <std::size_t depth>
std::string Statistics<depth>::formatCurrency(double amount) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << "$" << amount;
    return oss.str();
}

template <std::size_t depth>
std::string Statistics<depth>::formatPercentage(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << (value * 100.0) << "%";
    return oss.str();
}

template <std::size_t depth>
std::string Statistics<depth>::formatOrderInstruction(OrderInstruction instruction) const {
    switch (instruction) {
        case OrderInstruction::Buy:
            return "BUY";
        case OrderInstruction::Sell:
            return "SELL";
        default:
            return "UNKNOWN";
    }
}

template <std::size_t depth>
std::string Statistics<depth>::formatOrderType(OrderType orderType) const {
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

template <std::size_t depth>
std::string Statistics<depth>::formatTimeInForce(TimeInForce timeInForce) const {
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
template<std::size_t depth>
void Statistics<depth>::updateStatistics(Ticks portfolioLiquidationValue) {
    double portfolioLiquidationValueAsDouble = std::static_cast<double>(portfolioLiquidationValue.value());
    runningStatistics.update(portfolioLiquidationValueAsDouble, sampleRateSeconds_);
}

template <std::size_t depth>
void Statistics<depth>::recordOrder(const NewOrder& order, TimeStamp timestamp) {
    OrderWithTimestamp orderWithTimestamp;
    orderWithTimestamp.order = order;
    orderWithTimestamp.timestamp = timestamp;
    ordersPlaced_.push_back(orderWithTimestamp);
}

template <std::size_t depth>
void Statistics<depth>::recordFill(const Fill& fill) {
    fillsReceived_.push_back(fill);
}

template <std::size_t depth>
void Statistics<depth>::updateInterestOwed(Ticks interestOwed) {
    interestOwed_ = interestOwed;
}

template<std::size_t depth>
double Statistics<depth>::calculateVolatility() {
    return runningStatistics.calculateAnnualizedVolatility(sampleRateSeconds_);
}

template<std::size_t depth>
double Statistics<depth>::calculateAnnualizedSharpeRatio() {
    return runningStatistics.getSharpe(sampleRateSeconds_);
}

template<std::size_t depth>
double Statistics<depth>::calculateMaxDrawdownPercent() {
    Ticks minimumMarketValue = runningStatistics.minimum();
    return (std::static_cast<double>(startingMarketValue_) - minimumMarketValue) / std::static_cast<double>(startingMarketValue_);
}

// Explicit template instantiations for common depths
template class Statistics<1>;
template class Statistics<5>;
template class Statistics<10>;

}  // namespace sim
