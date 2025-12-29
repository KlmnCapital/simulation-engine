// portfolio.cpp
module simulation_engine;

import std;

namespace sim {

    void Portfolio::updatePortfolio(const Fill& fill) {
        Quantity qty = fill.quantity;
        Ticks price = fill.price;
        Ticks totalCost = price * qty;

        if (fill.instruction == OrderInstruction::Buy) {
            // Calculate how much of this purchase is on margin vs settled funds
            Ticks settledUsed = std::min(totalCost, settledFunds);
            Ticks marginAmount = totalCost - settledUsed;

            // Update loan amount for the margin portion
            loan += marginAmount;

            // Reduce settled funds by the amount used
            settledFunds -= settledUsed;

            // Always subtract from total cash
            cash -= totalCost;

            // If we have sufficient settled funds, add to unsettled funds
            // Otherwise, the purchase uses unsettled funds directly
            if (hasSufficientSettledFunds(totalCost)) {
                addUnsettledFunds(totalCost, fill.timestamp);
            }

            if (longQuantity >= Quantity{0}) {
                // Already long or flat → add more long
                if (longQuantity > 0) {
                    // extend long → update cost basis
                    costBasis = ((costBasis * longQuantity) + (price * qty)) / (longQuantity + qty);
                } else {
                    // flat → new long
                    costBasis = price;
                }
                longQuantity += qty;
            } else {
                // Currently short → buy reduces short
                Quantity closing = std::min(qty, -longQuantity);
                longQuantity += closing;  // closer to 0

                if (longQuantity == 0) {
                    costBasis = Ticks{0};  // flat
                }

                Quantity remaining = qty - closing;
                if (remaining > 0) {
                    // crossed to new long
                    costBasis = price;
                    longQuantity += remaining;
                }
            }
        } else if (fill.instruction == OrderInstruction::Sell) {
            // Always add to total cash
            cash += totalCost;

            // When selling positions bought on margin, use proceeds to pay down the loan first
            if (loan > Ticks{0}) {
                // Calculate how much of the proceeds to use for loan repayment
                Ticks loanPayment = std::min(totalCost, loan);
                loan -= loanPayment;

                // Only add remaining proceeds to unsettled funds
                Ticks remainingProceeds = totalCost - loanPayment;
                if (remainingProceeds > Ticks{0}) {
                    addUnsettledFunds(remainingProceeds, fill.timestamp);
                }
            } else {
                // No outstanding loan, add all proceeds to unsettled funds
                addUnsettledFunds(totalCost, fill.timestamp);
            }

            if (longQuantity <= Quantity{0}) {
                // Already short or flat → add more short
                if (longQuantity < 0) {
                    // extend short → update cost basis
                    Quantity absQty = -longQuantity;
                    costBasis = ((costBasis * absQty) + (price * qty)) / (absQty + qty);
                } else {
                    // flat → new short
                    costBasis = price;
                }
                longQuantity -= qty;
            } else {
                // Currently long → sell reduces long
                Quantity closing = std::min(qty, longQuantity);
                longQuantity -= closing;

                if (longQuantity == 0) {
                    costBasis = Ticks{0};  // flat
                }

                Quantity remaining = qty - closing;
                if (remaining > 0) {
                    // crossed to new short
                    costBasis = price;
                    longQuantity -= remaining;
                }
            }
        }
    }

    Ticks Portfolio::currentValue(Ticks bestBid, Ticks bestAsk) const {
        if (longQuantity > 0) {
            // Long valued at bid
            return cash + (bestBid * longQuantity);
        } else if (longQuantity < 0) {
            // Short valued at ask
            return cash + (bestAsk * longQuantity);  // longQuantity is negative
        } else {
            return cash;
        }
    }

    Ticks Portfolio::loanNeeded(Ticks purchaseAmount) const {
        return (purchaseAmount - cash > Ticks{0} ? purchaseAmount - cash : Ticks{0});
    }

    Ticks Portfolio::calculateMarginAmount(Ticks purchaseAmount) const {
        // Margin amount is the difference between purchase amount and available settled funds
        if (purchaseAmount > settledFunds) {
            return purchaseAmount - settledFunds;
        }
        return Ticks{0};
    }

    Ticks Portfolio::calculateSettledFundsUsed(Ticks purchaseAmount) const {
        // Use settled funds up to the purchase amount, or all settled funds if purchase is larger
        return std::min(purchaseAmount, settledFunds);
    }

    bool Portfolio::canMakePurchase(Ticks purchaseAmount) const {
        // Can make purchase if we have enough total cash (settled + unsettled)
        // This allows for margin trading as long as total cash is sufficient
        return cash >= purchaseAmount;
    }

    void Portfolio::processSettlements(TimeStamp currentTime) {
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

    void Portfolio::addUnsettledFunds(Ticks amount, TimeStamp currentTime) {
        // 36 hours = 36 * 60 * 60 * 1e9 nanoseconds (assuming TimeStamp is in nanoseconds)
        constexpr TimeStamp settlementDelay{25ULL * 60 * 60 * 1000000000ULL};
        TimeStamp settlementTime = currentTime + settlementDelay;

        pendingFunds_.emplace_back(UnsettledFunds{settlementTime, amount});
    }

    bool Portfolio::hasSufficientSettledFunds(Ticks purchaseAmount) const {
        return settledFunds >= purchaseAmount;
    }

    void Portfolio::calculateDailyInterest(TimeStamp currentTime) {
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

    Ticks Portfolio::payInterest(Ticks amount) {
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

    Ticks Portfolio::longMarketValue(Ticks bestBid) const { return bestBid * longQuantity; }

    Ticks Portfolio::shortMarketValue(Ticks bestAsk) const { return bestAsk * shortQuantity; }

    Ticks Portfolio::equity(Ticks bestBid, Ticks bestAsk) const {
        Ticks longVal = longMarketValue(bestBid);
        Ticks shortVal = shortMarketValue(bestAsk);
        Ticks proceeds = shortVal;  // proceeds are held as collateral
        return cash + longVal + proceeds - loan - shortVal;
    }

    Ticks Portfolio::maintenanceRequirement(Ticks price) const {
        Ticks longVal = longMarketValue(price);
        Ticks shortVal = shortMarketValue(price);
        return (longVal + shortVal) * 3 / 10;  // 30%
    }

    bool Portfolio::sufficientEquityForOrder(const NewOrder& order,
        Ticks bestBid,
        Ticks bestAsk,
        double leverageFactor) const {
        // Determine effective price
        Ticks effectivePrice;
        if (order.orderType == OrderType::Limit) {
            effectivePrice = order.price;
        } else {  // Market order
            if (order.instruction == OrderInstruction::Buy) {
                effectivePrice = bestAsk;  // buy at ask
            } else {
                effectivePrice = bestBid;  // sell at bid
            }
        }

        // Calculate notional for this order
        Ticks notional = effectivePrice * order.quantity;

        switch (order.instruction) {
            // -------- BUY --------
            case OrderInstruction::Buy: {
                if (order.quantity <= shortQuantity) {
                    // Just covering shorts
                    return true;
                } else {
                    // Cover shorts, plus open new long
                    Quantity extraLong = order.quantity - shortQuantity;
                    Ticks extraNotional = effectivePrice * extraLong;

                    // Cash first
                    if (cash >= extraNotional) return true;

                    // Otherwise check leverage
                    Ticks currentEquity = equity(bestBid, bestAsk);
                    Ticks buyingPower = currentEquity * leverageFactor;
                    return extraNotional <= buyingPower;
                }
            }

            // -------- SELL --------
            case OrderInstruction::Sell: {
                if (order.quantity <= longQuantity) {
                    // Just selling longs
                    return true;
                } else {
                    // Sell longs, plus open new short
                    Quantity extraShort = order.quantity - longQuantity;
                    Ticks extraNotional = effectivePrice * extraShort;

                    Ticks currentEquity = equity(bestBid, bestAsk);
                    Ticks shortCapacity = currentEquity * leverageFactor;
                    return extraNotional <= shortCapacity;
                }
            }

            default:
                return false;  // safety
        }
    }

}  // namespace sim
