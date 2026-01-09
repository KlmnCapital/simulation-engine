// strategy_interface.cppm
export module simulation_engine:strategy_interface;

import std;

import :order_placement;
import :portfolio;
import :quote;
import :types;
import :market_state;

export namespace sim {
// Forward declaration for Engine class
template <std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
class Engine;

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
template <std::size_t depth, std::uint16_t numberOfSymbols, typename Distribution>
class IStrategy {
   public:
    IStrategy(Portfolio<numberOfSymbols, Distribution> portfolio) : portfolio_(portfolio) {}
    virtual ~IStrategy() = default;

    virtual void onStart() {}
    virtual void onMarketData(const MarketState<depth, numberOfSymbols>& marketState) {}
    virtual void onEnd() {}

    void setEngine(Engine<depth, numberOfSymbols, Distribution>* engine) { engine_ = engine; }

    void onFill(const Fill& fill) { portfolio_.updatePortfolio(fill); }

    OrderId placeOrder(std::uint16_t symbol,
        OrderInstruction instruction,
        OrderType orderType,
        Quantity quantity,
        TimeInForce timeInForce = TimeInForce::Day,
        Ticks price = Ticks{0}) {
        OrderId orderId =
            engine_->placeOrder(symbol, instruction, orderType, quantity, timeInForce, price);

        NewOrder newOrder{orderId, symbol, timeInForce, instruction, orderType, price, quantity};

        Ticks orderValue = (price > Ticks{0}) ? (price * quantity) : Ticks{0};

        PendingOrder pendingOrder;
        pendingOrder.order = newOrder;
        pendingOrder.sendTime = engine_->marketData->currentTimeStamp();
        pendingOrders.push_back(std::move(pendingOrder));

        // Immediately update strategy portfolio when order is placed
        // This simulates real-world behavior where placing an order locks up funds
        if (instruction == OrderInstruction::Buy) {
            // Calculate how much will be on margin vs settled funds
            Ticks settledUsed = std::min(orderValue, portfolio_.settledFunds);
            Ticks marginAmount = orderValue - settledUsed;

            // Reduce settled funds
            portfolio_.settledFunds -= settledUsed;

            // Increase loan amount for margin portion
            portfolio_.loan += marginAmount;
        } else if (instruction == OrderInstruction::Sell) {
            // Increase settled funds when selling (immediate credit)
            portfolio_.settledFunds += orderValue;
        }

        return orderId;
    }

    bool cancel(OrderId orderId) {
        bool success = engine_->cancel(orderId);

        // If cancellation was successful, return funds to settled funds
        if (success) {
            auto it = std::find_if(pendingOrders.begin(), pendingOrders.end(),
                [orderId](const PendingOrder& po) { return po.order.id == orderId; });

            if (it != pendingOrders.end()) {
                Ticks orderValue = it->order.price * it->order.quantity;
                // Return funds based on original order
                if (it->order.instruction == OrderInstruction::Buy) {
                    // Calculate how much was on margin vs settled funds

                    Ticks settledUsed = std::min(orderValue, portfolio_.settledFunds + orderValue);
                    Ticks marginAmount = orderValue - settledUsed;

                    // Return settled funds
                    portfolio_.settledFunds += settledUsed;

                    // Reduce loan amount for margin portion
                    portfolio_.loan -= marginAmount;
                } else if (it->order.instruction == OrderInstruction::Sell) {
                    // Remove the immediate credit
                    portfolio_.settledFunds -= orderValue;
                }

                // Remove from pending orders
                pendingOrders.erase(it);
            }
        }

        return success;
    }

    bool replace(OrderId orderId, Quantity newQuantity, Ticks newPrice) {
        return engine_->replace(orderId, newQuantity, newPrice);
    }

    const Portfolio<numberOfSymbols, Distribution>& portfolio() const { return portfolio_; }

    Ticks currentPortfolioValue() const {
        return portfolio().netLiquidationValue(
            engine_->marketData->bestBids(), engine_->marketData->bestAsks());
    }

    // Order validation methods
    bool sufficientEquityForOrder(const NewOrder& order) const {
        return portfolio().sufficientEquityForOrder(engine_->marketData->bestBids(),
            engine_->marketData->bestAsks(), order, currentPortfolioValue(),
            engine_->leverageFactor);
    }

   protected:
    Engine<depth, numberOfSymbols, Distribution>* engine_{nullptr};
    Portfolio<numberOfSymbols, Distribution> portfolio_;

    // Track orders for fund management
    std::vector<PendingOrder> pendingOrders;
};

}  // namespace sim
