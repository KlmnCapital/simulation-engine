// portfolio.cppm
export module simulation_engine:portfolio;

import :probability_distributions;
import :order_placement;
import :types;
import :run_params;

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
template <std::uint16_t numberOfSymbols, typename Distribution>
class Portfolio {
   public:
    Portfolio(RunParams<Distribution> runParams) {
        cash = runParams.startingCash;
        settledFunds = runParams.startingCash;
        interestRate = runParams.interestRate;
    }

    Ticks cash{0};
    Ticks settledFunds{0};

    // The nth entry represents the value for the symbol with id n.
    std::array<Quantity, numberOfSymbols> longQuantity{Quantity{0}};
    std::array<Quantity, numberOfSymbols> shortQuantity{Quantity{0}};

    std::array<Ticks, numberOfSymbols> costBasis{Ticks{0}};  //
    Ticks loan{0};
    Ticks interestOwed{0};
    Percentage interestRate{0};
    std::vector<UnsettledFunds> pendingFunds_;

    /**
     * @brief Update the portfolio state following a trade execution (fill).
     * @details Updates quantities, adjusts cash balances, calculates new cost bases,
     * and manages margin loan increases or repayments based on the trade direction.
     * @param fill The execution report containing symbol, side, quantity, and price.
     */
    void updatePortfolio(const Fill& fill);

    /**
     * @brief Determine the amount of additional borrowing required to fund a purchase.
     * @details Compares the purchase cost against available total cash to find the shortfall.
     * @param purchaseAmount The total notional value of the intended purchase.
     * @return The amount that must be added to the margin loan if the purchase proceeds.
     */
    Ticks loanNeeded(Ticks purchaseAmount) const;

    /**
     * @brief Calculate the portion of a purchase that must be covered by margin.
     * @details Specifically checks the shortfall relative to 'settled' funds rather than total
     * cash.
     * @param purchaseAmount The total cost of the position to be opened.
     * @return The specific amount of the purchase that will be classified as a margin debt.
     */
    Ticks calculateMarginAmount(Ticks purchaseAmount) const;

    /**
     * @brief Calculate how much of the existing settled cash will be consumed by a purchase.
     * @details Used to determine the impact on the immediate liquidity of the account.
     * @param purchaseAmount The total cost of the transaction.
     * @return The amount of settledFunds that will be subtracted (capped by current settledFunds).
     */
    Ticks calculateSettledFundsUsed(Ticks purchaseAmount) const;

    /**
     * @brief Evaluate if the account has enough buying power to execute a purchase.
     * @details Checks if total cash (settled + unsettled) plus available margin capacity
     * is greater than or equal to the purchase cost.
     * @param purchaseAmount The total notional cost of the buy order.
     * @return True if the account can technically fund the transaction.
     */
    bool canMakePurchase(Ticks purchaseAmount) const;

    /**
     * @brief Pay accrued interest on margin loans using settled funds.
     * @param amount The specific amount to pay. If 0, attempts to pay the full interestOwed.
     * @return The actual amount of interest successfully paid (capped by available settled funds).
     */
    Ticks payInterest(Ticks amount = Ticks{0});

    /**
     * @brief Calculate and accrue daily interest on the outstanding margin loan.
     * @details Compounds interest based on the annual interestRate divided by 365.
     * @param currentTime The current simulation timestamp used to track the last interest accrual.
     */
    void calculateDailyInterest(TimeStamp currentTime);

    /**
     * @brief Iterate through pending unsettled funds and move matured amounts to settled funds.
     * @details Checks if the settlement delay (e.g., T+2) has passed for each pending credit.
     * @param currentTime The current simulation time to compare against settlement maturity.
     */
    void processSettlements(TimeStamp currentTime);

    /**
     * @brief Schedule a cash credit to be available after the standard settlement delay.
     * @details Typically called after a Sell order to simulate the delay in receiving cash from the
     * exchange.
     * @param amount The amount of cash to be credited upon settlement.
     * @param currentTime The current simulation time used to calculate the future settlement date.
     */
    void addUnsettledFunds(Ticks amount, TimeStamp currentTime);

    /**
     * @brief Verify if the cash balance already settled (T+0) is enough to cover a purchase.
     * @param purchaseAmount Total cost of the transaction including fees.
     * @return True if settledFunds >= purchaseAmount.
     */
    bool hasSufficientSettledFunds(Ticks purchaseAmount) const;

    /**
     * @brief Calculate the total market value of all long positions.
     * @details Sums (Quantity * Bid Price) for all symbols where quantity is positive.
     * @param bestBids An array of the current best bid prices for all symbols in the universe.
     * @return The gross value of all owned assets.
     */
    Ticks longMarketValue(const std::array<Ticks, numberOfSymbols>& bestBids) const;

    /**
     * @brief Calculate the total market value of all short positions.
     * @details Sums (ABS(Quantity) * Ask Price) for all symbols where quantity is negative.
     * @param bestAsks An array of the current best ask prices (cost to cover) for all symbols.
     * @return The absolute cost required to buy back all shorted shares.
     */
    Ticks shortMarketValue(const std::array<Ticks, numberOfSymbols>& bestAsks) const;

    /**
     * @brief Calculate the total absolute exposure of the portfolio.
     * @details Often called Gross Market Value (GMV). Sums |Longs| + |Shorts|.
     * @param bestBids Current best bid prices.
     * @param bestAsks Current best ask prices.
     * @return Total market footprint used for leverage and risk limit calculations.
     */
    Ticks grossMarketValue(const std::array<Ticks, numberOfSymbols>& bestBids,
        const std::array<Ticks, numberOfSymbols>& bestAsks) const;

    /**
     * @brief Calculate the net directional exposure of the portfolio.
     * @details Often called Net Market Value (NMV). Sums Longs - Shorts.
     * @param bestBids Current best bid prices.
     * @param bestAsks Current best ask prices.
     * @return The directional bias (Positive = Net Long, Negative = Net Short).
     */
    Ticks netMarketValue(const std::array<Ticks, numberOfSymbols>& bestBids,
        const std::array<Ticks, numberOfSymbols>& bestAsks) const;

    /**
     * @brief Calculate the "True Value" or Net Worth of the account.
     * @details Often called Net Liquidation Value (NLV).
     * Formula: Cash + Net Market Value - Margin Loan - Interest Owed.
     * @param bestBids Current best bid prices.
     * @param bestAsks Current best ask prices.
     * @return Total equity available if all positions were closed immediately.
     */
    Ticks netLiquidationValue(const std::array<Ticks, numberOfSymbols>& bestBids,
        const std::array<Ticks, numberOfSymbols>& bestAsks) const;

    /**
     * @brief Calculate the minimum equity required by the broker to keep positions open.
     * @details Usually a percentage (e.g., 30%) of the Gross Market Value.
     * @param prices Current market prices for the symbols held.
     * @return The minimum Net Liquidation Value required to avoid a margin call.
     */
    Ticks maintenanceRequirement(const std::array<Ticks, numberOfSymbols>& bestBids,
        const std::array<Ticks, numberOfSymbols>& bestAsks) const;

    /**
     */
    bool violatesMarginRequirement(const std::array<Ticks, numberOfSymbols>& bestBids,
        const std::array<Ticks, numberOfSymbols>& bestAsks) const;

    /**
     * @brief Validate if an order can be placed without violating margin or leverage limits.

     */
    bool sufficientEquityForOrder(const std::array<Ticks, numberOfSymbols>& bestBids,
        const std::array<Ticks, numberOfSymbols>& bestAsks,
        const NewOrder& order,
        Ticks totalOrderPrice,
        double leverageFactor) const;

   private:
    /**
     * @brief Update cost basis with weighted average calculation
     * @param fillPrice Price of the new fill
     * @param fillQuantity Quantity of the new fill
     */
    void updateCostBasis(std::uint16_t symbolId, Ticks fillPrice, Quantity fillQuantity);
};

}  // namespace sim
