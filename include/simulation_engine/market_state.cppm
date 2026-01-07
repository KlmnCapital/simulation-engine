// market_state.cppm
export module simulation_engine:market_state;

import :types;
import :quote;

import std;

export namespace sim {

    template <std::size_t depth, std::uint16_t numberOfSymbols>
    struct MarketState {
        // Data storage
        TimeStamp timestamp{0};
        std::array<Quote<depth>>, std::size_t numberOfSymbols> data{};

        // Methods
        const Ticks& operator[](std::size_t idx) const;

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

        static constexpr std::size_t getLevelCount(std::size_t) { return depth; }
        static constexpr std::size_t numberOfsymbolIds() { return state.size(); }
    };

    template <std::size_t depth, std::uint16_t numberOfSymbols>
    inline std::array<Ticks, numberOfSymbols> getBestBids() const {
        std::array<Ticks, numberOfSymbols> bestBids;
        for (std::uint16_t symbolId = 0; symbolId < numberOfSymbols; ++symbolId) {
            bestBids.push_back(bestBid(symboldId));
        }

        return bestBids;
    }

    template <std::size_t depth, std::uint16_t numberOfSymbols>
    inline std::array<Ticks, numberOfSymbols> getBestAsks() const {
        std::array<Ticks, numberOfSymbols> bestAsks;
        for (std::uint16_t symbolId = 0; symbolId < numberOfSymbols; ++symbolId) {
            bestAsks.push_back(bestAsk(symboldId));
        }

        return bestBids;
    }

    template <std::size_t depth>
    inline const Quote<depth>& Quote<depth>::operator[](std::size_t idx) const {
        return data[idx];
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::bestBid(std::uint16_t symbolId) const {
        // Find first valid (non-zero) bid price
        for (std::size_t i = 0; i < depth; ++i) {
            if (data[symbolId][i].value() > 0) {
                return data[symbolId].getBid(i);
            }
        }
        return data[symbolId].getBid(0); // Return first level if all are invalid
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::bestAsk(std::uint16_t symbolId) const {
        // Find first valid (non-zero) ask price
        for (std::size_t i = 0; i < depth; ++i) {
            if (data[symbolId][depth + i].value() > 0) {
                return data[symbolId].getAsk(i);
            }
        }
        return data[symbolId].gtAsk(0); // Return first level if all are invalid
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getBid(std::size_t priceLevel,std::uint16_t symbolId) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Bid price level out of range");
        }
        return data[symbolId].getBid(priceLevel);
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getAsk(std::size_t priceLevel, std::uint16_t symbolId) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Ask price level out of range");
        }
        return data[symbolId].getAsk(priceLevel);
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getBidSize(std::size_t priceLevel, std::uint16_t symbolId) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Bid size level out of range");
        }
        return data[symbolId].getBidSize(priceLevel);
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getAskSize(std::size_t priceLevel, std::uint16_t symbolId) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Ask size level out of range");
        }
        return data[symbolId].getAskSize(priceLevel);
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getBids(std::uint16_t symbolId) const {
        std::array<Ticks, depth> bids;
        for (std::size_t i = 0; i < depth; ++i) {
            bids[i] = data[symbolId].getBid(i);
        }
        return bids;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getAsks(std::uint16_t symbolId) const {
        std::array<Ticks, depth> asks;
        for (std::size_t i = 0; i < depth; ++i) {
            asks[i] = data[symbolId].getAsk(i);
        }
        return asks;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getBidSizes(std::uint16_t symbolId) const {
        std::array<Ticks, depth> bidSizes;
        for (std::size_t i = 0; i < depth; ++i) {
            bidSizes[i] = data[symbolId].getBidSize(i);
        }
        return bidSizes;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getAskSizes(std::uint16_t symbolId) const {
        std::array<Ticks, depth> askSizes;
        for (std::size_t i = 0; i < depth; ++i) {
            askSizes[i] = data[symbolId].getAskSize(i);
        }
        return askSizes;
    }
