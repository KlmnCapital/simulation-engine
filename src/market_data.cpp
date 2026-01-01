// market_data.cpp
module;

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

module simulation_engine;

import std;

namespace sim {
    /*---------------------------------------------------*/
    /*      SingleSymbolMarketDataParquet methods        */
    /*---------------------------------------------------*/
    template<std::size_t depth>
    SingleSymbolMarketDataParquet<depth>::SingleSymbolMarketDataParquet(
         const std::string& marketDataFilePath,
         const std::unordered_map<std::string, SymbolId>& symbolIdMap)
        : IMarketData<depth>(marketDataFilePath, false){
        loadQuotes(marketDataFilePath);
    }

    template<std::size_t depth>
    SingleSymbolMarketDataParquet<depth>::SingleSymbolMarketDataParquet(
         const std::vector<std::string>& marketDataFilePaths,
         const std::unordered_map<std::string, SymbolId>& symbolIdMap)
        : IMarketData<depth>(marketDataFilePaths, true) {
        loadQuotes(marketDataFilePaths[0]);
    }

    template<std::size_t depth>
    bool SingleSymbolMarketDataParquet<depth>::loadQuotes() {
        if (!this->multipleFiles_) {
            loadQuotes(this->marketDataFilePath_);
            return true;
        }

        if (this->currentFileIndex >= this->marketDataFilePaths_.size()) {
            return false;
        }
       loadQuotes(this->marketDataFilePaths_[this->currentFileIndex]);
        return true;
    }

    template <std::size_t depth>
    bool SingleSymbolMarketDataParquet<depth>::loadQuotes(const std::string& marketDataFilePath) {
        this->quotes_.clear();
        arrow::MemoryPool* pool = arrow::default_memory_pool();

        // Open File
        std::shared_ptr<arrow::io::ReadableFile> input;
        auto open_res = arrow::io::ReadableFile::Open(marketDataFilePath);
        if (!open_res.ok()) return false;
        input = open_res.ValueOrDie();

        auto reader_result = parquet::arrow::OpenFile(input, pool);
        if (!reader_result.ok()) return false;
        std::unique_ptr<parquet::arrow::FileReader> arrow_reader = std::move(reader_result).ValueOrDie();

        // Read the Table 
        std::shared_ptr<arrow::Table> table;
        if (!arrow_reader->ReadTable(&table).ok()) return false;

        // Ensure all chunks are merged so chunk(0) contains all rows
        auto combine_res = table->CombineChunks();
        if (!combine_res.ok()) return false;
        table = combine_res.ValueOrDie();

        const std::int64_t num_rows = table->num_rows();
        if (num_rows == 0) return true;
        this->quotes_.reserve(num_rows); 

        // Pre-fetch Column Pointers for 'depth' levels
        auto rtype_col = std::static_pointer_cast<arrow::Int8Array>(table->GetColumnByName("rtype")->chunk(0));
        auto ts_raw_col = table->GetColumnByName("ts_event");
        auto ts_col = std::static_pointer_cast<arrow::TimestampArray>(ts_raw_col->chunk(0));

        // Determine if we need to scale Micros to Nanos
        auto ts_type = std::static_pointer_cast<arrow::TimestampType>(ts_raw_col->type());
        int64_t ts_multiplier = (ts_type->unit() == arrow::TimeUnit::MICRO) ? 1000 : 1;

        std::array<std::shared_ptr<arrow::Int64Array>, depth> bid_px_cols;
        std::array<std::shared_ptr<arrow::Int64Array>, depth> ask_px_cols;
        std::array<std::shared_ptr<arrow::UInt32Array>, depth> bid_sz_cols;
        std::array<std::shared_ptr<arrow::UInt32Array>, depth> ask_sz_cols;

        for (std::size_t level = 0; level < depth; ++level) {
            // Formats index as 00, 01, 02... to match Databento Parquet naming
            std::string levelIndex = (level < 10) ? "0" + std::to_string(level) : std::to_string(level);

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
            quote.timestamp = TimeStamp{static_cast<std::uint64_t>(ts_col->Value(row) * ts_multiplier)};

            // Compiler unrolls this loop because depth is known at compile time
            for (std::size_t level = 0; level < depth; ++level) {
                quote.columns[level] = Ticks{bid_px_cols[level]->Value(row)};
                quote.columns[depth + level] = Ticks{ask_px_cols[level]->Value(row)};
                quote.columns[2 * depth + level] = Ticks{static_cast<std::int64_t>(bid_sz_cols[level]->Value(row))};
                quote.columns[3 * depth + level] = Ticks{static_cast<std::int64_t>(ask_sz_cols[level]->Value(row))};
            }

            this->quotes_.push_back(std::move(quote));
        }

        return true;
    }

    // Explicit template instantiations for common depths
    template class SingleSymbolMarketDataParquet<1>;
    template class SingleSymbolMarketDataParquet<5>;
    template class SingleSymbolMarketDataParquet<10>;

}  // namespace sim
