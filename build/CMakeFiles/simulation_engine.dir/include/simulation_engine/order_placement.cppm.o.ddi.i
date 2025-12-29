# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/order_placement.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/order_placement.cppm"
export module simulation_engine:order_placement;

import :types;

export namespace sim {
# 18 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/order_placement.cppm"
    struct NewOrder {
        OrderId id{0};
        SymbolId symbol{0};
        TimeInForce timeInForce{TimeInForce::Day};
        OrderInstruction instruction{OrderInstruction::Buy};
        OrderType orderType{OrderType::Limit};
        Ticks price;
        Quantity quantity{0};
    };
# 39 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/order_placement.cppm"
    struct Fill {
        OrderId id{0};
        SymbolId symbol{0};

        Quantity quantity{0};
        Ticks price{0};
        TimeStamp timestamp{0};


        OrderInstruction instruction{OrderInstruction::Buy};
        OrderType orderType{OrderType::Limit};
        TimeInForce timeInForce{TimeInForce::Day};
        Ticks originalPrice{0};
    };







    struct UnsettledFunds {
        TimeStamp earliestSettlement;
        Ticks cash;

        UnsettledFunds() = default;
        UnsettledFunds(TimeStamp settlementTime, Ticks amount)
            : earliestSettlement(settlementTime), cash(amount) {}
    };
# 76 "/home/nemeleks/kelemen_capital/dev/simulation_engine/include/simulation_engine/order_placement.cppm"
    struct PendingNotification {
        Fill fill;
        TimeStamp earliestNotifyTime;
        bool delivered{false};
    };







    struct PendingOrder {
        NewOrder order;
        TimeStamp sendTime;
        TimeStamp earliestExecution;
    };







    struct CancelOrder {
        OrderId orderId;
        TimeStamp sendTime;
        TimeStamp earliestExecution;
    };







    struct ReplaceOrder {
        OrderId orderId;
        Quantity newQuantity;
        Ticks newPrice;
        TimeStamp sendTime;
        TimeStamp earliestExecution;
    };

}
