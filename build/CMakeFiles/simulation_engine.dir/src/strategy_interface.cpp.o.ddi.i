# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/src/strategy_interface.cpp"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/src/strategy_interface.cpp"

module simulation_engine;

namespace sim {
    template <std::size_t depth>
    void IStrategy<depth>::setEngine(Engine<depth>* engine) {
        engine_ = engine;
    }
    template <std::size_t depth>
    void IStrategy<depth>::onFill(const Fill& fill) {

        portfolio_.updatePortfolio(fill);
    }

    template <std::size_t depth>
    OrderId IStrategy<depth>::placeOrder(SymbolId symbol,
        OrderInstruction instruction,
        OrderType orderType,
        Quantity quantity,
        TimeInForce timeInForce,
        Ticks price) {
        OrderId orderId =
            engine_->placeOrder(symbol, instruction, orderType, quantity, timeInForce, price);


        Ticks orderValue = (price > Ticks{0}) ? (price * quantity) : (Ticks{0} * quantity);


        pendingOrders_.push_back({orderId, instruction, quantity, price, orderValue});



        if (instruction == OrderInstruction::Buy) {

            Ticks settledUsed = std::min(orderValue, portfolio_.settledFunds);
            Ticks marginAmount = orderValue - settledUsed;


            portfolio_.settledFunds -= settledUsed;


            portfolio_.loan += marginAmount;
        } else if (instruction == OrderInstruction::Sell) {

            portfolio_.settledFunds += orderValue;
        }

        return orderId;
    }

    template <std::size_t depth>
    bool IStrategy<depth>::cancel(OrderId orderId) {
        bool success = engine_->cancel(orderId);


        if (success) {
            auto it = std::find_if(pendingOrders_.begin(), pendingOrders_.end(),
                [orderId](const PendingOrder& po) { return po.id == orderId; });

            if (it != pendingOrders_.end()) {

                if (it->instruction == OrderInstruction::Buy) {

                    Ticks settledUsed = std::min(it->value, portfolio_.settledFunds + it->value);
                    Ticks marginAmount = it->value - settledUsed;


                    portfolio_.settledFunds += settledUsed;


                    portfolio_.loan -= marginAmount;
                } else if (it->instruction == OrderInstruction::Sell) {

                    portfolio_.settledFunds -= it->value;
                }


                pendingOrders_.erase(it);
            }
        }

        return success;
    }

    template <std::size_t depth>
    bool IStrategy<depth>::replace(OrderId orderId, Quantity newQuantity, Ticks newPrice) {
        return engine_->replace(orderId, newQuantity, newPrice);
    }

    template <std::size_t depth>
    const Portfolio& IStrategy<depth>::portfolio() const {
        return engine_->portfolio();
    }

    template <std::size_t depth>
    Ticks IStrategy<depth>::currentPortfolioValue() const {
        return engine_->currentPortfolioValue();
    }

    template <std::size_t depth>
    bool IStrategy<depth>::sufficientEquityForOrder(const NewOrder& order) const {

        auto currentQuote = engine_->marketData(order.symbol);
        Ticks bestBid = currentQuote.bestBid();
        Ticks bestAsk = currentQuote.bestAsk();


        return portfolio_.sufficientEquityForOrder(
            order, bestBid, bestAsk, 2.0);
    }


    template class IStrategy<1>;
    template class IStrategy<5>;
    template class IStrategy<10>;

}
