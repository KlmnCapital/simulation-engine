# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/src/portfolio.cpp"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/src/portfolio.cpp"

module simulation_engine;

import std;

namespace sim {

    void Portfolio::updatePortfolio(const Fill& fill) {
        Quantity qty = fill.quantity;
        Ticks price = fill.price;
        Ticks totalCost = price * qty;

        if (fill.instruction == OrderInstruction::Buy) {

            Ticks settledUsed = std::min(totalCost, settledFunds);
            Ticks marginAmount = totalCost - settledUsed;


            loan += marginAmount;


            settledFunds -= settledUsed;


            cash -= totalCost;



            if (hasSufficientSettledFunds(totalCost)) {
                addUnsettledFunds(totalCost, fill.timestamp);
            }

            if (longQuantity >= Quantity{0}) {

                if (longQuantity > 0) {

                    costBasis = ((costBasis * longQuantity) + (price * qty)) / (longQuantity + qty);
                } else {

                    costBasis = price;
                }
                longQuantity += qty;
            } else {

                Quantity closing = std::min(qty, -longQuantity);
                longQuantity += closing;

                if (longQuantity == 0) {
                    costBasis = Ticks{0};
                }

                Quantity remaining = qty - closing;
                if (remaining > 0) {

                    costBasis = price;
                    longQuantity += remaining;
                }
            }
        } else if (fill.instruction == OrderInstruction::Sell) {

            cash += totalCost;


            if (loan > Ticks{0}) {

                Ticks loanPayment = std::min(totalCost, loan);
                loan -= loanPayment;


                Ticks remainingProceeds = totalCost - loanPayment;
                if (remainingProceeds > Ticks{0}) {
                    addUnsettledFunds(remainingProceeds, fill.timestamp);
                }
            } else {

                addUnsettledFunds(totalCost, fill.timestamp);
            }

            if (longQuantity <= Quantity{0}) {

                if (longQuantity < 0) {

                    Quantity absQty = -longQuantity;
                    costBasis = ((costBasis * absQty) + (price * qty)) / (absQty + qty);
                } else {

                    costBasis = price;
                }
                longQuantity -= qty;
            } else {

                Quantity closing = std::min(qty, longQuantity);
                longQuantity -= closing;

                if (longQuantity == 0) {
                    costBasis = Ticks{0};
                }

                Quantity remaining = qty - closing;
                if (remaining > 0) {

                    costBasis = price;
                    longQuantity -= remaining;
                }
            }
        }
    }

    Ticks Portfolio::currentValue(Ticks bestBid, Ticks bestAsk) const {
        if (longQuantity > 0) {

            return cash + (bestBid * longQuantity);
        } else if (longQuantity < 0) {

            return cash + (bestAsk * longQuantity);
        } else {
            return cash;
        }
    }

    Ticks Portfolio::loanNeeded(Ticks purchaseAmount) const {
        return (purchaseAmount - cash > Ticks{0} ? purchaseAmount - cash : Ticks{0});
    }

    Ticks Portfolio::calculateMarginAmount(Ticks purchaseAmount) const {

        if (purchaseAmount > settledFunds) {
            return purchaseAmount - settledFunds;
        }
        return Ticks{0};
    }

    Ticks Portfolio::calculateSettledFundsUsed(Ticks purchaseAmount) const {

        return std::min(purchaseAmount, settledFunds);
    }

    bool Portfolio::canMakePurchase(Ticks purchaseAmount) const {


        return cash >= purchaseAmount;
    }

    void Portfolio::processSettlements(TimeStamp currentTime) {
        auto it = pendingFunds_.begin();
        while (it != pendingFunds_.end()) {
            if (it->earliestSettlement <= currentTime) {


                settledFunds += it->cash;
                it = pendingFunds_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void Portfolio::addUnsettledFunds(Ticks amount, TimeStamp currentTime) {

        constexpr TimeStamp settlementDelay{25ULL * 60 * 60 * 1000000000ULL};
        TimeStamp settlementTime = currentTime + settlementDelay;

        pendingFunds_.emplace_back(UnsettledFunds{settlementTime, amount});
    }

    bool Portfolio::hasSufficientSettledFunds(Ticks purchaseAmount) const {
        return settledFunds >= purchaseAmount;
    }

    void Portfolio::calculateDailyInterest(TimeStamp currentTime) {

        if (loan <= Ticks{0}) {
            return;
        }



        double dailyRate = static_cast<double>(interestRate) / (365.0 * 100.0);


        double interestAmount = static_cast<double>((loan + interestOwed).value()) * dailyRate;
        Ticks interestTicks{static_cast<std::int64_t>(interestAmount)};


        interestOwed += interestTicks;
    }

    Ticks Portfolio::payInterest(Ticks amount) {

        Ticks amountToPay = (amount == Ticks{0}) ? interestOwed : amount;


        amountToPay = std::min(amountToPay, interestOwed);


        amountToPay = std::min(amountToPay, settledFunds);


        settledFunds -= amountToPay;
        interestOwed -= amountToPay;

        return amountToPay;
    }

    Ticks Portfolio::longMarketValue(Ticks bestBid) const { return bestBid * longQuantity; }

    Ticks Portfolio::shortMarketValue(Ticks bestAsk) const { return bestAsk * shortQuantity; }

    Ticks Portfolio::equity(Ticks bestBid, Ticks bestAsk) const {
        Ticks longVal = longMarketValue(bestBid);
        Ticks shortVal = shortMarketValue(bestAsk);
        Ticks proceeds = shortVal;
        return cash + longVal + proceeds - loan - shortVal;
    }

    Ticks Portfolio::maintenanceRequirement(Ticks price) const {
        Ticks longVal = longMarketValue(price);
        Ticks shortVal = shortMarketValue(price);
        return (longVal + shortVal) * 3 / 10;
    }

    bool Portfolio::sufficientEquityForOrder(const NewOrder& order,
        Ticks bestBid,
        Ticks bestAsk,
        double leverageFactor) const {

        Ticks effectivePrice;
        if (order.orderType == OrderType::Limit) {
            effectivePrice = order.price;
        } else {
            if (order.instruction == OrderInstruction::Buy) {
                effectivePrice = bestAsk;
            } else {
                effectivePrice = bestBid;
            }
        }


        Ticks notional = effectivePrice * order.quantity;

        switch (order.instruction) {

            case OrderInstruction::Buy: {
                if (order.quantity <= shortQuantity) {

                    return true;
                } else {

                    Quantity extraLong = order.quantity - shortQuantity;
                    Ticks extraNotional = effectivePrice * extraLong;


                    if (cash >= extraNotional) return true;


                    Ticks currentEquity = equity(bestBid, bestAsk);
                    Ticks buyingPower = currentEquity * leverageFactor;
                    return extraNotional <= buyingPower;
                }
            }


            case OrderInstruction::Sell: {
                if (order.quantity <= longQuantity) {

                    return true;
                } else {

                    Quantity extraShort = order.quantity - longQuantity;
                    Ticks extraNotional = effectivePrice * extraShort;

                    Ticks currentEquity = equity(bestBid, bestAsk);
                    Ticks shortCapacity = currentEquity * leverageFactor;
                    return extraNotional <= shortCapacity;
                }
            }

            default:
                return false;
        }
    }

}
