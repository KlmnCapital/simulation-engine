export module simulation_engine:order_placement;

import :types;

export namespace sim {

    /**
    * @brief New order to be submitted to the market
    * @details
    * Represents a new order that a strategy wants to submit to the market.
    * The order contains all necessary information for execution including
    * symbol, instruction, quantity, price, and timing parameters.
    *
    * The engine assigns a unique OrderId when the order is accepted.
    * Orders can be market orders (immediate execution) or limit orders
    * (execution at specified price or better).
    */
    struct NewOrder {
        OrderId id{0};
        std::uint16_t symbol{0};
        TimeInForce timeInForce{TimeInForce::Day};
        OrderInstruction instruction{OrderInstruction::Buy};
        OrderType orderType{OrderType::Limit};
        Ticks price;  // Price for limit and stop limit orders.
        Quantity quantity{0};
    };

    /**
    * @brief Order execution result
    * @details
    * Represents the execution of an order at a specific price and quantity.
    * Fills are generated when orders are matched against market data and
    * are used to update portfolio positions and calculate P&L.
    *
    * Each fill corresponds to a partial or complete execution of an order.
    * Multiple fills may be generated for a single order if it executes
    * across multiple price levels or time periods.
    */
    struct Fill {
        OrderId id{0};       // Order ID that generated this fill
        std::uint16_t symbol{0};  // Symbol that was traded

        Quantity quantity{0};    // Quantity executed in this fill
        Ticks price{0};          // Execution price in ticks
        TimeStamp timestamp{0};  // Timestamp when fill occurred

        // Original order details
        OrderInstruction instruction{OrderInstruction::Buy};
        OrderType orderType{OrderType::Limit};
        TimeInForce timeInForce{TimeInForce::Day};
        Ticks originalPrice{0};  // Original order price (for limit orders)
    };

    /**
    * @brief Unsettled funds tracking
    * @details
    * Represents funds that are pending settlement after a trade.
    * Funds become available after the settlement period (typically 36 hours).
    */
    struct UnsettledFunds {
        TimeStamp earliestSettlement;  // When these funds will be available
        Ticks cash;                    // Amount of unsettled cash

        UnsettledFunds() = default;
        UnsettledFunds(TimeStamp settlementTime, Ticks amount)
            : earliestSettlement(settlementTime), cash(amount) {}
    };

    /**
    * @brief Pending notification for delayed strategy callbacks
    * @details
    * Represents a fill notification that should be delivered to the strategy
    * at a specific future timestamp. This allows for realistic simulation
    * of order execution delays and notification timing.
    */
    struct PendingNotification {
        Fill fill;                     // The fill to notify about
        TimeStamp earliestNotifyTime;  // Earliest time to deliver notification
        bool delivered{false};         // Whether notification has been delivered
    };

    /**
    * @brief Pending order waiting for execution
    * @details
    * Represents an order that has been submitted but not yet executed.
    * Tracks timing information for latency simulation.
    */
    struct PendingOrder {
        NewOrder order;
        TimeStamp sendTime;           // When order was sent
        TimeStamp earliestExecution;  // When order can execute (sendTime + latency)
    };

    /**
    * @brief Pending order cancellation
    * @details
    * Represents a cancellation request that has been submitted but not yet processed.
    * Tracks timing information for latency simulation.
    */
    struct CancelOrder {
        OrderId orderId;              // Order ID to cancel
        TimeStamp sendTime;           // When cancel was sent
        TimeStamp earliestExecution;  // When cancel can execute (sendTime + latency)
    };

    /**
    * @brief Pending order replacement
    * @details
    * Represents an order replacement request that has been submitted but not yet processed.
    * Tracks timing information for latency simulation.
    */
    struct ReplaceOrder {
        OrderId orderId;              // Order ID to replace
        Quantity newQuantity;         // New quantity
        Ticks newPrice;               // New price
        TimeStamp sendTime;           // When replace was sent
        TimeStamp earliestExecution;  // When replace can execute (sendTime + latency)
    };

}  // namespace sim
