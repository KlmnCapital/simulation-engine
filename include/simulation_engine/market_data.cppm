// market_data.cppm
export module simulation_engine:market_data_multi_symbol;

import :quote;
import :types;

import datetime;

import std;

export namespace sim {
    template <std::size_t depth, std::size_t numberOfSymbols>
    class IMarketDataMultiSymbol {
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

        const Quote<depth>& currentMarketState() const {
            return marketState_;
        }

        bool nextMarketState() {
            // Use a while loop to skip empty files without using the stack
            while (currentQuoteIndex_ >= quotes_.size()) {
                if (!multipleFiles_ || (currentFileIndex + 1) >= marketDataFilePaths_.size()) {
                    return false; 
                }

                currentFileIndex++;
                loadQuotes(currentFileIndex);
                currentQuoteIndex_ = 0;

                // Loop continues if loadQuotes resulted in an empty quotes_ vector
            }

            std::size_t SymbolId symbolWithNextUpdate = std::static_cast<std::size_t>(quotes_[currentQuoteIndex_++]);
            marketState_[symbolId] = quotes_[currentQuoteIndex_];
            return true;
        }

        std::size_t getCurrentIndex() const {
            return currentQuoteIndex_;
        }

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


    template<std::size_t depth>
    class SingleSymbolMarketDataParquet : public IMarketData<depth> {
       public:
        SingleSymbolMarketDataParquet(
            const std::string& marketDataFilePath,
            const std::unordered_map<std::string, SymbolId>& symbolIdMap);

        SingleSymbolMarketDataParquet(
            const std::vector<std::string>& marketDataFilePaths,
            const std::unordered_map<std::string, SymbolId>& symbolIdMap);

       protected:
        bool loadData() override;
        bool loadData(const std::string& marketDataFilePath) override;
    };

// Explicit template instantiations for common depths
template class IMarketData<1>;
template class IMarketData<5>;
template class IMarketData<10>;

}  // namespace sim

