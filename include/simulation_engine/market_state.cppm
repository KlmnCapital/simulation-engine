// market_state.cppm
module;

#include <cassert>

export module simulation_engine:market_state;

import :types;
import :quote;

import std;

export namespace sim {

template <std::size_t depth, std::uint16_t numberOfSymbols>
struct MarketState {
    // Data storage
    TimeStamp timestamp{0};
    std::array<Quote<depth>, numberOfSymbols> data{};

    Quote<depth>& operator[](std::uint16_t symbolId);
    const Quote<depth>& operator[](std::uint16_t symbolId) const;
    const Quote<depth>& getQuote(std::uint16_t symbolId) const;

    const Ticks& bestBid(std::uint16_t symbolId) const;
    const Ticks& bestAsk(std::uint16_t symbolId) const;

    const Ticks& getBid(std::size_t priceLevel, std::uint16_t symbolId) const;
    const Ticks& getAsk(std::size_t priceLevel, std::uint16_t symbolId) const;
    const Ticks& getBidSize(std::size_t priceLevel, std::uint16_t symbolId) const;
    const Ticks& getAskSize(std::size_t priceLevel, std::uint16_t symbolId) const;

    std::array<Ticks, depth> getBids(std::uint16_t symbolId) const;
    std::array<Ticks, depth> getAsks(std::uint16_t symbolId) const;
    std::array<Ticks, depth> getBidSizes(std::uint16_t symbolId) const;
    std::array<Ticks, depth> getAskSizes(std::uint16_t symbolId) const;

    std::array<Ticks, numberOfSymbols> getBestBids() const;
    std::array<Ticks, numberOfSymbols> getBestAsks() const;
};

template<std::size_t depth, std::uint16_t numberOfSymbols>
inline Quote<depth>& MarketState<depth, numberOfSymbols>::operator[](std::uint16_t symbolId) {
    assert(symbol Id < numberOfSymbols);
    return data[symbolId];
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
inline const Quote<depth>& MarketState<depth, numberOfSymbols>::operator[](std::uint16_t symbolId) const {
    return data[symbolId];
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
inline const Quote<depth>& MarketState<depth, numberOfSymbols>::getQuote(std::uint16_t symbolId) const {
    return data[symbolId];
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
inline std::array<Ticks, numberOfSymbols> MarketState<depth, numberOfSymbols>::getBestBids() const {
    std::array<Ticks, numberOfSymbols> bestBids;
    for (std::uint16_t i = 0; i < numberOfSymbols; ++i) {
        bestBids[i] = bestBid(i);
    }
    return bestBids;
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
inline std::array<Ticks, numberOfSymbols> MarketState<depth, numberOfSymbols>::getBestAsks() const {
    std::array<Ticks, numberOfSymbols> bestAsks;
    for (std::uint16_t i = 0; i < numberOfSymbols; ++i) {
        bestAsks[i] = bestAsk(i);
    }
    return bestAsks;
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
inline const Ticks& MarketState<depth, numberOfSymbols>::bestBid(std::uint16_t symbolId) const {
    return data[symbolId].bestBid();
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
inline const Ticks& MarketState<depth, numberOfSymbols>::bestAsk(std::uint16_t symbolId) const {
    return data[symbolId].bestAsk();
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
inline const Ticks& MarketState<depth, numberOfSymbols>::getBid(std::size_t level, std::uint16_t symbolId) const {
    return data[symbolId].getBid(level);
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
inline const Ticks& MarketState<depth, numberOfSymbols>::getAsk(std::size_t level, std::uint16_t symbolId) const {
    return data[symbolId].getAsk(level);
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
inline const Ticks& MarketState<depth, numberOfSymbols>::getBidSize(std::size_t level, std::uint16_t symbolId) const {
    return data[symbolId].getBidSize(level);
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
inline const Ticks& MarketState<depth, numberOfSymbols>::getAskSize(std::size_t level, std::uint16_t symbolId) const {
    return data[symbolId].getAskSize(level);
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
inline std::array<Ticks, depth> MarketState<depth, numberOfSymbols>::getBids(std::uint16_t symbolId) const {
    return data[symbolId].getBids();
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
inline std::array<Ticks, depth> MarketState<depth, numberOfSymbols>::getAsks(std::uint16_t symbolId) const {
    return data[symbolId].getAsks();
}

template struct MarketState<10, 4>;
template struct MarketState<10, 1>;

} // namespace sim
