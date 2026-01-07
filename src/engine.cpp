// engine.cpp
module;
#include <cassert>

module simulation_engine;

import std;
import datetime;

namespace sim {

template <std::size_t depth, std::uint16_t numberOfSymbols>
Engine<depth, numberOfSymbols>::Engine(std::unique_ptr<IMarketData<depth, numberOfSymbols>> marketData, RunParams params)
    : marketData()(std::move(marketData)),
      params_(params),
      stats(params),
      portfolio(params),
      buyFillRateDistribution{params.buyFillRateDistribution},
      sellFillRateDistribution{params.sellFillRateDistribution},
      verbosityLevel{params.verbosityLevel},
      statisticsUpateRateSeconds{params.statisticsUpateRateSeconds},
      sendLatencyNs{params.sendLatencyNanoseconds},
      receiveLatencyNs{params.receiveLatencyNanoseconds},
      totalLatencyNs{params.receiveLatencyNs + params.sendLatencyNanoseconds},
      leverageFactor{params.leverageFactor} {}

template <std::size_t depth, std::uint16_t numberOfSymbols>
bool Engine<depth, numberOfSymbols>::canTrade(TimeStamp currentTimeStamp) const {
    if (!params_.enforceTradingHours) return true;

    datetime::DateTime currentDateTime = datetime::DateTime::fromEpochTime(static_cast<std::uint64_t>(currentTimeStamp));

    if (currentDateTime.isWeekend()) return false;

    bool isDST = params_.daylightSavings && currentDateTime.isInsideUSDST();
    double timeAsDecimal = currentDateTime.timeAsDecimal();

    // Define UTC Windows
    double regularStart = isDST ? 13.5 : 14.5; // 13:30 or 14:30 UTC
    double regularEnd = isDST ? 20.0 : 21.0; // 20:00 or 21:00 UTC
    double preStart = 9.0;                 // 09:00 UTC (Fixed)
    // afterEnd is 0.0 (Midnight) or 1.0 (1 AM)
    double afterEnd = isDST ? 0.0 : 1.0;

    // Regular Trading Hours (RTH) Check
    if (timeAsDecimal >= regularStart && timeAsDecimal < regularEnd) {
        return true;
    }

    // Extended Hours Check (Only if permitted)
    if (params_.allowExtendedHoursTrading) {
        // Pre-market: From 09:00 UTC up to the start of RTH
        if (timeAsDecimal >= preStart && timeAsDecimal < regularStart) {
            return true;
        }

        // After-hours: From end of RTH up to afterEnd (Handles UTC midnight wrap)
        if (isDST) {
            // DST: 20:00 to 00:00 (Midnight)
            if (timeAsDecimal >= 20.0 && timeAsDecimal < 24.0) return true;
        } else {
            // Non-DST: 21:00 to 01:00
            if (timeAsDecimal >= 21.0 || timeAsDecimal < 1.0) return true;
        }
    }

    // If we reach here, it's either overnight (no trade) or extended hours but flag is false
    return false;
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
Result Engine<depth, numberOfSymbols>::run(IStrategy<depth, numberOfSymbols>& strategy, std::ostream& out) {
    strategy_ = &strategy;
    strategy.setEngine(this);
    Result result = simulate(strategy);
    statistics().outputSummary(out, verbosity);
    return result;
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
bool Engine<depth, numberOfSymbols>::sufficientEquityForOrder(const NewOrder& order) {
    assert(order.orderType == OrderType::Limit || order.orderType == OrderType::Market);

    return portfolio.sufficientEquityForOrder(
        order,
        marketData->bestBids(),
        marketData->bestAsks,
        calculateOrderCost(),
        leverageFactor
    );
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
Ticks Engine<depth, numberOfSymbols>::estimateTotalOrderPrice(NewOrder order) {
    assert(order.orderType == OrderType::Limit || order.orderType == OrderType::Market);
    if (order.OrderType == OrderType::Limit) {
        return order.quantity * order.price;
    }
    else if (order.OrderType == OrderType::Market) {
        if (orderInstruction == OrderInstruction::Buy) {
            Ticks totalOrderPrice = 0;
            Quantity numberOfSharesRemaining = order.quantity;

            for (size_t level = 0; level < depth; ++level) {
                Ticks levelSize = marketData->askSize(order.symbol, level);

                if (levelSize <= numberOfSharesRemaining) {
                    numberOfSharesRemaining -= levelSize;
                    totalOrderPrice += marketData->getAsk(order.symbol, level) * levelSize;
                } else {
                    break;
                }
            }
        } else if (orderInstruction == OrderInstruction::Sell) {
            Ticks totalOrderPrice = 0;
            Quantity numberOfSharesRemaining = order.quantity;

            for (size_t level = 0; level < depth; ++level) {
                Ticks levelSize = marketData->bidSize(order.symbol, level);

                if (levelSize <= numberOfSharesRemaining) {
                    numberOfSharesRemaining -= bidSize;
                    totalOrderPrice += marketData->getBid(order.symbol, level) * levelSize;
                } else {
                    break;
                }
            }
        }
    }
    return totalOrderPrice;
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
OrderId Engine<depth, numberOfSymbols>::placeOrder(std::uint16_t symbolId,
    OrderInstruction instruction,
    OrderType orderType,
    Quantity quantity,
    TimeInForce timeInForce,
    Ticks price
) {
    NewOrder order;
    order.id = ++nextOrderId_;
    order.symbol = symbol;
    order.instruction = instruction;
    order.orderType = orderType;
    order.quantity = quantity;
    order.timeInForce = timeInForce;
    order.price = price;

    sufficentEquityForOrder = portfolio.sufficientEquityForOrder(
        marketData->bestBids(),
        marketData->bestAsks(),
        order,
        estimateTotalOrderPrice(order),
        leverageFactor
    );

    if (!sufficientEquityForOrder) {
        std::cout << "Conditions not met to place order!";
        return OrderId{0};
    }

    TimeStamp sendTime = marketData.currentTimeStamp();
    TimeStamp earliestExecution = TimeStamp(sendTime.value() + totalLatencyNs_);

    PendingOrder pendingOrder;
    pendingOrder.order = order;
    pendingOrder.sendTime = sendTime;
    pendingOrder.earliestExecution = earliestExecution;

    pendingOrders.push_back(pendingOrder);

    statistics().recordOrder(order, sendTime);

    return order.id;
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
bool Engine<depth, numberOfSymbols>::cancel(OrderId orderId) {
    // Check if order exists in pending orders
    auto it = std::find_if(pendingOrders_.begin(), pendingOrders_.end(),
        [orderId](const PendingOrder& po) { return po.order.id == orderId; });

    if (it != pendingOrders_.end()) {
        // Add cancel order with latency
        TimeStamp sendTime = marketData->currentTimeStamp();
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

template <std::size_t depth, std::uint16_t numberOfSymbols>
bool Engine<depth, numberOfSymbols>::replace(OrderId orderId, Quantity newQuantity, Ticks newPrice) {
    // Check if order exists in pending orders
    auto it = std::find_if(pendingOrders_.begin(), pendingOrders_.end(),
        [orderId](const PendingOrder& po) { return po.order.id == orderId; });

    if (it != pendingOrders_.end()) {
        // Add replace order with latency
        TimeStamp sendTime = marketData->currentTimeStamp;
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

template <std::size_t depth, std::uint16_t numberOfSymbols>
Result Engine<depth, numberOfSymbols>::simulate(IStrategy<depth, numberOfSymbols>& strategy) {
    strategy_ = &strategy;

    // Process market data
    while (marketData->nextMarketState()) {
        ++quotesProcessed_;

        // Send strategy market data
        strategy.onMarketData(marketData->currentMarketState());

        // Check margin requirements and execute margin calls if necessary
        checkMarginRequirement();

        // Try to fill orders after 'sendLatency_' + 'recieveLatency' has
        // passed since order was sent from strategy.
        processPendingOrders();

        // Send fill notifications to strategy after 'recieveLatency' time has passed since fill.
        processPendingNotifications(strategy);

        // Process settlements each morning after 9am
        processSettlements();
    }

    strategy.onEnd();

    // Update final statistics including interest owed
    statistics.updateInterestOwed(portfolio.interestOwed);

    return Result{std::move(fills_), portfolio_, quotesProcessed_};
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
double Engine<depth, numberOfSymbols>::determineFillRate(std::mt19937& rng, OrderInstruction orderInstruction) {
    // Get a value between 1 and 100 from the distribution and convert it to an integer
    switch (orderInstruction) {
        case OrderInstruction::Buy:
        {
            return std::clamp(buyFillRateDistribution(randomNumberGenerator_), 0, 100);
            break;
        }
        case OrderInstruction::Sell:
        {
            return std::clamp(sellFillRateDistribution(randomNumberGenerator_), 0, 100);
            break;
        }
    }
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
Quantity Engine<depth, numberOfSymbols>::numberOfSharesToFillForLimitOrder(
    const Quote<depth>& quote,
    OrderInstruction OrderInstruction,
    Ticks price,
    Quantity desiredNumberOfShares
) {
    int numberOfSharesAvailable = 0;

    switch (orderInstruction) {
        case OrderInstruction::Buy:
        {
            for (size_t level = 0; level < depth && numberOfSharesAvailable < desiredNumberOfShares; ++depth) {
                if (quote.getAsk(level) <= price) {
                    numberOfSharesAvailable += quote.getAskSize(level);
                } else {
                    break;
                }
            }
            break;
        }
        case OrderInstruction::Sell:
        {
            for (size_t level = 0; level < depth && numberOfSharesAvailable < desiredNumberOfShares; ++depth) {
                if (quote.getBid(level) >= price) {
                    numberOfSharesAvailable += quote.getBidSize(level);
                } else {
                    break;
                }
            }
            break;
        }
    }

    double fillRate = determineFillRate(orderInstruction);

    return Quantity{
        std::static_cast<int>(
            std::min(numberOfSharesAvailable, desiredNumberOfShares.value()) * fillRate / 100
        )
    };
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
Quantity Engine<depth, numberOfSymbols>::numberOfSharesToFillForMarketOrder(
    const Quote<depth>& quote,
    OrderInstruction OrderInstruction,
    Quantity desiredNumberOfShares
) {
    int numberOfSharesAvailable = 0;

    switch (orderInstruction) {
        case OrderInstruction::Buy:
        {
            for (size_t level = 0; level < depth && numberOfSharesAvailable < desiredNumberOfShares; ++depth) {
                numberOfSharesAvailable += quote.getAskSize(level);
            }
            break;
        }
        case OrderInstruction::Sell:
        {
            for (size_t level = 0; level < depth && numberOfSharesAvailable < desiredNumberOfShares; ++depth) {
                numberOfSharesAvailable += quote.getBidSize(level);
            }
            break;
        }
    }

    double fillRate = determineFillRate(orderInstruction);

    return Quantity{
        std::static_cast<int>(
            std::min(numberOfSharesAvailable, desiredNumberOfShares.value()) * fillRate / 100
        )
    };
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
Ticks Engine<depth, numberOfSymbols>::averageExecutionPrice(
    Quote<depth>& quote,
    Quantity numberOfShares,
    OrderInstruction orderInstruction
) {
    Ticks totalPrice = 0;
    switch (orderInstruction) {
        case OrderInstruction::Buy:
        {
            for (size_t level = 0; level < depth; ++level) {
                totalPrice += quote.getAsk(level) * quote.getAskSize(level);
            }
            break;
        }
        case OrderInstruction::Sell:
        {
            for (size_t level = 0; level < depth; ++level) {
                totalPrice += quote.getBid(level) * quote.getBidSize(level);
            }
            break;
        }
    }

    return totalPrice / numberOfShares;
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
Engine<depth, numberOfSymbols>::ExecutionResult Engine<depth, numberOfSymbols>::tryExecute(
    const NewOrder& newOrder,
    TimeStamp sendTs
) {
    Quantity numberOfSharesToFill;
    switch (orderType) {
        case OrderType::Market:
        {
            numberOfSharesToFill = numberOfSharesToFillForMarketOrder(quote, newOrder.instruction, newOrder.quantity);
        }
        case OrderType::Limit:
        {
            numberOfSharesToFill = numberOfSharesToFillForLimitOrder(quote, newOrder.instruction, newOrder.price, newOrder.quantity);
        }
    }

    Ticks averageExecutionPrice = averageExecutionPrice(quote, newOrder.quantity, newOrder.instruction);

    ExecutionResult result;
    result.fills.clear();
    result.remainingOrder = newOrder;
    result.isComplete = false;

    Fill fill;
    fill.id = newOrder.id;
    fill.symbol = newOrder.symbol;
    fill.quantity = numberOfSharesToFill;
    fill.price = averageExecutionPrice;
    fill.timestamp = quote.timestamp;
    fill.instruction = newOrder.instruction;
    fill.orderType = newOrder.orderType;
    fill.timeInForce = newOrder.timeInForce;
    fill.originalPrice = newOrder.price;

    Quantity remainingSharesUnfilled = newOrder.quantity - numberOfSharesToFill;

    if (remainingSharesUnfilled) {
        result.remainingOrder = newOrder;
        result.remainingOrder.quantity = remainingSharesUnfilled;
    }

    result.fills.push_back(fill);
    result.isComplete = (remainingSharesUnfilled == 0);

    Ticks portfolioLiquadationValue = portfolio.netLiquidationValue(marketData->bestBids(), marketData->bestAsks());

    portfolio.updatePortfolio(fill);
    statistics.recordFill(fill);
    statistics.updateStatistics(portfolioLiquidationValue);
    statistics.updateInterestOwed(portfolio().interestOwed);

    TimeStamp notificationTime = TimeStamp{fill.timestamp.value() + receiveLatencyNs_};
    notifyFill(fill, notificationTime);

    return result;
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
void Engine<depth, numberOfSymbols>::notifyFill(const Fill& fill, TimeStamp earliestNotificationTime) {
    // Add fill to results immediately
    fills.push_back(fill);

    // Queue notification for later delivery
    pendingNotifications.push_back({fill, earliestNotificationTime, false});
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
void Engine<depth, numberOfSymbols>::processPendingNotifications(IStrategy<depth, numberOfSymbols>& strategy) {
    auto it = pendingNotifications_.begin();
    while (it != pendingNotifications_.end()) {
        if (!it->delivered && marketData->timestamp >= it->earliestNotifyTime) {
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
        std::remove_if(pendingNotifications.begin(), pendingNotifications.end(),
            [](const PendingNotification& notification) { return notification.delivered; }),
        pendingNotifications_.end());
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
void Engine<depth, numberOfSymbols>::processPendingCancelOrders() {
    // Process pending cancel orders first
    auto cancelIt = pendingCancels_.begin();
    while (cancelIt != pendingCancels_.end()) {
        if (marketData->timestamp >= cancelIt->earliestExecution) {
            // Time to execute the cancel - remove the corresponding order
            auto orderIt = std::find_if(pendingOrders.begin(), pendingOrders.end(),
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
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
void Engine<depth, numberOfSymbols>::processPendingReplaceOrders() {
    // Process pending replace orders
    auto replaceIt = pendingReplaces_.begin();
    while (replaceIt != pendingReplaces_.end()) {
        if (marketData->timestamp >= replaceIt->earliestExecution) {
            // Time to execute the replace - modify the corresponding order
            auto orderIt = std::find_if(pendingOrders.begin(), pendingOrders.end(),
                [replaceIt](const PendingOrder& po) { return po.order.id == replaceIt->orderId; });

            if (orderIt != pendingOrders_.end()) {
                orderIt->order.quantity = replaceIt->newQuantity;
                orderIt->order.price = replaceIt->newPrice;
            }

            // Remove the replace order
            replaceIt = pendingReplaces.erase(replaceIt);
        } else {
            ++replaceIt;
        }
    }
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
void Engine<depth, numberOfSymbols>::processPendingBuySellOrders() {
    // Process pending orders
    auto pendingIt = pendingOrders_.begin();
    while (pendingIt != pendingOrders_.end()) {
        if (marketData->timestamp >= pendingIt->earliestExecution) {

            // Only try to execute if we are within trading hours
            if (!canTrade(currentQuote.timestamp)) {
                ++pendingIt; 
                continue; // Order stays in pendingOrders_ for the next quote
            }

            ExecutionResult result = tryExecute(pendingIt->order, pendingIt->sendTime, currentQuote_);
            if (result.isComplete) {
                // Order is complete, remove from pending
                pendingIt = pendingOrders.erase(pendingIt);
            } else {
                // Order partially filled, update remaining quantity
                pendingIt->order = result.remainingOrder;
                ++pendingIt;
            }
        } else {
            ++pendingIt;
        }
    }
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
void Engine<depth, numberOfSymbols>::processPendingOrders() {
    processPendingCancelOrders();
    processPendingCancelOrders();
    processPendingBuySellOrders();
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
void Engine<depth, numberOfSymbols>::processSettlements() {
    // Only process settlements once per day after 9am
    if (isTimeForSettlement(marketData->timestamp)) {
        // Process unsettled funds settlements
        portfolio.processSettlements(marketData->timestamp);

        // Calculate and apply daily interest on outstanding loans
        portfolio.calculateDailyInterest(marketData->timestamp);

        lastSettlementDate = marketData->timestamp;
    }
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
bool Engine<depth, numberOfSymbols>::isTimeForSettlement(TimeStamp currentTime) const {
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

template <std::size_t depth, std::uint16_t numberOfSymbols>
void Engine<depth, numberOfSymbols>::checkMarginRequirement() {
    bool inViolationOfMarginRequirement = portfolio.violatesMarginRequirement(
        marketData->bestBids(),
        marketData->bestAsks()
    );

    if (inViolationOfMarginRequirement) {
        executeMarginCall();
    }
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
void Engine<depth, numberOfSymbols>::executeMarginCall() {
    Ticks bestBids = currentMarketState_.bestBids();
    Ticks bestAsks = currentMarketState_.bestAsks();

    // Liquidate positions until we meet 30% maintenance requirement
    while (true) {
        bool inViolationOfMarginRequirement = portfolio.violatesMarginRequirement(
            marketData->bestBids(),
            marketData->bestAsks()
        );
        if (inViolationOfMarginRequirement) {
            executeMarginCall();
        }

        // Determine what to liquidate first - start with largest positions
        bool liquidated = false;

        // Liquidate long positions first (sell at bid)
        if (portfolio.longQuantity[0] > Quantity{0}) { // liquidate the the symbol with id 0
            Quantity liquidateQuantity =
                std::min(portfolio.longQuantity[0], Quantity{100});  // Liquidate in chunks

            Fill marginCallFill;
            marginCallFill.id = OrderId{0};       // Special ID for margin calls
            marginCallFill.symbol = std::uint16_t{0};  // Liquitdate the symbols with id 0
            marginCallFill.quantity = liquidateQuantity;
            marginCallFill.price = marketData->bestBid(0);
            marginCallFill.timestamp = marketData->currentTimeStamp();
            marginCallFill.instruction = OrderInstruction::Sell;
            marginCallFill.orderType = OrderType::Market;
            marginCallFill.timeInForce = TimeInForce::Day;
            marginCallFill.originalPrice = marketData->bestBid(0);

            // Update portfolio with the forced liquidation
            portfolio.updatePortfolio(marginCallFill);

            // Record the fill and queue notification
            statistics.recordFill(marginCallFill);
            TimeStamp notificationTime =
                TimeStamp{marginCallFill.timestamp.value() + receiveLatencyNs_};
            notifyFill(marginCallFill, notificationTime);

            liquidated = true;
        }
        // Liquidate short positions (buy to cover at ask)
        else if (portfolio.shortQuantity[0] > Quantity{0}) {
            Quantity liquidateQuantity =
                std::min(portfolio.shortQuantity[0], Quantity{100});  // Liquidate in chunks, liquidate the symbol with id 0

            Fill marginCallFill;
            marginCallFill.id = OrderId{0};       // Special ID for margin calls
            marginCallFill.symbol = std::uint16_t{0};  // Assume single symbol for now
            marginCallFill.quantity = liquidateQuantity;
            marginCallFill.price = marketData->bestAsk(0);
            marginCallFill.timestamp = marketData->timestamp;
            marginCallFill.instruction = OrderInstruction::Buy;  // Buy to cover short
            marginCallFill.orderType = OrderType::Market;
            marginCallFill.timeInForce = TimeInForce::Day;
            marginCallFill.originalPrice = marketData->bestAsk(0);

            // Update portfolio with the forced liquidation
            portfolio.updatePortfolio(marginCallFill);

            // Record the fill and queue notification
            statistics.recordFill(marginCallFill);
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
template class Engine<10, 1>;
template class Engine<10, 4>;

}  // namespace sim
