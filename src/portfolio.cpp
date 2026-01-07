// portfolio.cpp
module simulation_engine;

import std;

namespace sim {

template<std::uint16_t numberOfSymbols>
void Portfolio<numberOfSymbols>::updatePortfolio(const Fill& fill) {
    const std::uint16_t symbolId = fill.symbol;
    const Quantity fillQuantity = fill.quantity;
    const Ticks fillPrice = fill.price;
    const Ticks totalNotional = fillPrice * fillQuantity;

    if (fill.instruction == OrderInstruction::Buy) {
        // A Buy consumes cash immediately.
        const Ticks settledFundsUsed = std::min(totalNotional, settledFunds);
        const Ticks loanIncrease = totalNotional - settledFundsUsed;

        settledFunds -= settledFundsUsed;
        loan += loanIncrease;
        cash -= totalNotional;

        // Does this buy cover an existing short?
        Quantity quantityToCover = std::min(fillQuantity, shortQuantity[symbolId]);
        Quantity quantityToOpen = fillQuantity - quantityToCover;

        if (quantityToCover > 0) {
            shortQuantity[symbolId] -= quantityToCover;
            // If we fully closed the short, reset cost basis for that symbol
            if (shortQuantity[symbolId] == 0 && quantityToOpen == 0) {
                costBasis[symbolId] = Ticks{0};
            }
        }

        if (quantityToOpen > 0) {
            // Updating cost basis for the new/extended long position
            updateCostBasis(symbolId, fillPrice, quantityToOpen);
            longQuantity[symbolId] += quantityToOpen;
        }

    } else if (fill.instruction == OrderInstruction::Sell) {
        // Proceeds from a sell pay down the loan first, then go to unsettled funds.
        cash += totalNotional;

        const Ticks loanRepayment = std::min(totalNotional, loan);
        loan -= loanRepayment;

        const Ticks remainingProceeds = totalNotional - loanRepayment;
        if (remainingProceeds > 0) {
            addUnsettledFunds(remainingProceeds, fill.timestamp);
        }

        // Does this sell cover an existing long?
        Quantity quantityToCover = std::min(fillQuantity, longQuantity[symbolId]);
        Quantity quantityToOpen = fillQuantity - quantityToCover;

        if (quantityToCover > 0) {
            longQuantity[symbolId] -= quantityToCover;
            // If we fully closed the long, reset cost basis
            if (longQuantity[symbolId] == 0 && quantityToOpen == 0) {
                costBasis[symbolId] = Ticks{0};
            }
        }

        if (quantityToOpen > 0) {
            // Updating cost basis for the new/extended short position
            updateCostBasis(symbolId, fillPrice, quantityToOpen);
            shortQuantity[symbolId] += quantityToOpen;
        }
    }
}


template<std::uint16_t numberOfSymbols>
Ticks Portfolio<numberOfSymbols>::grossMarketValue(const std::array<Ticks, numberOfSymbols>& bestBids, const std::array<Ticks, numberOfSymbols>& bestAsks) const {
    Ticks currentLongMarketValue = longMarketValue(bestBids);
    Ticks currentShortMarketValue = shortMarketValue(bestAsks);

    return currentLongMarketValue + currentShortMarketValue;
}

template<std::uint16_t numberOfSymbols>
Ticks Portfolio<numberOfSymbols>::netMarketValue(const std::array<Ticks, numberOfSymbols>& bestBids, const std::array<Ticks, numberOfSymbols>& bestAsks) const {
    Ticks currentLongMarketValue = longMarketValue(bestBids);
    Ticks currentShortMarketValue = shortMarketValue(bestAsks);

    return currentLongMarketValue - currentShortMarketValue;
}

template<std::uint16_t numberOfSymbols>
Ticks Portfolio<numberOfSymbols>::netLiquidationValue(const std::array<Ticks, numberOfSymbols>& bestBids, const std::array<Ticks, numberOfSymbols>& bestAsks) const {
    Ticks currentNetMarketValue = netMarketValue(bestBids, bestAsks);

    return cash + currentNetMarketValue - (loan + interestOwed);
}

template<std::uint16_t numberOfSymbols>
Ticks Portfolio<numberOfSymbols>::longMarketValue(std::array<Ticks, numberOfSymbols>& bestBids) const {
    // Perform inner produce on bestBids and longQuantity to get the marketValue of the long positions
    // Note: This could be more accurate by accounting for numher of shares at each level, but since 
    // this method is not used in the core logic of fills, we simplify it here.
    return std::transform_reduce(
        std::execution::unseq, // Hint to use SIMD
        longQuantity.begin(), longQuantity.end(),
        bestBids.begin(),
        Ticks{0}
    );
}

template<std::uint16_t numberOfSymbols>
Ticks Portfolio<numberOfSymbols>::shortMarketValue(std::array<Ticks, numberOfSymbols>& bestAsks) const { 
    // Perform inner produce on bestBids and longQuantity to get the marketValue of the long positions
    // Note: This could be more accurate by accounting for numher of shares at each level, but since 
    // this method is not used in the core logic of fills, we simplify it here.
    return std::transform_reduce(
        std::execution::unseq, // Hint to use SIMD
        shortQuantity.begin(), shortQuantity.end(),
        bestAsks.begin(),
        Ticks{0}
    );
}


template<std::uint16_t numberOfSymbols>
Ticks Portfolio<numberOfSymbols>::loanNeeded(Ticks purchaseAmount) const {
    return (purchaseAmount - cash > Ticks{0} ? purchaseAmount - cash : Ticks{0});
}

template<std::uint16_t numberOfSymbols>
Ticks Portfolio<numberOfSymbols>::calculateMarginAmount(Ticks purchaseAmount) const {
    // Margin amount is the difference between purchase amount and available settled funds
    if (purchaseAmount > settledFunds) {
        return purchaseAmount - settledFunds;
    }
    return Ticks{0};
}

template<std::uint16_t numberOfSymbols>
Ticks Portfolio<numberOfSymbols>::calculateSettledFundsUsed(Ticks purchaseAmount) const {
    // Use settled funds up to the purchase amount, or all settled funds if purchase is larger
    return std::min(purchaseAmount, settledFunds);
}

template<std::uint16_t numberOfSymbols>
bool Portfolio<numberOfSymbols>::canMakePurchase(Ticks purchaseAmount) const {
    // Can make purchase if we have enough total cash (settled + unsettled)
    // This allows for margin trading as long as total cash is sufficient
    if (loan) {
        return cash >= purchaseAmount;
    }
    return settledFunds >= purchaseAmount;
}

template<std::uint16_t numberOfSymbols>
bool Portfolio<numberOfSymbols>::hasSufficientSettledFunds(Ticks purchaseAmount) const {
    return settledFunds >= purchaseAmount;
}

template<std::uint16_t numberOfSymbols>
void Portfolio<numberOfSymbols>::calculateDailyInterest(TimeStamp currentTime) {
    // Only calculate interest if there's an outstanding loan
    if (loan <= Ticks{0}) {
        return;
    }

    // Calculate daily interest rate (assuming interestRate is annual percentage)
    // Convert annual rate to daily rate: daily_rate = annual_rate / 365
    double dailyRate = static_cast<double>(interestRate) / (365.0 * 100.0);

    // Calculate interest amount based on loan principal plus accrued interest (compound interest)
    double interestAmount = static_cast<double>((loan + interestOwed).value()) * dailyRate;
    Ticks interestTicks{static_cast<std::int64_t>(interestAmount)};

    // Add interest to the interestOwed field
    interestOwed += interestTicks;
}

template<std::uint16_t numberOfSymbols>
Ticks Portfolio<numberOfSymbols>::maintenanceRequirement(const std::array<Ticks, numberOfSymbols>& bestBids, const std::array<Ticks, numberOfSymbols>& bestAsks) const {
    Ticks grossMarketValue = grossMarketValue(bestBids, bestAsks);
    return grossMarketValue * 3 / 10;  // 30%
}

template<std::uint16_t numberOfSymbols>
Ticks Portfolio<numberOfSymbols>::payInterest(Ticks amount) {
    // If amount is 0, pay all interest owed
    Ticks amountToPay = (amount == Ticks{0}) ? interestOwed : amount;

    // Don't pay more than what's owed
    amountToPay = std::min(amountToPay, interestOwed);

    // Don't pay more than available settled funds
    amountToPay = std::min(amountToPay, settledFunds);

    // Deduct from settled funds and interest owed
    settledFunds -= amountToPay;
    interestOwed -= amountToPay;

    return amountToPay;
}

template<std::uint16_t numberOfSymbols>
void Portfolio<numberOfSymbols>::addUnsettledFunds(Ticks amount, TimeStamp currentTime) {
    // 36 hours = 36 * 60 * 60 * 1e9 nanoseconds (assuming TimeStamp is in nanoseconds)
    constexpr TimeStamp settlementDelay{25ULL * 60 * 60 * 1000000000ULL};
    TimeStamp settlementTime = currentTime + settlementDelay;

    pendingFunds_.emplace_back(UnsettledFunds{settlementTime, amount});
}

template<std::uint16_t numberOfSymbols>
void Portfolio<numberOfSymbols>::processSettlements(TimeStamp currentTime) {
    auto it = pendingFunds_.begin();
    while (it != pendingFunds_.end()) {
        if (it->earliestSettlement <= currentTime) {
            // Move from unsettled to settled funds
            // Note: total cash remains unchanged, just reclassifying from unsettled to settled
            settledFunds += it->cash;
            it = pendingFunds_.erase(it);
        } else {
            ++it;
        }
    }
}

template<std::uint16_t numberOfSymbols>
bool Portfolio<numberOfSymbols>::violatesMarginRequirement(const std::array<Ticks, numberOfSymbols>& bestBids, const std::array<Ticks, numberOfSymbols>& bestAsks) const {
    Ticks currentEquity = netLiquidationValue(bestBids, bestAsks);
    Ticks mainenanceReq = maintenanceRequirement(bestBids, bestAsks);
    if (currentEquity < maintenanceReq) {
        return true;
    }
    return false;
}

template<std::uint16_t numberOfSymbols>
bool Portfolio<numberOfSymbols>::sufficientEquityForOrder(
    const std::array<Ticks, numberOfSymbols>& bestBids,
    const std::array<Ticks, numberOfSymbols>& bestAsks,
    const NewOrder& order, 
    Ticks totalOrderPrice, 
    double leverageFactor) const 
{
    const std::uint16_t symbolId = order.symbol;

    // Calculate Current State
    const Ticks currentNetLiquidationValue = netLiquidationValue(bestBids, bestAsks);
    const Ticks currentGrossMarketValue = grossMarketValue(bestBids, bestAsks);

    // Quantity Netting
    const Quantity totalQuantity = order.quantity;
    const Quantity closingQuantity = (order.instruction == OrderInstruction::Buy) 
                                     ? std::min(totalQuantity, shortQuantity[symbolId]) 
                                     : std::min(totalQuantity, longQuantity[symbolId]);
    const Quantity openingQuantity = totalQuantity - closingQuantity;

    // Exposure Impact
    const double openingRatio = static_cast<double>(openingQuantity.value()) / totalQuantity.value();
    const double closingRatio = static_cast<double>(closingQuantity.value()) / totalQuantity.value();

    const double addedExposure = openingRatio * static_cast<double>(totalOrderPrice.value());
    const double reducedExposure = closingRatio * static_cast<double>(totalOrderPrice.value());

    // New Exposure = Old Exposure + New Position - Closed Position
    const double projectedGrossMarketValue = static_cast<double>(currentGrossMarketValue.value()) 
                                             + addedExposure 
                                             - reducedExposure;

    return (currentNetLiquidationValue > Ticks{0}) && 
           (projectedGrossMarketValue <= (static_cast<double>(currentNetLiquidationValue.value()) * leverageFactor));
}

}  // namespace sim
