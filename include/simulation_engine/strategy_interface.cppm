// strategy_interface.cppm
export module simulation_engine:strategy_interface;

import std;

import :order_placement;
import :portfolio;
import :quote;
import :types;

export namespace sim {
    // Forward declaration for Engine class
    template <std::size_t depth> class Engine;

    /**
    * @brief Interface for trading strategies
    * @details
    * Abstract base class that defines the contract for trading strategies.
    * Strategies implement specific trading logic and receive callbacks from
    * the simulation engine at key events during the simulation.
    *
    * The strategy interface provides a clean separation between trading logic
    * and simulation infrastructure. Strategies can focus on their specific
    * algorithms while the engine handles market data, order execution, and
    * portfolio management.
    *
    * Key strategy callbacks:
    * - onMarketData: Called when new market data arrives
    * - onFill: Called when orders are executed
    * - onEnd: Called at the end of simulation
    */
    template <std::size_t depth, std::uint64_t numberOfSymbols>
    class IStrategy {
    public:
        IStrategy(Portfolio portfolio) : portfolio_(portfolio) {}
        virtual ~IStrategy() = default;

        virtual void onStart() {}
        virtual void onMarketData(const MarketState<depth, numberOfSymbols>&) {}
        virtual void onEnd() {}

        void setEngine(Engine<depth, numberOfSymbols>* engine);
        void onFill(const Fill& fill);

        // Order placement helper methods
        OrderId placeOrder(SymbolId symbol,
            OrderInstruction instruction,
            OrderType orderType,
            Quantity quantity,
            TimeInForce timeInForce = TimeInForce::Day,
            Ticks price = Ticks{0});

        bool cancel(OrderId orderId);

        bool replace(OrderId orderId, Quantity newQuantity, Ticks newPrice);

        // Portfolio access methods
        const Portfolio& portfolio() const;
        Ticks currentPortfolioValue() const;

        // Order validation methods
        bool sufficientEquityForOrder(const NewOrder& order) const;

    protected:
        Engine<depth>* engine_{nullptr};
        Portfolio portfolio_;

        // Track orders for fund management
        std::vector<PendingOrder> pendingOrders_;
    };

}  // namespace sim
