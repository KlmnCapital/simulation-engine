// engine.cpp
module;
#include <cassert>

module simulation_engine;

import std;

namespace sim {

    template <std::size_t depth>
    Engine<depth>::Engine(std::unique_ptr<IMarketData<depth>> marketData, RunParams params)
        : marketData_(std::move(marketData)), params_(params), stats_(params) {
        portfolio_.cash = params.startingCash;
        portfolio_.settledFunds = params.startingCash;  // Initialize settled funds to starting cash
        portfolio_.interestRate = params.interestRate;

        // Initialize random number generator
        if (params.useRandomness) {
            if (params.randomSeed == 0) {
                // Use random seed
                std::random_device rd;
                randomNumberGenerator_.seed(rd());
            } else {
                // Use specific seed
                randomNumberGenerator_.seed(params.randomSeed);
            }
        }

        // Initialize distributions
        fillRateDistribution_ = std::normal_distribution<float>(params.fillRate, params.fillRateStdDev);
        partialFillProbabilityDistribution_ =
            std::uniform_int_distribution<int>(0, 99);  // 0-99 for percentage
    }

    template <std::size_t depth>
    Result Engine<depth>::run(IStrategy<depth>& strategy, VerbosityLevel verbosity, std::ostream& out) {
        strategy_ = &strategy;
        strategy.setEngine(this);
        currentVerbosity_ = verbosity;

        // Initialize latency from params
        sendLatencyNs_ = params_.sendLatencyNanoseconds;
        receiveLatencyNs_ = params_.receiveLatencyNanoseconds;
        totalLatencyNs_ = sendLatencyNs_ + receiveLatencyNs_;

        Result result = simulate(strategy);

        // Print results
        stats_.outputSummary(out, verbosity);

        return result;
    }

    template <std::size_t depth>
    const Portfolio& Engine<depth>::portfolio() const {
        return portfolio_;
    }

    template <std::size_t depth>
    const Quote<depth>& Engine<depth>::marketData(SymbolId symbol) const {
        return currentQuote_;
    }

    template <std::size_t depth>
    bool Engine<depth>::sufficientEquityForOrder(const NewOrder& order) {
        assert(order.orderType == OrderType::Limit || order.orderType == OrderType::Market);

        auto currentQuote = marketData_->currentQuote();
        Ticks bestBid = currentQuote.bestBid();
        Ticks bestAsk = currentQuote.bestAsk();

        return portfolio_.sufficientEquityForOrder(order, bestBid, bestAsk, params_.leverageFactor);
    }

    template <std::size_t depth>
    OrderId Engine<depth>::placeOrder(SymbolId symbol,
        OrderInstruction instruction,
        OrderType orderType,
        Quantity quantity,
        TimeInForce timeInForce,
        Ticks price) {
        NewOrder order;
        order.id = ++nextOrderId_;
        order.symbol = symbol;
        order.instruction = instruction;
        order.orderType = orderType;
        order.quantity = quantity;
        order.timeInForce = timeInForce;
        order.price = price;

        if (!portfolio_.sufficientEquityForOrder(
                order, currentQuote_.bestBid(), currentQuote_.bestAsk(), params_.leverageFactor)) {
            std::cout << "Conditions not met to place order!";
            return OrderId{0};
        }

        TimeStamp sendTime = currentQuote_.timestamp;
        TimeStamp earliestExecution = TimeStamp(sendTime.value() + totalLatencyNs_);

        PendingOrder pendingOrder;
        pendingOrder.order = order;
        pendingOrder.sendTime = sendTime;
        pendingOrder.earliestExecution = earliestExecution;

        pendingOrders_.push_back(pendingOrder);

        stats_.recordOrderPlaced(order, sendTime);

        return order.id;
    }

    template <std::size_t depth>
    bool Engine<depth>::cancel(OrderId orderId) {
        // Check if order exists in pending orders
        auto it = std::find_if(pendingOrders_.begin(), pendingOrders_.end(),
            [orderId](const PendingOrder& po) { return po.order.id == orderId; });

        if (it != pendingOrders_.end()) {
            // Note: Since portfolio updates now happen only when orders fill,
            // there's no need to reverse portfolio changes when cancelling orders

            // Add cancel order with latency
            TimeStamp sendTime = currentQuote_.timestamp;
            TimeStamp earliestExecution = TimeStamp(sendTime.value() + totalLatencyNs_);

            CancelOrder cancelOrder;
            cancelOrder.orderId = orderId;
            cancelOrder.sendTime = sendTime;
            cancelOrder.earliestExecution = earliestExecution;

            pendingCancels_.push_back(cancelOrder);
            return true;
        }
        return false;
    }

    template <std::size_t depth>
    bool Engine<depth>::replace(OrderId orderId, Quantity newQuantity, Ticks newPrice) {
        // Check if order exists in pending orders
        auto it = std::find_if(pendingOrders_.begin(), pendingOrders_.end(),
            [orderId](const PendingOrder& po) { return po.order.id == orderId; });

        if (it != pendingOrders_.end()) {
            // Add replace order with latency
            TimeStamp sendTime = currentQuote_.timestamp;
            TimeStamp earliestExecution = TimeStamp(sendTime.value() + totalLatencyNs_);

            ReplaceOrder replaceOrder;
            replaceOrder.orderId = orderId;
            replaceOrder.newQuantity = newQuantity;
            replaceOrder.newPrice = newPrice;
            replaceOrder.sendTime = sendTime;
            replaceOrder.earliestExecution = earliestExecution;

            pendingReplaces_.push_back(replaceOrder);
            return true;
        }
        return false;
    }

    template <std::size_t depth>
    Result Engine<depth>::simulate(IStrategy<depth>& strategy) {
        strategy_ = &strategy;

        // Process market data
        while (marketData_->nextQuote()) {
            currentQuote_ = marketData_->currentQuote();
            ++quotesProcessed_;

            // Send strategy market data
            strategy.onMarketData(currentQuote_);

            // Check margin requirements and execute margin calls if necessary
            checkMarginRequirement();

            // Try to fill orders after 'sendLatency_' + 'recieveLatency' has
            // passed since order was sent from strategy.
            processPendingOrders();

            // Send fill notifications to strategy after 'recieveLatency_' time
            // has passed since fill.
            processPendingNotifications(strategy);

            // Process settlements each morning after 9am
            processSettlements();
        }

        strategy.onEnd();

        // Update final statistics including interest owed
        stats_.updateInterestOwed(portfolio_.interestOwed);

        return Result{std::move(fills_), portfolio_, quotesProcessed_};
    }

    template <std::size_t depth>
    typename Engine<depth>::ExecutionResult Engine<depth>::tryExecute(const NewOrder& newOrder,
        TimeStamp sendTs,
        const Quote<depth>& quote) {
        ExecutionResult result;
        result.fills.clear();
        result.remainingOrder = newOrder;
        result.isComplete = false;

        if (newOrder.orderType == OrderType::Market) {
            // Market order: check if it should fill based on randomness
            int actualFillRate;
            int fillDecision;

            if (params_.useRandomness) {
                actualFillRate =
                    std::clamp(static_cast<int>(fillRateDistribution_(randomNumberGenerator_)), 0, 100);
                fillDecision = partialFillProbabilityDistribution_(randomNumberGenerator_);
            } else {
                actualFillRate = params_.fillRate;
                fillDecision = 0;  // Always fill when randomness is disabled
            }

            if (fillDecision < actualFillRate) {
                // Order fills - determine if partial or complete
                bool isPartialFill;
                if (params_.useRandomness) {
                    isPartialFill = (partialFillProbabilityDistribution_(randomNumberGenerator_) <
                        params_.partialFillProbability);
                } else {
                    isPartialFill = (params_.partialFillProbability > 0);
                }
                Quantity fillQuantity =
                    isPartialFill ? Quantity{newOrder.quantity.value() / 2} : newOrder.quantity;

                Ticks executionPrice;
                if (newOrder.instruction == OrderInstruction::Buy) {
                    executionPrice = quote.bestAsk();
                } else {
                    executionPrice = quote.bestBid();
                }

                Fill fill;
                fill.id = newOrder.id;
                fill.symbol = newOrder.symbol;
                fill.quantity = fillQuantity;
                fill.price = executionPrice;
                fill.timestamp = quote.timestamp;
                fill.instruction = newOrder.instruction;
                fill.orderType = newOrder.orderType;
                fill.timeInForce = newOrder.timeInForce;
                fill.originalPrice = newOrder.price;

                result.fills.push_back(fill);
                result.isComplete = !isPartialFill;

                if (isPartialFill) {
                    result.remainingOrder = newOrder;
                    result.remainingOrder.quantity =
                        Quantity{newOrder.quantity.value() - fillQuantity.value()};
                }

                // Update portfolio with the fill
                portfolio_.updatePortfolio(fill);

                // Record the fill for results tracking
                stats_.recordFillReceived(fill);

                // Update current equity for display
                Ticks currentEquity =
                    portfolio_.currentValue(currentQuote_.bestBid(), currentQuote_.bestAsk());
                stats_.updateCurrentEquity(currentEquity);

                // Update statistics with proper portfolio value
                stats_.updateStatsOnFill(
                    portfolio_.cash, portfolio_.longQuantity, portfolio_.costBasis, currentVerbosity_);
                stats_.updateInterestOwed(portfolio_.interestOwed);

                // Schedule notification
                TimeStamp notificationTime = TimeStamp{fill.timestamp.value() + receiveLatencyNs_};
                notifyFill(fill, notificationTime);
            }
            // If fillDecision >= actualFillRate, order doesn't fill (result.isComplete = false)

        } else if (newOrder.orderType == OrderType::Limit) {
            // Limit order: check if price is favorable
            bool priceFavorable = false;
            Ticks executionPrice = newOrder.price;

            if (newOrder.instruction == OrderInstruction::Buy) {
                // Buy limit: execute if market ask <= our limit price
                priceFavorable = (quote.bestAsk() <= newOrder.price);
                executionPrice = std::min(quote.bestAsk(), newOrder.price);
            } else {
                // Sell limit: execute if market bid >= our limit price
                priceFavorable = (quote.bestBid() >= newOrder.price);
                executionPrice = std::max(quote.bestBid(), newOrder.price);
            }

            if (priceFavorable) {
                // Price is favorable, now check if it should fill based on randomness
                int actualFillRate;
                int fillDecision;

                if (params_.useRandomness) {
                    actualFillRate = std::clamp(
                        static_cast<int>(fillRateDistribution_(randomNumberGenerator_)), 0, 100);
                    fillDecision = partialFillProbabilityDistribution_(randomNumberGenerator_);
                } else {
                    actualFillRate = params_.fillRate;
                    fillDecision = 0;  // Always fill when randomness is disabled
                }

                if (fillDecision < actualFillRate) {
                    // Order fills - determine if partial or complete
                    bool isPartialFill;
                    if (params_.useRandomness) {
                        isPartialFill = (partialFillProbabilityDistribution_(randomNumberGenerator_) <
                            params_.partialFillProbability);
                    } else {
                        isPartialFill = (params_.partialFillProbability > 0);
                    }
                    Quantity fillQuantity =
                        isPartialFill ? Quantity{newOrder.quantity.value() / 2} : newOrder.quantity;

                    Fill fill;
                    fill.id = newOrder.id;
                    fill.symbol = newOrder.symbol;
                    fill.quantity = fillQuantity;
                    fill.price = executionPrice;
                    fill.timestamp = quote.timestamp;
                    fill.instruction = newOrder.instruction;
                    fill.orderType = newOrder.orderType;
                    fill.timeInForce = newOrder.timeInForce;
                    fill.originalPrice = newOrder.price;

                    result.fills.push_back(fill);
                    result.isComplete = !isPartialFill;

                    if (isPartialFill) {
                        result.remainingOrder = newOrder;
                        result.remainingOrder.quantity =
                            Quantity{newOrder.quantity.value() - fillQuantity.value()};
                    }

                    // Update portfolio with the fill
                    portfolio_.updatePortfolio(fill);

                    // Record the fill for results tracking
                    stats_.recordFillReceived(fill);

                    // Update current equity for display
                    Ticks currentEquity =
                        portfolio_.currentValue(currentQuote_.bestBid(), currentQuote_.bestAsk());
                    stats_.updateCurrentEquity(currentEquity);

                    // Update statistics with proper portfolio value
                    stats_.updateStatsOnFill(portfolio_.cash, portfolio_.longQuantity,
                        portfolio_.costBasis, currentVerbosity_);
                    stats_.updateInterestOwed(portfolio_.interestOwed);

                    // Schedule notification
                    TimeStamp notificationTime = TimeStamp{fill.timestamp.value() + receiveLatencyNs_};
                    notifyFill(fill, notificationTime);
                }
                // If fillDecision >= actualFillRate, order doesn't fill despite favorable price
            }
            // If price not favorable, order remains pending (result.isComplete = false)
        }

        return result;
    }

    template <std::size_t depth>
    void Engine<depth>::notifyFill(const Fill& fill, TimeStamp earliestNotificationTime) {
        // Add fill to results immediately
        fills_.push_back(fill);

        // Queue notification for later delivery
        pendingNotifications_.push_back({fill, earliestNotificationTime, false});
    }

    template <std::size_t depth>
    void Engine<depth>::processPendingNotifications(IStrategy<depth>& strategy) {
        auto it = pendingNotifications_.begin();
        while (it != pendingNotifications_.end()) {
            if (!it->delivered && currentQuote_.timestamp >= it->earliestNotifyTime) {
                // Time to deliver the notification
                strategy.onFill(it->fill);
                it->delivered = true;
                ++it;
            } else {
                ++it;
            }
        }

        // Remove delivered notifications to keep the queue clean
        pendingNotifications_.erase(
            std::remove_if(pendingNotifications_.begin(), pendingNotifications_.end(),
                [](const PendingNotification& notif) { return notif.delivered; }),
            pendingNotifications_.end());
    }

    template <std::size_t depth>
    void Engine<depth>::processPendingOrders() {
        // Process pending cancel orders first
        auto cancelIt = pendingCancels_.begin();
        while (cancelIt != pendingCancels_.end()) {
            if (currentQuote_.timestamp >= cancelIt->earliestExecution) {
                // Time to execute the cancel - remove the corresponding order
                auto orderIt = std::find_if(pendingOrders_.begin(), pendingOrders_.end(),
                    [cancelIt](const PendingOrder& po) { return po.order.id == cancelIt->orderId; });

                if (orderIt != pendingOrders_.end()) {
                    pendingOrders_.erase(orderIt);
                }

                // Remove the cancel order
                cancelIt = pendingCancels_.erase(cancelIt);
            } else {
                ++cancelIt;
            }
        }

        // Process pending replace orders
        auto replaceIt = pendingReplaces_.begin();
        while (replaceIt != pendingReplaces_.end()) {
            if (currentQuote_.timestamp >= replaceIt->earliestExecution) {
                // Time to execute the replace - modify the corresponding order
                auto orderIt = std::find_if(pendingOrders_.begin(), pendingOrders_.end(),
                    [replaceIt](const PendingOrder& po) { return po.order.id == replaceIt->orderId; });

                if (orderIt != pendingOrders_.end()) {
                    orderIt->order.quantity = replaceIt->newQuantity;
                    orderIt->order.price = replaceIt->newPrice;
                }

                // Remove the replace order
                replaceIt = pendingReplaces_.erase(replaceIt);
            } else {
                ++replaceIt;
            }
        }

        // Process pending orders
        auto it = pendingOrders_.begin();
        while (it != pendingOrders_.end()) {
            if (currentQuote_.timestamp >= it->earliestExecution) {
                ExecutionResult result = tryExecute(it->order, it->sendTime, currentQuote_);

                if (result.isComplete) {
                    // Order is complete, remove from pending
                    it = pendingOrders_.erase(it);
                } else {
                    // Order partially filled, update remaining quantity
                    it->order = result.remainingOrder;
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }

    template <std::size_t depth>
    Ticks Engine<depth>::currentPortfolioValue() const {
        return portfolio_.currentValue(currentQuote_.bestBid(), currentQuote_.bestAsk());
    }

    template <std::size_t depth>
    void Engine<depth>::processSettlements() {
        // Only process settlements once per day after 9am
        if (isTimeForSettlement(currentQuote_.timestamp)) {
            // Process unsettled funds settlements
            portfolio_.processSettlements(currentQuote_.timestamp);

            // Calculate and apply daily interest on outstanding loans
            portfolio_.calculateDailyInterest(currentQuote_.timestamp);

            lastSettlementDate_ = currentQuote_.timestamp;
        }
    }

    template <std::size_t depth>
    bool Engine<depth>::isTimeForSettlement(TimeStamp currentTime) const {
        // Convert timestamp to days since epoch (assuming nanoseconds since epoch)
        constexpr std::uint64_t nanosecondsPerDay = 24ULL * 60 * 60 * 1000000000ULL;
        constexpr std::uint64_t nanosecondsPerHour = 60ULL * 60 * 1000000000ULL;

        std::uint64_t currentDay = currentTime.value() / nanosecondsPerDay;
        std::uint64_t lastSettlementDay = lastSettlementDate_.value() / nanosecondsPerDay;

        // Check if it's a new day and after 9am
        if (currentDay > lastSettlementDay) {
            std::uint64_t timeInDay = currentTime.value() % nanosecondsPerDay;
            std::uint64_t nineAM = 9ULL * nanosecondsPerHour;  // 9am in nanoseconds

            return timeInDay >= nineAM;
        }

        return false;
    }

    template <std::size_t depth>
    void Engine<depth>::checkMarginRequirement() {
        Ticks bestBid = currentQuote_.bestBid();
        Ticks bestAsk = currentQuote_.bestAsk();

        Ticks currentEquity = portfolio_.equity(bestBid, bestAsk);
        Ticks maintenanceReq =
            portfolio_.maintenanceRequirement(bestBid);  // Use bid as conservative price

        // Check if equity is less than 30% maintenance requirement
        if (currentEquity < maintenanceReq) {
            executeMarginCall();
        }
    }

    template <std::size_t depth>
    void Engine<depth>::executeMarginCall() {
        Ticks bestBid = currentQuote_.bestBid();
        Ticks bestAsk = currentQuote_.bestAsk();

        // Liquidate positions until we meet 30% maintenance requirement
        while (true) {
            Ticks currentEquity = portfolio_.equity(bestBid, bestAsk);
            Ticks maintenanceReq = portfolio_.maintenanceRequirement(bestBid);

            // Check if we've met the requirement
            if (currentEquity >= maintenanceReq) {
                break;
            }

            // Determine what to liquidate first - start with largest positions
            bool liquidated = false;

            // Liquidate long positions first (sell at bid)
            if (portfolio_.longQuantity > Quantity{0}) {
                Quantity liquidateQty =
                    std::min(portfolio_.longQuantity, Quantity{100});  // Liquidate in chunks

                Fill marginCallFill;
                marginCallFill.id = OrderId{0};       // Special ID for margin calls
                marginCallFill.symbol = SymbolId{1};  // Assume single symbol for now
                marginCallFill.quantity = liquidateQty;
                marginCallFill.price = bestBid;
                marginCallFill.timestamp = currentQuote_.timestamp;
                marginCallFill.instruction = OrderInstruction::Sell;
                marginCallFill.orderType = OrderType::Market;
                marginCallFill.timeInForce = TimeInForce::Day;
                marginCallFill.originalPrice = bestBid;

                // Update portfolio with the forced liquidation
                portfolio_.updatePortfolio(marginCallFill);

                // Record the fill and queue notification
                stats_.recordFillReceived(marginCallFill);
                TimeStamp notificationTime =
                    TimeStamp{marginCallFill.timestamp.value() + receiveLatencyNs_};
                notifyFill(marginCallFill, notificationTime);

                liquidated = true;
            }
            // Liquidate short positions (buy to cover at ask)
            else if (portfolio_.shortQuantity > Quantity{0}) {
                Quantity liquidateQty =
                    std::min(portfolio_.shortQuantity, Quantity{100});  // Liquidate in chunks

                Fill marginCallFill;
                marginCallFill.id = OrderId{0};       // Special ID for margin calls
                marginCallFill.symbol = SymbolId{1};  // Assume single symbol for now
                marginCallFill.quantity = liquidateQty;
                marginCallFill.price = bestAsk;
                marginCallFill.timestamp = currentQuote_.timestamp;
                marginCallFill.instruction = OrderInstruction::Buy;  // Buy to cover short
                marginCallFill.orderType = OrderType::Market;
                marginCallFill.timeInForce = TimeInForce::Day;
                marginCallFill.originalPrice = bestAsk;

                // Update portfolio with the forced liquidation
                portfolio_.updatePortfolio(marginCallFill);

                // Record the fill and queue notification
                stats_.recordFillReceived(marginCallFill);
                TimeStamp notificationTime =
                    TimeStamp{marginCallFill.timestamp.value() + receiveLatencyNs_};
                notifyFill(marginCallFill, notificationTime);

                liquidated = true;
            }

            // If we couldn't liquidate anything, break to avoid infinite loop
            if (!liquidated) {
                break;
            }
        }
    }

    // Explicit template instantiations for common depths
    template class Engine<1>;
    template class Engine<5>;
    template class Engine<10>;

}  // namespace sim
