# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/portfolio.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/portfolio.cppm"

export module simulation_engine:portfolio;

import :order_placement;
import :types;

import std;

export namespace sim {
# 26 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/portfolio.cppm"
    struct Portfolio {
        Ticks cash{0};
        Ticks settledFunds{0};
        Quantity longQuantity{0};
        Quantity shortQuantity{0};
        Ticks costBasis{0};
        Ticks loan{0};
        Ticks interestOwed{0};
        Percentage interestRate{0};
        std::vector<UnsettledFunds> pendingFunds_;





        void updatePortfolio(const Fill& fill);







        Ticks currentValue(Ticks bestBid, Ticks bestAsk) const;

        Ticks loanNeeded(Ticks purchaseAmount) const;






        Ticks calculateMarginAmount(Ticks purchaseAmount) const;






        Ticks calculateSettledFundsUsed(Ticks purchaseAmount) const;






        bool canMakePurchase(Ticks purchaseAmount) const;






        Ticks payInterest(Ticks amount = Ticks{0});





        void calculateDailyInterest(TimeStamp currentTime);





        void processSettlements(TimeStamp currentTime);






        void addUnsettledFunds(Ticks amount, TimeStamp currentTime);






        bool hasSufficientSettledFunds(Ticks purchaseAmount) const;

        Ticks longMarketValue(Ticks bestBid) const;

        Ticks shortMarketValue(Ticks bestAsk) const;

        Ticks equity(Ticks bestBid, Ticks bestAsk) const;

        Ticks maintenanceRequirement(Ticks price) const;
# 123 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/portfolio.cppm"
        bool sufficientEquityForOrder(const NewOrder& order,
            Ticks bestBid,
            Ticks bestAsk,
            double leverageFactor) const;

    private:





        void updateCostBasis(Ticks fillPrice, Quantity fillQuantity);
    };

}
