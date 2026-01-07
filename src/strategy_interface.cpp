// strategy_interface.cpp
module simulation_engine;

namespace sim {
    template <std::size_t depth>
    void IStrategy<depth>::setEngine(Engine<depth>* engine) {
        engine_ = engine;
    }

    template <std::size_t depth>
    void IStrategy<depth>::onFill(const Fill& fill) {
        // Update local portfolio with fill (strategy sees fills with delay)
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

        // Calculate order value (use market price for market orders)
        Ticks orderValue = (price > Ticks{0}) ? (price * quantity) : (Ticks{0} * quantity);

        // Track the order for fund management
        pendingOrders_.push_back({orderId, instruction, quantity, price, orderValue});

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

    template <std::size_t depth>
    bool IStrategy<depth>::cancel(OrderId orderId) {
        bool success = engine_->cancel(orderId);

        // If cancellation was successful, return funds to settled funds
        if (success) {
            auto it = std::find_if(pendingOrders_.begin(), pendingOrders_.end(),
                [orderId](const PendingOrder& po) { return po.id == orderId; });

            if (it != pendingOrders_.end()) {
                // Return funds based on original order
                if (it->instruction == OrderInstruction::Buy) {
                    // Calculate how much was on margin vs settled funds
                    Ticks settledUsed = std::min(it->value, portfolio_.settledFunds + it->value);
                    Ticks marginAmount = it->value - settledUsed;

                    // Return settled funds
                    portfolio_.settledFunds += settledUsed;

                    // Reduce loan amount for margin portion
                    portfolio_.loan -= marginAmount;
                } else if (it->instruction == OrderInstruction::Sell) {
                    // Remove the immediate credit
                    portfolio_.settledFunds -= it->value;
                }

                // Remove from pending orders
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
        // Get current market data from engine
        auto currentQuote = engine_->marketData(order.symbol);
        Ticks bestBid = currentQuote.bestBid();
        Ticks bestAsk = currentQuote.bestAsk();

        // Use portfolio's validation method
        return portfolio_.sufficientEquityForOrder(
            order, bestBid, bestAsk, 2.0);  // Default 2x leverage
    }

    // Explicit template instantiations for common depths
    template class IStrategy<1>;
    template class IStrategy<5>;
    template class IStrategy<10>;

}  // namespace sim
