export module simulation_engine:engine;

import std;

import :market_data;
import :order_placement;
import :portfolio;
import :run_params;
import :statistics;
import :strategy_interface;
import :types;
import :quote;

export namespace sim {

struct Result {
    std::vector<Fill> fills;         // Vector of all fills during simulation
    Portfolio finalPortfolio;        // Final portfolio state
    std::size_t quotesProcessed{0};  // Total number of quotes processed
};

struct ExecutionResult {
    std::vector<Fill> fills;  // Fills generated from execution
    NewOrder remainingOrder;  // Order with remaining quantity
    bool isComplete;          // True if order is fully filled
};



template <std::size_t depth, std::uint16_t numberofSymbols, typename Distribution>
class Engine final {
   public:
    /**
     * @brief Construct a new Engine object.
     * @details Initializes the simulation environment with market data providers and execution parameters.
     * @param marketData Unique pointer to the market data source.
     * @param params Configuration parameters for the simulation run.
     */
    Engine(std::unique_ptr<IMarketData<depth>> marketData, RunParams params);

    /**
     * @brief Start and manage the simulation loop.
     * @details Iterates through market data, invokes the strategy, and manages order lifecycles.
     * @param strat Reference to the trading strategy to be executed.
     * @param verbosity Level of logging detail output to the stream.
     * @param out The output stream for logging.
     * @return A Result struct containing fills, final portfolio, and processing stats.
     */
    Result run(IStrategy<depth>& strat,
        VerbosityLevel verbosity = VerbosityLevel::STANDARD,
        std::ostream& out = std::cout
    );

    /**
     * @brief Retrieve the current state of the portfolio.
     * @return A constant reference to the Portfolio object.
     */
    const Portfolio<numberOfSymbols>& portfolio() const;

    /**
     * @brief Calculate the total mark-to-market value of the portfolio.
     * @details Combines cash balances and the value of open positions based on current market prices.
     * @return The total value in Ticks.
     */
    Ticks currentPortfolioValue() const;

    /**
     * @brief Provide access to the underlying market data provider.
     * @return A constant reference to the MarketData object.
     */
    const MarketData<depth, numberOfSymbols>& marketData() const;

    /**
     * @brief Provide access to the current snapshots of the market.
     * @return A constant reference to the MarketState object.
     */
    const MarketState<depth, numberOfSymbols>& marketState() const;

    /**
     * @brief Retrieve performance and simulation statistics.
     * @return A constant reference to the Statistics object.
     */
    const Statistics& statistics() const;

    /**
     * @brief Validate if the account has enough equity/margin to support a new order.
     * @details Checked prior to order placement to prevent margin violations.
     * @param order The order details to be validated.
     * @return True if the equity requirement is met, false otherwise.
     */
    bool sufficientEquityForOrder(const NewOrder& order);

    /**
     * @brief Place a new order into the simulation engine.
     * @details Queues the order and assigns a unique OrderId, accounting for send latency.
     * @param symbol The identifier for the instrument.
     * @param instruction Buy or Sell.
     * @param orderType Market, Limit, etc.
     * @param quantity The amount to trade.
     * @param timeInForce Duration the order remains active.
     * @param price Limit price (if applicable).
     * @return The unique OrderId assigned to the new order.
     */
    OrderId placeOrder(SymbolId symbol,
        OrderInstruction instruction,
        OrderType orderType,
        Quantity quantity,
        TimeInForce timeInForce = TimeInForce::Day,
        Ticks price = Ticks{0}
    );

    /**
     * @brief Request the cancellation of an existing order.
     * @param orderId The unique identifier of the order to cancel.
     * @return True if the cancel request was successfully queued.
     */
    bool cancel(OrderId orderId);

    /**
     * @brief Request the modification of an existing order's parameters.
     * @param orderId The unique identifier of the order to replace.
     * @param newQuantity The updated volume.
     * @param newPrice The updated limit price.
     * @return True if the replace request was successfully queued.
     */
    bool replace(OrderId orderId, Quantity newQuantity, Ticks newPrice);

   private:
    RunParams params_;
    std::unique_ptr<IMarketData<depth>> marketData;
    IStrategy<depth>* strategy{nullptr};
    Portfolio portfolio;
    Distribution buyFillRateDistribution;
    Distribution sellFillRateDistribution;
    std::mt19937& randomNumberGenerator;
    Statistics<depth> statistics;
    VerbosityLevel verbosityLevel;
    int statisticsUpateRateSeconds;
    std::uint8_t leverageFactor;
    std::vector<PendingOrder> pendingOrders;
    std::vector<CancelOrder> pendingCancels;
    std::vector<ReplaceOrder> pendingReplaces;
    std::vector<PendingNotification> pendingNotifications;
    std::uint64_t sendLatencyNs;
    std::uint64_t receiveLatencyNs;
    std::uint64_t totalLatencyNs;
    std::vector<Fill> fills;
    std::size_t quotesProcessed{0};
    OrderId nextOrderId{1};
    TimeStamp lastSettlementDate{0};

    /**
     * @brief Deliver execution and order updates to the strategy.
     * @details Respects receive latency by only notifying the strategy once the time has passed.
     * @param strategy The strategy instance to notify.
     */
    void processPendingNotifications(IStrategy<depth>& strategy);

    /**
     * @brief Handle daily clearing and settlement logic.
     * @details Updates settled cash balances and resets daily trading limits.
     */
    void processSettlements();

    /**
     * @brief Determine if enough time has passed to trigger a settlement cycle.
     * @param currentTime The current simulation timestamp.
     * @return True if a settlement is due.
     */
    bool isTimeForSettlement(TimeStamp currentTime) const;

    /**
     * @brief Internal helper to queue a fill notification for the strategy.
     * @param fill The details of the trade execution.
     * @param earliestNotificationTime The timestamp when the strategy can "see" this fill.
     */
    void notifyFill(const Fill& fill, TimeStamp earliestNotificationTime);

    /**
     * @brief Orchestrate the processing of all queued order actions.
     * @details Coordinates the execution of new, replace, and cancel orders.
     */
    void processPendingOrders();

    /**
     * @brief Process new Buy and Sell orders that have arrived after latency.
     */
    void processPendingBuySellOrders();

    /**
     * @brief Process queued requests to modify existing orders.
     */
    void processPendingReplaceOrders();

    /**
     * @brief Process queued requests to remove existing orders.
     */
    void processPendingCancelOrders();

    /**
     * @brief Evaluate the portfolio against margin maintenance requirements.
     */
    void checkMarginRequirement();

    /**
     * @brief Forcefully close positions to restore required margin levels.
     */
    void executeMarginCall();

    /**
     * @brief Core simulation logic runner.
     * @param strategy The strategy being tested.
     * @return The final results of the simulation.
     */
    Result simulate(IStrategy<depth>& strategy);

    /**
     * @brief Attempt to match a single order against the current market quote.
     * @details Simulates the exchange matching engine logic and fill rate probabilities.
     * @param newOrder The order to be executed.
     * @param sendTs The time the order arrived at the exchange.
     * @param quote The current market depth for matching.
     * @return ExecutionResult containing any generated fills and remaining quantity.
     */
    ExecutionResult tryExecute(const NewOrder& newOrder, TimeStamp sendTs, const Quote<depth>& quote);

    /**
     * @brief Check if the simulation time falls within allowed trading hours.
     * @param currentTime The current simulation timestamp.
     * @return True if trading is permitted.
     */
    bool canTrade(TimeStamp currentTime) const;

    /**
     * @brief Determine if the current timestamp falls within Daylight Savings Time.
     * @param currentTime The timestamp to check.
     * @return True if DST is active.
     */
    bool isInsideDST(TimeStamp currentTime) const;
};

}  // namespace sim
