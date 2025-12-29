// portfolio.cppm
export module simulation_engine:portfolio;

import :order_placement;
import :types;

import std;

export namespace sim {

    /**
    * @brief Portfolio tracking structure
    * @details
    * Tracks the current state of a trading portfolio including cash, position quantity,
    * and cost basis. Provides methods to update the portfolio with fills and calculate
    * current portfolio value.
    *
    * The portfolio maintains:
    * - Cash: Available cash for trading
    * - LongQuantity: Current long position size (positive = long, negative = short)
    * - Cost Basis: Weighted average price of the current position
    *
    * Tracks the average price and quantity for the symbol,
    * allowing for accurate P&L calculations and risk management.
    */
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

        /**
        * @brief Update portfolio with a fill
        * @param fill The fill to process
        */
        void updatePortfolio(const Fill& fill);

        /**
        * @brief Calculate current portfolio value
        * @param bestBid Current best bid price
        * @param bestAsk Current best ask price
        * @return Total portfolio value including cash and position value
        */
        Ticks currentValue(Ticks bestBid, Ticks bestAsk) const;

        Ticks loanNeeded(Ticks purchaseAmount) const;

        /**
        * @brief Calculate how much of a purchase will be on margin
        * @param purchaseAmount Total amount needed for purchase
        * @return Amount that will be borrowed (on margin)
        */
        Ticks calculateMarginAmount(Ticks purchaseAmount) const;

        /**
        * @brief Calculate how much settled funds will be used for purchase
        * @param purchaseAmount Total amount needed for purchase
        * @return Amount that will be paid from settled funds
        */
        Ticks calculateSettledFundsUsed(Ticks purchaseAmount) const;

        /**
        * @brief Check if purchase can be made with available funds
        * @param purchaseAmount Total amount needed for purchase
        * @return True if purchase can be made (either with settled funds or margin)
        */
        bool canMakePurchase(Ticks purchaseAmount) const;

        /**
        * @brief Pay interest from settled funds
        * @param amount Amount of interest to pay (if 0, pays all interest)
        * @return Amount actually paid
        */
        Ticks payInterest(Ticks amount = Ticks{0});

        /**
        * @brief Calculate and apply daily interest on outstanding loans
        * @param currentTime Current simulation time
        */
        void calculateDailyInterest(TimeStamp currentTime);

        /**
        * @brief Process unsettled funds settlement at current time
        * @param currentTime Current simulation time
        */
        void processSettlements(TimeStamp currentTime);

        /**
        * @brief Add unsettled funds with 36-hour settlement
        * @param amount Amount to add as unsettled funds
        * @param currentTime Current simulation time
        */
        void addUnsettledFunds(Ticks amount, TimeStamp currentTime);

        /**
        * @brief Check if we have sufficient settled funds for a purchase
        * @param purchaseAmount Amount needed for purchase
        * @return True if we have enough settled funds
        */
        bool hasSufficientSettledFunds(Ticks purchaseAmount) const;

        Ticks longMarketValue(Ticks bestBid) const;

        Ticks shortMarketValue(Ticks bestAsk) const;

        Ticks equity(Ticks bestBid, Ticks bestAsk) const;

        Ticks maintenanceRequirement(Ticks price) const;

        /**
        * @brief Check if portfolio has sufficient equity for an order
        * @param order The order to validate
        * @param bestBid Current best bid price
        * @param bestAsk Current best ask price
        * @param leverageFactor Leverage factor for margin calculations
        * @return True if portfolio can support the order
        */
        bool sufficientEquityForOrder(const NewOrder& order,
            Ticks bestBid,
            Ticks bestAsk,
            double leverageFactor) const;

    private:
        /**
        * @brief Update cost basis with weighted average calculation
        * @param fillPrice Price of the new fill
        * @param fillQuantity Quantity of the new fill
        */
        void updateCostBasis(Ticks fillPrice, Quantity fillQuantity);
    };

}  // namespace sim
