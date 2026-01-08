// engine.cpp
module;
#include <cassert>

module simulation_engine;

import std;
import datetime;

namespace sim {

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
Engine<depth, numberOfSymbols, Distribution>::Engine(std::unique_ptr<IMarketData<depth, numberOfSymbols>> marketData, RunParams<Distribution> params)
    : marketData(std::move(marketData)),
      params_(params),
      statistics(params),
      portfolio(params),
      buyFillRateDistribution{params.buyFillRateDistribution},
      sellFillRateDistribution{params.sellFillRateDistribution},
      verbosityLevel{params.verbosityLevel},
      statisticsUpateRateSeconds{params.statisticsUpateRateSeconds},
      sendLatencyNs{params.sendLatencyNanoseconds},
      receiveLatencyNs{params.receiveLatencyNanoseconds},
      totalLatencyNs{params.receiveLatencyNs + params.sendLatencyNanoseconds},
      leverageFactor{params.leverageFactor} {}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
bool Engine<depth, numberOfSymbols, Distribution>::canTrade(TimeStamp currentTimeStamp) const {
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

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
Result<numberOfSymbols, Distribution> Engine<depth, numberOfSymbols, Distribution>::run(IStrategy<depth, numberOfSymbols, Distribution>& strategy, std::ostream& out) {
    strategy = &strategy;
    strategy.setEngine(this);
    Result<numberOfSymbols, Distribution> result = simulate(strategy);
    statistics().outputSummary(out, verbosityLevel);
    return result;
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
bool Engine<depth, numberOfSymbols, Distribution>::sufficientEquityForOrder(const NewOrder& order) {
    assert(order.orderType == OrderType::Limit || order.orderType == OrderType::Market);

    return portfolio.sufficientEquityForOrder(
        order,
        marketData->bestBids(),
        marketData->bestAsks,
        this->estimateTotalOrderPrice(order),
        leverageFactor
    );
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
Ticks Engine<depth, numberOfSymbols, Distribution>::estimateTotalOrderPrice(NewOrder order) {
    assert(order.orderType == OrderType::Limit || order.orderType == OrderType::Market);

    Ticks totalOrderPrice = 0;
    if (order.orderType == OrderType::Limit) {
        return order.quantity * order.price;
    } else if (order.orderType == OrderType::Market) {
        if (order.instruction == OrderInstruction::Buy) {
            Ticks totalOrderPrice = 0;
            Quantity numberOfSharesRemaining = order.quantity;

            for (std::size_t level = 0; level < depth; ++level) {
                Ticks askSize = marketData->askSize(order.symbol, level);

                if (askSize <= numberOfSharesRemaining.value()) {
                    numberOfSharesRemaining -= askSize.value();
                    totalOrderPrice += marketData->getAsk(order.symbol, level) * askSize;
                } else {
                    break;
                }
            }
        } else if (order.instruction == OrderInstruction::Sell) {
            Ticks totalOrderPrice = 0;
            Quantity numberOfSharesRemaining = order.quantity;

            for (std::size_t level = 0; level < depth; ++level) {
                Ticks bidSize = marketData->bidSize(order.symbol, level);

                if (bidSize <= numberOfSharesRemaining.value()) {
                    numberOfSharesRemaining -= bidSize.value();
                    totalOrderPrice += marketData->getBid(order.symbol, level) * bidSize;
                } else {
                    break;
                }
            }
        }
    }
    return totalOrderPrice;
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
OrderId Engine<depth, numberOfSymbols, Distribution>::placeOrder(std::uint16_t symbolId,
    OrderInstruction instruction,
    OrderType orderType,
    Quantity quantity,
    TimeInForce timeInForce,
    Ticks price
) {
    NewOrder order;
    order.id = ++nextOrderId;
    order.symbol = symbolId;
    order.instruction = instruction;
    order.orderType = orderType;
    order.quantity = quantity;
    order.timeInForce = timeInForce;
    order.price = price;

    sufficientEquityForOrder = portfolio.sufficientEquityForOrder(
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
    TimeStamp earliestExecution = TimeStamp(sendTime.value() + totalLatencyNs);

    PendingOrder pendingOrder;
    pendingOrder.order = order;
    pendingOrder.sendTime = sendTime;
    pendingOrder.earliestExecution = earliestExecution;

    pendingOrders.push_back(pendingOrder);

    statistics().recordOrder(order, sendTime);

    return order.id;
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
bool Engine<depth, numberOfSymbols, Distribution>::cancel(OrderId orderId) {
    // Check if order exists in pending orders
    auto it = std::find_if(pendingOrders.begin(), pendingOrders.end(),
        [orderId](const PendingOrder& po) { return po.order.id == orderId; });

    if (it != pendingOrders.end()) {
        // Add cancel order with latency
        TimeStamp sendTime = marketData->currentTimeStamp();
        TimeStamp earliestExecution = TimeStamp(sendTime.value() + totalLatencyNs);

        CancelOrder cancelOrder;
        cancelOrder.orderId = orderId;
        cancelOrder.sendTime = sendTime;
        cancelOrder.earliestExecution = earliestExecution;

        pendingCancels.push_back(cancelOrder);
        return true;
    }
    return false;
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
bool Engine<depth, numberOfSymbols, Distribution>::replace(OrderId orderId, Quantity newQuantity, Ticks newPrice) {
    // Check if order exists in pending orders
    auto it = std::find_if(pendingOrders.begin(), pendingOrders.end(),
        [orderId](const PendingOrder& po) { return po.order.id == orderId; });

    if (it != pendingOrders.end()) {
        // Add replace order with latency
        TimeStamp sendTime = marketData->currentTimeStamp;
        TimeStamp earliestExecution = TimeStamp(sendTime.value() + totalLatencyNs);

        ReplaceOrder replaceOrder;
        replaceOrder.orderId = orderId;
        replaceOrder.newQuantity = newQuantity;
        replaceOrder.newPrice = newPrice;
        replaceOrder.sendTime = sendTime;
        replaceOrder.earliestExecution = earliestExecution;

        pendingReplaces.push_back(replaceOrder);
        return true;
    }
    return false;
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
Result<numberOfSymbols, Distribution> Engine<depth, numberOfSymbols, Distribution>::simulate(IStrategy<depth, numberOfSymbols, Distribution>& strategy) {
    strategy = &strategy;

    // Process market data
    while (marketData->nextMarketState()) {
        ++quotesProcessed;

        // Send strategy market data
        strategy.onMarketData(marketData->currentMarketState());

        // Check margin requirements and execute margin calls if necessary
        checkMarginRequirement();

        // Try to fill orders after 'sendLatency' + 'receiveLatency' has
        // passed since order was sent from strategy.
        processPendingOrders();

        // Send fill notifications to strategy after 'receiveLatency' time has passed since fill.
        processPendingNotifications(strategy);

        // Process settlements each morning after 9am
        processSettlements();
    }

    strategy.onEnd();

    // Update final statistics including interest owed
    statistics.updateInterestOwed(portfolio.interestOwed);

    return Result{std::move(fills), portfolio, quotesProcessed};
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
double Engine<depth, numberOfSymbols, Distribution>::determineFillRate(std::mt19937& rng, OrderInstruction orderInstruction) {
    // Get a value between 1 and 100 from the distribution and convert it to an integer
    switch (orderInstruction) {
        case OrderInstruction::Buy:
        {
            return std::clamp(buyFillRateDistribution(randomNumberGenerator), 0, 100);
            break;
        }
        case OrderInstruction::Sell:
        {
            return std::clamp(sellFillRateDistribution(randomNumberGenerator), 0, 100);
            break;
        }
    }
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
Quantity Engine<depth, numberOfSymbols, Distribution>::numberOfSharesToFillForLimitOrder(
    const Quote<depth>& quote,
    OrderInstruction orderInstruction,
    Ticks price,
    Quantity desiredNumberOfShares
) {
    int numberOfSharesAvailable = 0;

    switch (orderInstruction) {
        case OrderInstruction::Buy:
        {
            for (std::size_t level = 0; level < depth && numberOfSharesAvailable < desiredNumberOfShares; ++level) {
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
            for (std::size_t level = 0; level < depth && numberOfSharesAvailable < desiredNumberOfShares; ++level) {
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
        static_cast<int>(
            std::min(numberOfSharesAvailable, static_cast<int>(desiredNumberOfShares.value())) * fillRate / 100
        )
    };
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
Quantity Engine<depth, numberOfSymbols, Distribution>::numberOfSharesToFillForMarketOrder(
    const Quote<depth>& quote,
    OrderInstruction orderInstruction,
    Quantity desiredNumberOfShares
) {
    int numberOfSharesAvailable = 0;

    switch (orderInstruction) {
        case OrderInstruction::Buy:
        {
            for (std::size_t level = 0; level < depth && numberOfSharesAvailable < desiredNumberOfShares; ++level) {
                numberOfSharesAvailable += quote.getAskSize(level);
            }
            break;
        }
        case OrderInstruction::Sell:
        {
            for (std::size_t level = 0; level < depth && numberOfSharesAvailable < desiredNumberOfShares; ++level) {
                numberOfSharesAvailable += quote.getBidSize(level);
            }
            break;
        }
    }

    double fillRate = determineFillRate(orderInstruction);

    return Quantity{
        static_cast<int>(
            std::min(numberOfSharesAvailable, static_cast<int>(desiredNumberOfShares.value())) * fillRate / 100
        )
    };
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
Ticks Engine<depth, numberOfSymbols, Distribution>::averageExecutionPrice(
    const Quote<depth>& quote,
    Quantity numberOfShares,
    OrderInstruction orderInstruction
) {
    Ticks totalPrice = 0;
    switch (orderInstruction) {
        case OrderInstruction::Buy:
        {
            for (std::size_t level = 0; level < depth; ++level) {
                totalPrice += quote.getAsk(level) * quote.getAskSize(level);
            }
            break;
        }
        case OrderInstruction::Sell:
        {
            for (std::size_t level = 0; level < depth; ++level) {
                totalPrice += quote.getBid(level) * quote.getBidSize(level);
            }
            break;
        }
    }

    return totalPrice / numberOfShares;
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
ExecutionResult Engine<depth, numberOfSymbols, Distribution>::tryExecute(
    const NewOrder& newOrder,
    TimeStamp sendTs
) {
    Quantity numberOfSharesToFill;
    const Quote<depth> quote = marketData->currentMarketState().getQuote(newOrder.symbol);

    switch (newOrder.orderType) {
        case OrderType::Market:
        {
            numberOfSharesToFill = numberOfSharesToFillForMarketOrder(quote, newOrder.instruction, newOrder.quantity);
        }
        case OrderType::Limit:
        {
            numberOfSharesToFill = numberOfSharesToFillForLimitOrder(quote, newOrder.instruction, newOrder.price, newOrder.quantity);
        }
    }

    Ticks averageExecutionPrice = averageExecutionPrice(
        marketData->getQuote(newOrder.symbol),
        newOrder.quantity, 
        newOrder.instruction
    );

    ExecutionResult result;
    result.fills.clear();
    result.remainingOrder = newOrder;
    result.isComplete = false;

    Fill fill;
    fill.id = newOrder.id;
    fill.symbol = newOrder.symbol;
    fill.quantity = numberOfSharesToFill;
    fill.price = averageExecutionPrice;
    fill.timestamp = marketData->currentTimeStamp();
    fill.instruction = newOrder.instruction;
    fill.orderType = newOrder.orderType;
    fill.timeInForce = newOrder.timeInForce;
    fill.originalPrice = newOrder.price;

    Quantity remainingSharesUnfilled = newOrder.quantity - numberOfSharesToFill;

    if (remainingSharesUnfilled.value() > 0) {
        result.remainingOrder = newOrder;
        result.remainingOrder.quantity = remainingSharesUnfilled;
    }

    result.fills.push_back(fill);
    result.isComplete = (remainingSharesUnfilled == 0);

    Ticks portfolioLiquidationValue = portfolio.netLiquidationValue(marketData->bestBids(), marketData->bestAsks());

    portfolio.updatePortfolio(fill);
    statistics.recordFill(fill);
    statistics.updateStatistics(portfolioLiquidationValue);
    statistics.updateInterestOwed(portfolio().interestOwed);

    TimeStamp notificationTime = TimeStamp{fill.timestamp.value() + receiveLatencyNs};
    notifyFill(fill, notificationTime);

    return result;
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
void Engine<depth, numberOfSymbols, Distribution>::notifyFill(const Fill& fill, TimeStamp earliestNotificationTime) {
    // Add fill to results immediately
    fills.push_back(fill);

    // Queue notification for later delivery
    pendingNotifications.push_back({fill, earliestNotificationTime, false});
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
void Engine<depth, numberOfSymbols, Distribution>::processPendingNotifications(IStrategy<depth, numberOfSymbols, Distribution>& strategy) {
    auto it = pendingNotifications.begin();
    while (it != pendingNotifications.end()) {
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
    pendingNotifications.erase(
        std::remove_if(pendingNotifications.begin(), pendingNotifications.end(),
            [](const PendingNotification& notification) { return notification.delivered; }),
        pendingNotifications.end());
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
void Engine<depth, numberOfSymbols, Distribution>::processPendingCancelOrders() {
    // Process pending cancel orders first
    auto cancelIt = pendingCancels.begin();
    while (cancelIt != pendingCancels.end()) {
        if (marketData->timestamp >= cancelIt->earliestExecution) {
            // Time to execute the cancel - remove the corresponding order
            auto orderIt = std::find_if(pendingOrders.begin(), pendingOrders.end(),
                [cancelIt](const PendingOrder& po) { return po.order.id == cancelIt->orderId; });

            if (orderIt != pendingOrders.end()) {
                pendingOrders.erase(orderIt);
            }

            // Remove the cancel order
            cancelIt = pendingCancels.erase(cancelIt);
        } else {
            ++cancelIt;
        }
    }
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
void Engine<depth, numberOfSymbols, Distribution>::processPendingReplaceOrders() {
    // Process pending replace orders
    auto replaceIt = pendingReplaces.begin();
    while (replaceIt != pendingReplaces.end()) {
        if (marketData->timestamp >= replaceIt->earliestExecution) {
            // Time to execute the replace - modify the corresponding order
            auto orderIt = std::find_if(pendingOrders.begin(), pendingOrders.end(),
                [replaceIt](const PendingOrder& po) { return po.order.id == replaceIt->orderId; });

            if (orderIt != pendingOrders.end()) {
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

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
void Engine<depth, numberOfSymbols, Distribution>::processPendingBuySellOrders() {
    // Process pending orders
    auto pendingIt = pendingOrders.begin();
    while (pendingIt != pendingOrders.end()) {
        if (marketData->timestamp >= pendingIt->earliestExecution) {

            // Only try to execute if we are within trading hours
            if (!canTrade(marketData.currentTimeStamp())) {
                ++pendingIt; 
                continue; // Order stays in pendingOrders for the next quote
            }

            ExecutionResult result = tryExecute(pendingIt->order, pendingIt->sendTime);
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

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
void Engine<depth, numberOfSymbols, Distribution>::processPendingOrders() {
    processPendingCancelOrders();
    processPendingCancelOrders();
    processPendingBuySellOrders();
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
void Engine<depth, numberOfSymbols, Distribution>::processSettlements() {
    // Only process settlements once per day after 9am
    if (isTimeForSettlement(marketData->timestamp)) {
        // Process unsettled funds settlements
        portfolio.processSettlements(marketData->timestamp);

        // Calculate and apply daily interest on outstanding loans
        portfolio.calculateDailyInterest(marketData->timestamp);

        lastSettlementDate = marketData->timestamp;
    }
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
bool Engine<depth, numberOfSymbols, Distribution>::isTimeForSettlement(TimeStamp currentTime) const {
    // Convert timestamp to days since epoch (assuming nanoseconds since epoch)
    constexpr std::uint64_t nanosecondsPerDay = 24ULL * 60 * 60 * 1000000000ULL;
    constexpr std::uint64_t nanosecondsPerHour = 60ULL * 60 * 1000000000ULL;

    std::uint64_t currentDay = currentTime.value() / nanosecondsPerDay;
    std::uint64_t lastSettlementDay = lastSettlementDate.value() / nanosecondsPerDay;

    // Check if it's a new day and after 9am
    if (currentDay > lastSettlementDay) {
        std::uint64_t timeInDay = currentTime.value() % nanosecondsPerDay;
        std::uint64_t nineAM = 9ULL * nanosecondsPerHour;  // 9am in nanoseconds

        return timeInDay >= nineAM;
    }

    return false;
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
void Engine<depth, numberOfSymbols, Distribution>::checkMarginRequirement() {
    bool inViolationOfMarginRequirement = portfolio.violatesMarginRequirement(
        marketData->bestBids(),
        marketData->bestAsks()
    );

    if (inViolationOfMarginRequirement) {
        executeMarginCall();
    }
}

template<std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
void Engine<depth, numberOfSymbols, Distribution>::executeMarginCall() {
    Ticks bestBids = marketData->currentMarketState().bestBids();
    Ticks bestAsks = marketData->currentMarketState().bestAsks();

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
                TimeStamp{marginCallFill.timestamp.value() + receiveLatencyNs};
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
                TimeStamp{marginCallFill.timestamp.value() + receiveLatencyNs};
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
// TODO:
//template class Engine<10, 1>;
//template class Engine<10, 4>;

}  // namespace sim
