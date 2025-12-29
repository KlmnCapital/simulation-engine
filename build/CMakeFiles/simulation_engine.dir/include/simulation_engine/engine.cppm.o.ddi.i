# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/engine.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/engine.cppm"
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
        std::vector<Fill> fills;
        Portfolio finalPortfolio;
        std::size_t quotesProcessed{0};
    };

    template <std::size_t depth>
    class Engine final {
    public:
        Engine(std::unique_ptr<IMarketData<depth>> marketData, RunParams params);
        Result run(IStrategy<depth>& strat,
            VerbosityLevel verbosity = VerbosityLevel::STANDARD,
            std::ostream& out = std::cout);


        const Portfolio& portfolio() const;
        Portfolio& getPortfolio() { return portfolio_; }
        Ticks currentPortfolioValue() const;


        const Quote<depth>& marketData(SymbolId) const;
        template <std::size_t N>
        const Quote<N>& marketData(SymbolId symbol) const {
            static_assert(N <= 255, "Maximum supported depth is 255 levels");
            static_assert(N <= depth, "Requested depth exceeds configured depth");
            return reinterpret_cast<const Quote<N>&>(currentQuote_);
        }


        bool sufficientEquityForOrder(const NewOrder& order);
        OrderId placeOrder(SymbolId symbol,
            OrderInstruction instruction,
            OrderType orderType,
            Quantity quantity,
            TimeInForce timeInForce = TimeInForce::Day,
            Ticks price = Ticks{0});
        bool cancel(OrderId orderId);
        bool replace(OrderId orderId, Quantity newQuaneity, Ticks newPrice);

    private:
        struct ExecutionResult {
            std::vector<Fill> fills;
            NewOrder remainingOrder;
            bool isComplete;
        };


        Result simulate(IStrategy<depth>& strategy);
        ExecutionResult tryExecute(const NewOrder& newOrder,
            TimeStamp sendTs,
            const Quote<depth>& quote);
        void notifyFill(const Fill& fill, TimeStamp earliestNotificationTime);
        void processPendingOrders();
        void processPendingNotifications(IStrategy<depth>& strategy);
        void processSettlements();
        bool isTimeForSettlement(TimeStamp currentTime) const;


        void checkMarginRequirement();
        void executeMarginCall();


        RunParams params_;
        std::unique_ptr<IMarketData<depth>> marketData_;
        Statistics<depth> stats_;


        Portfolio portfolio_;
        std::vector<Fill> fills_;
        std::size_t quotesProcessed_{0};
        Quote<depth> currentQuote_;
        OrderId nextOrderId_{1};
        std::vector<PendingOrder> pendingOrders_;
        std::vector<CancelOrder> pendingCancels_;
        std::vector<ReplaceOrder> pendingReplaces_;
        std::vector<PendingNotification> pendingNotifications_;


        IStrategy<depth>* strategy_{nullptr};
        TimeStamp lastSettlementDate_{0};


        std::uint64_t sendLatencyNs_{0};
        std::uint64_t receiveLatencyNs_{0};
        std::uint64_t totalLatencyNs_{0};


        std::mt19937 randomNumberGenerator_;
        std::normal_distribution<float> fillRateDistribution_;
        std::uniform_int_distribution<int> partialFillProbabilityDistribution_;


        VerbosityLevel currentVerbosity_{VerbosityLevel::STANDARD};
    };

}
