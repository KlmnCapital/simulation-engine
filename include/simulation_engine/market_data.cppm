// market_data.cppm
export module simulation_engine:market_data;

import :quote;
import :types;
import :market_state;

import datetime;

import std;

export namespace sim {

template <std::size_t depth, std::uint16_t numberOfSymbols>
class IMarketData {
   public:
    IMarketData(const std::string& marketDataFilePath, bool multipleFiles)
        : marketDataFilePath_(marketDataFilePath),
          marketDataFilePaths_{},
          multipleFiles_(multipleFiles),
          currentFileIndex(0) {}

    IMarketData(const std::vector<std::string>& marketDataFilePaths, bool multipleFiles)
        : marketDataFilePath_{},
          marketDataFilePaths_(marketDataFilePaths),
          multipleFiles_(multipleFiles),
          currentFileIndex(0) {}

    virtual ~IMarketData() = default;

    const MarketState<depth, numberOfSymbols>& currentMarketState() const { return marketState_; }

    bool nextMarketState() {
        // Use a while loop to skip empty files without using the stack
        while (currentQuoteIndex_ >= quotes_.size()) {
            if (!multipleFiles_ || (currentFileIndex + 1) >= marketDataFilePaths_.size()) {
                return false;
            }

            currentFileIndex++;
            loadData(currentFileIndex);
            currentQuoteIndex_ = 0;

            // Loop continues if loadQuotes resulted in an empty quotes_ vector
        }

        const Quote<depth>& nextQuote = quotes_[currentQuoteIndex_++];
        std::uint16_t symbolWithNextUpdate = nextQuote.symbolId;
        marketState_[symbolWithNextUpdate] = nextQuote;
        marketState_.timestamp = nextQuote.timestamp;  // Update timestamp from current quote
        return true;
    }

    const Quote<depth>& getQuote(std::uint16_t symbol) { return marketState_.getQuote(symbol); }

    std::size_t getCurrentIndex() const { return currentQuoteIndex_; }

    const std::array<Ticks, numberOfSymbols> bestBids() { return marketState_.getBestBids(); }

    const std::array<Ticks, numberOfSymbols> bestAsks() { return marketState_.getBestAsks(); }

    Ticks bestBid(std::uint16_t symbolId) { return marketState_.bestBid(symbolId); }

    Ticks bestAsk(std::uint16_t symbolId) { return marketState_.bestAsk(symbolId); }

    Ticks getAsk(std::uint16_t symbolId, std::size_t level) {
        return marketState_.getAsk(level, symbolId);
    }

    Ticks getBid(std::uint16_t symbolId, std::size_t level) {
        return marketState_.getBid(level, symbolId);
    }

    const std::array<Ticks, depth> bestBidSizes(std::uint16_t symbolId) {
        return marketState_.getBidSizes(symbolId);
    }

    const std::array<Ticks, depth> getAskSizes(std::uint16_t symbolId) {
        return marketState_.getAskSizes(symbolId);
    }

    Ticks getAskSize(std::uint16_t symbolId, std::size_t level) {
        return marketState_.getAskSize(level, symbolId);
    }

    Ticks getBidSize(std::uint16_t symbolId, std::size_t level) {
        return marketState_.getBidSize(level, symbolId);
    }

    TimeStamp currentTimeStamp() { return marketState_.timestamp; }

   protected:
    bool loadData(std::size_t fileIndex) {
        return loadData(marketDataFilePaths_[currentFileIndex]);
    }

    virtual bool loadData() = 0;
    virtual bool loadData(const std::string& marketDataFilePath) = 0;

    const std::string marketDataFilePath_;
    const std::vector<std::string> marketDataFilePaths_;
    std::vector<Quote<depth>> quotes_;
    std::size_t currentQuoteIndex_{0};
    MarketState<depth, numberOfSymbols> marketState_;
    bool multipleFiles_;

    std::size_t currentFileIndex;
};

template <std::size_t depth, std::uint16_t numberOfSymbols>
class MarketDataParquet : public IMarketData<depth, numberOfSymbols> {
   public:
    MarketDataParquet(const std::string& marketDataFilePath)
        : IMarketData<depth, numberOfSymbols>(marketDataFilePath, false) {
        loadData(marketDataFilePath);
    }

    MarketDataParquet(const std::vector<std::string>& marketDataFilePaths)
        : IMarketData<depth, numberOfSymbols>(marketDataFilePaths, true) {
        loadData(marketDataFilePaths[0]);
    }

   protected:
    bool loadData() override;
    bool loadData(const std::string& marketDataFilePath) override;
};

}  // namespace sim
