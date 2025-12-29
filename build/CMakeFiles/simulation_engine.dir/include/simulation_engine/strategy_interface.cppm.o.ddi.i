# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/strategy_interface.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/strategy_interface.cppm"

export module simulation_engine:strategy_interface;

import std;

import :order_placement;
import :portfolio;
import :quote;
import :types;

export namespace sim {

    template <std::size_t depth> class Engine;
# 32 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/strategy_interface.cppm"
    template <std::size_t depth>
    class IStrategy {
    public:
        IStrategy(Portfolio portfolio) : portfolio_(portfolio) {}
        virtual ~IStrategy() = default;

        virtual void onStart() {}
        virtual void onMarketData(const Quote<depth>&) {}
        virtual void onEnd() {}

        void setEngine(Engine<depth>* engine);
        void onFill(const Fill& fill);


        OrderId placeOrder(SymbolId symbol,
            OrderInstruction instruction,
            OrderType orderType,
            Quantity quantity,
            TimeInForce timeInForce = TimeInForce::Day,
            Ticks price = Ticks{0});

        bool cancel(OrderId orderId);

        bool replace(OrderId orderId, Quantity newQuantity, Ticks newPrice);


        const Portfolio& portfolio() const;
        Ticks currentPortfolioValue() const;


        bool sufficientEquityForOrder(const NewOrder& order) const;

    protected:
        Engine<depth>* engine_{nullptr};
        Portfolio portfolio_;


        struct PendingOrder {
            OrderId id;
            OrderInstruction instruction;
            Quantity quantity;
            Ticks price;
            Ticks value;
        };
        std::vector<PendingOrder> pendingOrders_;
    };

}
