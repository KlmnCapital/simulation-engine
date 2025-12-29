// market_data.cpp
module;

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

module simulation_engine;

import std;

namespace sim {
    /*--------------------------------------*/
    /*      IMarketDataSource methods       */
    /*--------------------------------------*/
    template <std::size_t depth>
    const Quote<depth>& IMarketData<depth>::currentQuote() const {
        return currentQuote_;
    }

    template <std::size_t depth>
    bool IMarketData<depth>::nextQuote() {
        if (currentQuoteIndex_ >= quotes_.size()) {
                return false;
        }
        currentQuote_ = quotes_[++currentQuoteIndex_];
        return true;
    }

    template <std::size_t depth>
    bool IMarketData<depth>::goToQuote(TimeStamp targetTimestamp) {
        // Find the first quote with timestamp >= targetTimestamp
        auto foundQuote = std::lower_bound(quotes_.begin(), quotes_.end(), targetTimestamp,
            [](const Quote<depth>& quote, TimeStamp timestamp) {
                return quote.timestamp < timestamp;
        });

        // Check if we found an exact match
        if (foundQuote != quotes_.end() && foundQuote->timestamp == targetTimestamp) {
            currentQuoteIndex_ = std::distance(quotes_.begin(), foundQuote);
            currentQuote_ = *foundQuote;
            return true;
        }

        // Timestamp not found
        return false;
    }

    template <std::size_t depth>
    void IMarketData<depth>::reset() {
        currentQuoteIndex_ = 0;
        if (!quotes_.empty()) {
            currentQuote_ = quotes_[currentQuoteIndex_];
        }
    }

    template <std::size_t depth>
    size_t IMarketData<depth>::getNumQuotes() const {
        return quotes_.size(); 
    }

    template <std::size_t depth>
    size_t IMarketData<depth>::getCurrentIndex() const {
        return currentQuoteIndex_;
    }

    /*---------------------------------------------------*/
    /*      SingleSymbolMarketDataParquet methods        */
    /*---------------------------------------------------*/
    template<std::size_t depth>
    SingleSymbolMarketDataParquet<depth>::SingleSymbolMarketDataParquet(const std::string& symbol,
        const std::unordered_map<std::string, SymbolId>& symbolIdMap,
        const std::string& startDate,
        const std::string& endDate,
        const std::string& parquetFilePath)
        : symbol_{symbolIdMap.at(symbol)},
        endDate_{endDate},
        startDate_{startDate},
        parquetFilePath_{parquetFilePath} {
        loadQuotes();
    }

    template <std::size_t depth>
    bool SingleSymbolMarketDataParquet<depth>::loadQuotes() { return loadQuotes(parquetFilePath_); }

    template <std::size_t depth>
    bool SingleSymbolMarketDataParquet<depth>::loadQuotes(const std::string& parquetFilePath) {
        arrow::MemoryPool* pool = arrow::default_memory_pool();

        // Open File
        std::shared_ptr<arrow::io::ReadableFile> input;
        auto open_res = arrow::io::ReadableFile::Open(parquetFilePath);
        if (!open_res.ok()) return false;
        input = open_res.ValueOrDie();

        auto reader_result = parquet::arrow::OpenFile(input, pool);
        if (!reader_result.ok()) return false;
        std::unique_ptr<parquet::arrow::FileReader> arrow_reader = std::move(reader_result).ValueOrDie();

        // Read the Table 
        std::shared_ptr<arrow::Table> table;
        if (!arrow_reader->ReadTable(&table).ok()) return false;

        const std::int64_t num_rows = table->num_rows();
        this->quotes_.reserve(num_rows); 

        // Pre-fetch Column Pointers for N levels
        auto rtype_col = std::static_pointer_cast<arrow::Int8Array>(table->GetColumnByName("rtype")->chunk(0));
        auto ts_col = std::static_pointer_cast<arrow::Int64Array>(table->GetColumnByName("ts_event")->chunk(0));

        std::array<std::shared_ptr<arrow::Int64Array>, depth> bid_px_cols;
        std::array<std::shared_ptr<arrow::Int64Array>, depth> ask_px_cols;
        std::array<std::shared_ptr<arrow::UInt32Array>, depth> bid_sz_cols;
        std::array<std::shared_ptr<arrow::UInt32Array>, depth> ask_sz_cols;

        for (std::size_t level = 0; level < depth; ++level) {
            // Formats index as 00, 01, 02... to match Databento Parquet naming
            std::string levelIndex = (level < depth) ? "0" + std::to_string(level) : std::to_string(level);

            bid_px_cols[level] = std::static_pointer_cast<arrow::Int64Array>(table->GetColumnByName("bid_px_" + levelIndex)->chunk(0));
            ask_px_cols[level] = std::static_pointer_cast<arrow::Int64Array>(table->GetColumnByName("ask_px_" + levelIndex)->chunk(0));
            bid_sz_cols[level] = std::static_pointer_cast<arrow::UInt32Array>(table->GetColumnByName("bid_sz_" + levelIndex)->chunk(0));
            ask_sz_cols[level] = std::static_pointer_cast<arrow::UInt32Array>(table->GetColumnByName("ask_sz_" + levelIndex)->chunk(0));
        }

        std::size_t totalRecords = 0;
        std::size_t crossedQuotes = 0;

        // Main Processing Loop
        for (std::int64_t row = 0; row < num_rows; ++row) {
            // Check that there are *depth* levels
            if (rtype_col->Value(row) != depth) continue;

            totalRecords++;

            // Use Top-of-Book for validation
            std::int64_t level1_bid = bid_px_cols[0]->Value(row);
            std::int64_t level1_ask = ask_px_cols[0]->Value(row);

            if (level1_bid >= level1_ask || level1_bid <= 0) {
                crossedQuotes++;
                continue;
            }

            Quote<depth> quote;
            quote.timestamp = TimeStamp{static_cast<std::uint64_t>(ts_col->Value(row))};

            // Compiler unrolls this loop because depth is known at compile time
            for (std::size_t level = 0; level < depth; ++level) {
                quote.columns[level]           = Ticks{bid_px_cols[level]->Value(row)};
                quote.columns[depth + level]       = Ticks{ask_px_cols[level]->Value(row)};
                quote.columns[2 * depth + level]   = Ticks{static_cast<std::int64_t>(bid_sz_cols[level]->Value(row))};
                quote.columns[3 * depth + level]   = Ticks{static_cast<std::int64_t>(ask_sz_cols[level]->Value(row))};
            }

            this->quotes_.push_back(std::move(quote));
        }

        return true;
    }

    // Explicit template instantiations for common depths
    template class IMarketData<1>;
    template class IMarketData<5>;
    template class IMarketData<10>;

    template class SingleSymbolMarketDataParquet<1>;
    template class SingleSymbolMarketDataParquet<5>;
    template class SingleSymbolMarketDataParquet<10>;

}  // namespace sim
