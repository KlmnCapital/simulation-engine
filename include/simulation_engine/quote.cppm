// quote.cppm
export module simulation_engine:quote;

import :types;

import std;

export namespace sim {

    template <std::size_t depth>
    struct Quote {
        // Data storage
        TimeStamp timestamp{0};
        std::array<Ticks, 2 * depth> prices{};
        std::array<Ticks, 2 * depth> sizes{};
        std::size_t symbolId;

        // Methods
        const Ticks& bestBid() const;
        const Ticks& bestAsk() const;

        const Ticks& getBid(std::size_t priceLevel) const;
        const Ticks& getAsk(std::size_t priceLevel) const;
        const Ticks& getBidSize(std::size_t priceLevel) const;
        const Ticks& getAskSize(std::size_t priceLevel) const;

        std::array<Ticks, depth> getBids() const;
        std::array<Ticks, depth> getAsks() const;
        std::array<Ticks, depth> getBidSizes() const;
        std::array<Ticks, depth> getAskSizes() const;
    };

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::bestBid() const {
        // Find first valid (non-zero) bid price
        for (std::size_t i = 0; i < depth; ++i) {
            if (prices[i].value() > 0) {
                return prices[i];
            }
        }
        return prices[0]; // Return first level if all are invalid
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::bestAsk() const {
        // Find first valid (non-zero) ask price 
        for (std::size_t i = 0; i < depth; ++i) { 
            if (prices[depth + i].value() > 0) {
                return prices[depth + i];
            }
        }
        return prices[depth]; // Return first level if all are invalid
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getBid(std::size_t priceLevel) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Bid price level out of range");
        }
        return prices[priceLevel];
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getAsk(std::size_t priceLevel) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Ask price level out of range");
        }
        return prices[depth + priceLevel];
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getBidSize(std::size_t priceLevel) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Bid size level out of range");
        }
        return sizes[priceLevel];
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getAskSize(std::size_t priceLevel) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Ask size level out of range");
        }
        return sizes[depth + priceLevel];
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getBids() const {
        std::array<Ticks, depth> bids;
        for (std::size_t i = 0; i < depth; ++i) {
            bids[i] = prices[i];
        }
        return bids;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getAsks() const {
        std::array<Ticks, depth> asks;
        for (std::size_t i = 0; i < depth; ++i) {
            asks[i] = prices[depth + i];
        }
        return asks;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getBidSizes() const {
        std::array<Ticks, depth> bidSizes;
        for (std::size_t i = 0; i < depth; ++i) {
            bidSizes[i] = sizes[i];
        }
        return bidSizes;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getAskSizes() const {
        std::array<Ticks, depth> askSizes;
        for (std::size_t i = 0; i < depth; ++i) {
            askSizes[i] = prices[depth + i];
        }
        return askSizes;
    }

template struct Quote<10>;

} // namespace sim
