// quote.cppm
export module simulation_engine:quote;

import :types;

import std;

export namespace sim {

    template <std::size_t depth>
    struct Quote {
        // Data storage
        TimeStamp timestamp{0};
        std::array<Ticks, 4 * depth> columns{};

        // Methods
        const Ticks& operator[](std::size_t idx) const;

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

        static constexpr std::size_t getLevelCount() { return depth; }
        static constexpr std::size_t getArraySize() { return 4 * depth; }
    };

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::operator[](std::size_t idx) const {
        return columns[idx];
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::bestBid() const {
        // Find first valid (non-zero) bid price
        for (std::size_t i = 0; i < depth; ++i) {
            if (columns[i].value() > 0) {
                return columns[i];
            }
        }
        return columns[0]; // Return first level if all are invalid
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::bestAsk() const {
        // Find first valid (non-zero) ask price
        for (std::size_t i = 0; i < depth; ++i) {
            if (columns[depth + i].value() > 0) {
                return columns[depth + i];
            }
        }
        return columns[depth]; // Return first level if all are invalid
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getBid(std::size_t priceLevel) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Bid price level out of range");
        }
        return columns[priceLevel];
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getAsk(std::size_t priceLevel) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Ask price level out of range");
        }
        return columns[depth + priceLevel];
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getBidSize(std::size_t priceLevel) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Bid size level out of range");
        }
        return columns[2 * depth + priceLevel];
    }

    template <std::size_t depth>
    inline const Ticks& Quote<depth>::getAskSize(std::size_t priceLevel) const {
        if (priceLevel >= depth) {
            throw std::out_of_range("Ask size level out of range");
        }
        return columns[3 * depth + priceLevel];
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getBids() const {
        std::array<Ticks, depth> bids;
        for (std::size_t i = 0; i < depth; ++i) {
            bids[i] = columns[i];
        }
        return bids;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getAsks() const {
        std::array<Ticks, depth> asks;
        for (std::size_t i = 0; i < depth; ++i) {
            asks[i] = columns[depth + i];
        }
        return asks;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getBidSizes() const {
        std::array<Ticks, depth> bidSizes;
        for (std::size_t i = 0; i < depth; ++i) {
            bidSizes[i] = columns[2 * depth + i];
        }
        return bidSizes;
    }

    template <std::size_t depth>
    inline std::array<Ticks, depth> Quote<depth>::getAskSizes() const {
        std::array<Ticks, depth> askSizes;
        for (std::size_t i = 0; i < depth; ++i) {
            askSizes[i] = columns[3 * depth + i];
        }
        return askSizes;
    }

};  // namespace sim
