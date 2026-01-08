// market_data.cpp
module;

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

module simulation_engine;

import std;

namespace sim {

/*---------------------------------------*/
/*      MarketDataParquet methods        */
/*---------------------------------------*/
template<std::size_t depth, std::uint16_t numberOfSymbols>
MarketDataParquet<depth, numberOfSymbols>::MarketDataParquet(
        const std::string& marketDataFilePath)
    : IMarketData<depth, numberOfSymbols>(marketDataFilePath, false){
    loadData(marketDataFilePath);
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
MarketDataParquet<depth, numberOfSymbols>::MarketDataParquet(
        const std::vector<std::string>& marketDataFilePaths)
    : IMarketData<depth, numberOfSymbols>(marketDataFilePaths, true) {
    loadData(marketDataFilePaths[0]);
}

template<std::size_t depth, std::uint16_t numberOfSymbols>
bool MarketDataParquet<depth, numberOfSymbols>::loadData() {
    if (!this->multipleFiles_) {
        loadData(this->marketDataFilePath_);
        return true;
    }

    if (this->currentFileIndex >= this->marketDataFilePaths_.size()) {
        return false;
    }
    loadData(this->marketDataFilePaths_[this->currentFileIndex]);
    return true;
}

template <std::size_t depth, std::uint16_t numberOfSymbols>
bool MarketDataParquet<depth, numberOfSymbols>::loadData(const std::string& marketDataFilePath) {
    this->quotes_.clear();
    arrow::MemoryPool* pool = arrow::default_memory_pool();

    // Open File
    std::shared_ptr<arrow::io::ReadableFile> input;
    auto openResult = arrow::io::ReadableFile::Open(marketDataFilePath);
    if (!openResult.ok()) return false;
    input = openResult.ValueOrDie();

    auto readerResult = parquet::arrow::OpenFile(input, pool);
    if (!readerResult.ok()) return false;
    std::unique_ptr<parquet::arrow::FileReader> arrowReader = std::move(readerResult).ValueOrDie();

    // Read the Table 
    std::shared_ptr<arrow::Table> table;
    if (!arrowReader->ReadTable(&table).ok()) return false;

    // Ensure all chunks are merged
    auto combineResult = table->CombineChunks();
    if (!combineResult.ok()) return false;
    table = combineResult.ValueOrDie();

    const std::int64_t numRows = table->num_rows();
    if (numRows == 0) return true;
    this->quotes_.reserve(numRows); 

    // Fetch Column Pointers
    auto rowType = std::static_pointer_cast<arrow::Int8Array>(table->GetColumnByName("rtype")->chunk(0));

    auto symbolIdColumn = std::static_pointer_cast<arrow::UInt16Array>(table->GetColumnByName("symbol_id")->chunk(0));

    auto rawTimestampColumn = table->GetColumnByName("ts_event");
    auto timestampColumn = std::static_pointer_cast<arrow::TimestampArray>(rawTimestampColumn->chunk(0));

    // Determine scaling for timestamps
    auto timestampType = std::static_pointer_cast<arrow::TimestampType>(rawTimestampColumn->type());
    int64_t timestampMultiplier = (timestampType->unit() == arrow::TimeUnit::MICRO) ? 1000 : 1;

    // Pre-fetch Level-based Column Pointers
    std::array<std::shared_ptr<arrow::Int64Array>, depth> bidPriceColumn;
    std::array<std::shared_ptr<arrow::Int64Array>, depth> askPriceColumn;
    std::array<std::shared_ptr<arrow::UInt32Array>, depth> bidSizeColumn;
    std::array<std::shared_ptr<arrow::UInt32Array>, depth> askSizeColumn;

    for (std::size_t level = 0; level < depth; ++level) {
        std::string levelIndex = (level < 10) ? "0" + std::to_string(level) : std::to_string(level);

        bidPriceColumn[level] = std::static_pointer_cast<arrow::Int64Array>(table->GetColumnByName("bid_px_" + levelIndex)->chunk(0));
        askPriceColumn[level] = std::static_pointer_cast<arrow::Int64Array>(table->GetColumnByName("ask_px_" + levelIndex)->chunk(0));
        bidSizeColumn[level] = std::static_pointer_cast<arrow::UInt32Array>(table->GetColumnByName("bid_sz_" + levelIndex)->chunk(0));
        askSizeColumn[level] = std::static_pointer_cast<arrow::UInt32Array>(table->GetColumnByName("ask_sz_" + levelIndex)->chunk(0));
    }

    // Main Processing Loop
    for (std::int64_t row = 0; row < numRows; ++row) {
        if (rowType->Value(row) != depth) continue;

        std::int64_t levelOneBid = bidPriceColumn[0]->Value(row);
        std::int64_t levelOneAsk = askPriceColumn[0]->Value(row);

        if (levelOneBid >= levelOneAsk || levelOneBid <= 0) {
            continue;
        }

        Quote<depth> quote;

        quote.symbolId = symbolIdColumn->Value(row);
        quote.timestamp = TimeStamp{static_cast<std::uint64_t>(timestampColumn->Value(row) * timestampMultiplier)};

        for (std::size_t level = 0; level < depth; ++level) {
            quote.prices[level] = Ticks{bidPriceColumn[level]->Value(row)};
            quote.prices[depth + level] = Ticks{askPriceColumn[level]->Value(row)};
            quote.sizes[level] = Ticks{static_cast<std::int64_t>(bidSizeColumn[level]->Value(row))};
            quote.sizes[depth + level] = Ticks{static_cast<std::int64_t>(askSizeColumn[level]->Value(row))};
        }

        this->quotes_.push_back(std::move(quote));
    }

    return true;
}

// Explicit template instantiations for common depths
template class MarketDataParquet<10, 1>;
template class MarketDataParquet<10, 4>;

}  // namespace sim
