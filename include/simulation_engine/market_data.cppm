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
        IMarketData() = default;
        virtual ~IMarketData() = default;

        const Quote<depth>& currentQuote() const;
        bool nextQuote();
        bool goToQuote(TimeStamp targetTimestamp);
        void reset();
        std::size_t getNumQuotes() const;
        std::size_t getCurrentIndex() const;

    protected:
        virtual bool loadQuotes() = 0;
        std::vector<Quote<depth>> quotes_;
        std::size_t currentQuoteIndex_{0};
        Quote<depth> currentQuote_;
    };

    template<std::size_t depth>
    class SingleSymbolMarketDataParquet : public IMarketData<depth> {
    public:
        SingleSymbolMarketDataParquet(const std::string& symbol,
            const std::unordered_map<std::string, SymbolId>& symbolIdMap,
            const std::string& startDate,
            const std::string& endDate,
            const std::string& parquetFilePath);

    protected:
        bool loadQuotes() override;
        bool loadQuotes(const std::string& parquetFilePath);

    private:
        const std::string& parquetFilePath_;
        SymbolId symbol_;
        datetime::Date startDate_;
        datetime::Date endDate_;
    };

}  // namespace sim
