// market_state.cppm
export module simulation_engine:market_state;

import :types;
import :quote;

import std;

export namespace sim {

    template <std::size_t depth, std::size_t numberOfSymbols>
    struct MarketState {
        // Data storage
        TimeStamp timestamp{0};
        std::array<Quote<depth>, numberOfSymbols> data{};

        // Methods
        const Ticks& operator[](std::size_t idx) const;

        const Ticks& bestBid(SymbolId symbol) const;
        const Ticks& bestAsk(SymbolId symbol) const;

        const Ticks& getBid(std::size_t priceLevel, symbolId symbol) const;
        const Ticks& getAsk(std::size_t priceLevel, SymbolId symbol) const;
        const Ticks& getBidSize(std::size_t priceLevel, symbolId symbol) const;
        const Ticks& getAskSize(std::size_t priceLevel, SymbolId symbol) const;

        std::array<Ticks, depth> getBids(SymbolId symbol) const;
        std::array<Ticks, depth> getAsks(SymbolId symbol) const;
        std::array<Ticks, depth> getBidSizes(SymbolId symbol) const;
        std::array<Ticks, depth> getAskSizes(SymbolId symbol) const;

        static constexpr std::size_t getLevelCount(SymbolId) { return depth; }
        static constexpr std::size_t numberOfSymbols() { return state.size(); }
    };

    template <std::size_t depth>
    inline const Quote<depth>& Quote<depth>::operator[](std::size_t idx) const {
        return data[idx];
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::bestBid(SymbolId symbol) const {
        // Find first valid (non-zero) bid price
        for (std::size_t i = 0; i < depth; ++i) {
            if (data[symbol][i].value() > 0) {
                return data[symbol].getBid(i);
            }
        }
        return data[symbol].getBid(0); // Return first level if all are invalid
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::bestAsk(SymbolId symbol) const {
        // Find first valid (non-zero) ask price
        for (std::size_t i = 0; i < depth; ++i) {
            if (data[symbol][depth + i].value() > 0) {
                return data[symbol].getAsk(i);
            }
        }
        return data[symbol].gtAsk(0); // Return first level if all are invalid
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getBid(std::size_t priceLevel,SymbolId symbol) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Bid price level out of range");
        }
        return data[symbol].getBid(priceLevel);
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getAsk(std::size_t priceLevel, SymbolId symbol) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Ask price level out of range");
        }
        return data[symbol].getAsk(priceLevel);
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getBidSize(std::size_t priceLevel, SymbolId symbol) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Bid size level out of range");
        }
        return data[symbol].getBidSize(priceLevel);
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getAskSize(std::size_t priceLevel, SymbolId symbol) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Ask size level out of range");
        }
        return data[symbol].getAskSize(priceLevel);
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getBids(SymbolId symbol) const {
        std::array<Ticks, depth> bids;
        for (std::size_t i = 0; i < depth; ++i) {
            bids[i] = data[symbol].getBid(i);
        }
        return bids;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getAsks(SymbolId symbol) const {
        std::array<Ticks, depth> asks;
        for (std::size_t i = 0; i < depth; ++i) {
            asks[i] = data[symbol].getAsk(i);
        }
        return asks;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getBidSizes(SymbolId symbol) const {
        std::array<Ticks, depth> bidSizes;
        for (std::size_t i = 0; i < depth; ++i) {
            bidSizes[i] = data[symbol].getBidSize(i);
        }
        return bidSizes;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getAskSizes(SymbolId symbol) const {
        std::array<Ticks, depth> askSizes;
        for (std::size_t i = 0; i < depth; ++i) {
            askSizes[i] = data[symbol].getAskSize(i);
        }
        return askSizes;
    }
