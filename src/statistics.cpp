// statistics.cpp
module simulation_engine;

import std;

namespace sim {

    template <std::size_t depth>
    Statistics<depth>::Statistics(const RunParams& simulationParams)
        : simulationParams_{simulationParams},
        startingEquity_{simulationParams.startingCash},
        currentEquity_{simulationParams.startingCash} {
        // Initialize metrics
        totalReturn_ = 0.0;
        maxDrawdown_ = 0.0;
        volatility_ = 0.0;
        sharpeRatio_ = 0.0;

        // Initialize tracking variables
        maxEquity_ = startingEquity_;
        minCash_ = startingEquity_;
        maxPosition_ = Quantity{0};
        totalPositionValue_ = 0.0;
        positionUpdates_ = 0;
        previousEquity_ = Ticks{0};
    }

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

        // Calculate total return from current equity
        double totalReturn = 0.0;
        if (startingEquity_.value() != 0) {
            totalReturn = (static_cast<double>(currentEquity_.value()) -
                            static_cast<double>(startingEquity_.value())) /
                static_cast<double>(startingEquity_.value());
        }

        out << "Starting Equity: " << formatTicksAsDollars(startingEquity_) << std::endl;
        out << "Current Equity: " << formatTicksAsDollars(currentEquity_) << std::endl;
        out << "Total Return: " << formatPercentage(totalReturn) << std::endl;
        out << "Max Drawdown: " << formatPercentage(maxDrawdown_) << std::endl;
        out << "Volatility: " << formatPercentage(volatility_) << std::endl;
        out << "Sharpe Ratio: " << std::fixed << std::setprecision(4) << sharpeRatio_ << std::endl;
        out << "Max Position Value Held: " << formatTicksAsDollars(maxPositionValueHeld_) << std::endl;
        out << "Interest Owed: " << formatTicksAsDollars(interestOwed_) << std::endl;
        out << "Fills: " << fillsReceived_.size() << std::endl;
    }

    template <std::size_t depth>
    void Statistics<depth>::outputStandard(std::ostream& out) const {
        // Output the minimal output
        outputMinimal(out);

        // Additional metrics for STANDARD verbosity
        if (positionUpdates_ > 0) {
            double avgPosition = totalPositionValue_ / positionUpdates_;
            out << "Average Position: " << std::fixed << std::setprecision(2) << avgPosition
                << std::endl;
        }
        out << "Max Position: " << maxPosition_.value() << std::endl;
        out << "Min Cash: " << formatTicksAsDollars(minCash_) << std::endl;

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

    // New methods for recording and outputting orders and fills
    template <std::size_t depth>
    void Statistics<depth>::recordOrderPlaced(const NewOrder& order, TimeStamp timestamp) {
        OrderWithTimestamp orderWithTimestamp;
        orderWithTimestamp.order = order;
        orderWithTimestamp.timestamp = timestamp;
        ordersPlaced_.push_back(orderWithTimestamp);
    }

    template <std::size_t depth>
    void Statistics<depth>::recordFillReceived(const Fill& fill) {
        fillsReceived_.push_back(fill);
    }

    template <std::size_t depth>
    void Statistics<depth>::updateCurrentEquity(Ticks equity) {
        currentEquity_ = equity;
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

    template <std::size_t depth>
    void Statistics<depth>::updateStatsOnFill(Ticks cash,
        Quantity quantity,
        Ticks costBasis,
        VerbosityLevel verbosity) {
        // Note: We can't call portfolio.currentValue() here because we don't have access to bid/ask
        // prices The engine should call updateCurrentEquity() directly with the proper portfolio value
        // This method focuses on updating metrics based on the provided portfolio state

        // Update metrics based on verbosity level
        updateVolatility();
        updateSharpe();
        updateMaxDrawdown();

        if (verbosity >= VerbosityLevel::STANDARD) {
            updateMaxPosition(quantity);
            updateMinCash(cash);
            updateAveragePosition(quantity);
        }
        
        // Update max position value held (always track this for minimal output)
        Ticks positionValue = Ticks{quantity.value() * costBasis.value()};
        updateMaxPositionValueHeld(positionValue);
    }

    template <std::size_t depth>
    void Statistics<depth>::updateVolatility() {
        if (previousEquity_ != Ticks{0}) {
            // Calculate return for this period
            double currentReturn = (static_cast<double>(currentEquity_.value()) -
                                    static_cast<double>(previousEquity_.value())) /
                static_cast<double>(previousEquity_.value());
            returns_.push_back(currentReturn);

            // Keep only last 100 returns for rolling volatility
            if (returns_.size() > 100) {
                returns_.erase(returns_.begin());
            }

            // Calculate volatility as standard deviation of returns
            if (returns_.size() > 1) {
                double mean = 0.0;
                for (double ret : returns_) {
                    mean += ret;
                }
                mean /= returns_.size();

                double variance = 0.0;
                for (double ret : returns_) {
                    variance += (ret - mean) * (ret - mean);
                }
                variance /= (returns_.size() - 1);

                volatility_ = std::sqrt(variance);
            }
        }
        previousEquity_ = currentEquity_;
    }

    template <std::size_t depth>
    void Statistics<depth>::updateSharpe() {
        if (volatility_ > 0.0) {
            // Calculate total return
            totalReturn_ = (static_cast<double>(currentEquity_.value()) -
                            static_cast<double>(startingEquity_.value())) /
                static_cast<double>(startingEquity_.value());

            // Sharpe ratio = (return - risk_free_rate) / volatility
            // Assuming risk-free rate of 0 for simplicity
            sharpeRatio_ = totalReturn_ / volatility_;
        } else {
            sharpeRatio_ = 0.0;
        }
    }

    template <std::size_t depth>
    void Statistics<depth>::updateMaxDrawdown() {
        // Update max equity if current is higher
        if (currentEquity_ > maxEquity_) {
            maxEquity_ = currentEquity_;
        }

        // Calculate current drawdown
        if (maxEquity_ > Ticks{0}) {
            double currentDrawdown = (static_cast<double>(maxEquity_.value()) -
                                        static_cast<double>(currentEquity_.value())) /
                static_cast<double>(maxEquity_.value());

            // Update max drawdown if current is worse
            if (currentDrawdown > maxDrawdown_) {
                maxDrawdown_ = currentDrawdown;
            }
        }
    }

    template <std::size_t depth>
    void Statistics<depth>::updateMaxPosition(Quantity currentQuantity) {
        // Track absolute value of position for max position
        Quantity absQuantity =
            currentQuantity >= Quantity{0} ? currentQuantity : Quantity{0} - currentQuantity;
        if (absQuantity > maxPosition_) {
            maxPosition_ = absQuantity;
        }
    }

    template <std::size_t depth>
    void Statistics<depth>::updateMaxPositionValueHeld(Ticks positionValue) {
        // Track absolute value of position value for max position value held
        Ticks absValue = positionValue >= Ticks{0} ? positionValue : Ticks{0} - positionValue;
        if (absValue > maxPositionValueHeld_) {
            maxPositionValueHeld_ = absValue;
        }
    }

    template <std::size_t depth>
    void Statistics<depth>::updateMinCash(Ticks currentCash) {
        if (currentCash < minCash_) {
            minCash_ = currentCash;
        }
    }

    template <std::size_t depth>
    void Statistics<depth>::updateAveragePosition(Quantity currentQuantity) {
        // Track running average of position size
        totalPositionValue_ += static_cast<double>(currentQuantity.value());
        positionUpdates_++;
    }

    // Explicit template instantiations for common depths
    template class Statistics<1>;
    template class Statistics<5>;
    template class Statistics<10>;

}  // namespace sim
