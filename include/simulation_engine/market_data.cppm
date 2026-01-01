// market_data.cppm
export module simulation_engine:market_data;

import :quote;
import :types;

import datetime;

import std;

export namespace sim {

    enum class Source : std::uint8_t {
        DATABENTO = 0,
        CHARLES_SCHWAB = 1 
    };

    enum class Service : std::uint8_t { NYSE_BOOK = 0, NASDAQ_BOOK = 1 };

    constexpr const char* toString(Source source) {
        switch (source) {
            case Source::DATABENTO:
                return "dbn";
            case Source::CHARLES_SCHWAB:
                return "schwab";
            default:
                return "unknown";
        }
    }

    constexpr const char* toString(Service service) {
        switch (service) {
            case Service::NYSE_BOOK:
                return "NYSE_BOOK";
            case Service::NASDAQ_BOOK:
                return "NASDAQ_BOOK";
            default:
                return "unknown";
        }
    }

    template <std::size_t depth>
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

        const Quote<depth>& currentQuote() const {
            return currentQuote_;
        }

        bool nextQuote() {
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

            currentQuote_ = quotes_[currentQuoteIndex_++];
            return true;
        }

        std::size_t getCurrentIndex() const {
            return currentQuoteIndex_;
        }

       protected:
        bool loadQuotes(std::size_t fileIndex) {
            return loadQuotes(marketDataFilePaths_[currentFileIndex]);
        }

        virtual bool loadQuotes() = 0;
        virtual bool loadQuotes(const std::string& marketDataFilePath) = 0;

        const std::string marketDataFilePath_;
        const std::vector<std::string> marketDataFilePaths_;
        std::vector<Quote<depth>> quotes_;
        std::size_t currentQuoteIndex_{0};
        Quote<depth> currentQuote_;
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
        bool loadQuotes() override;
        bool loadQuotes(const std::string& marketDataFilePath) override;
    };

// Explicit template instantiations for common depths
template class IMarketData<1>;
template class IMarketData<5>;
template class IMarketData<10>;

}  // namespace sim

