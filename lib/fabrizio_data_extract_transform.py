import duckdb
import os
import glob

# --- Configuration ---
RAW_INPUT_DIR = "/mnt/klmncap/mbp10_database/nyse"
BASE_OUTPUT_DIR = (
    "/mnt/klmncap3/tmp_multisymbol_simulation_data/simulation_data_indexed"
)
SYMBOLS = ["AAPL", "TSLA", "NVDA", "MSFT"]
SCALE_FACTOR = 10_000

# Define ID mapping for the CASE statement
SYMBOL_TO_ID = {symbol: idx for idx, symbol in enumerate(SYMBOLS)}
# ---------------------


def process_daily_file(con, input_path, output_path, symbols, mapping, scale):
    """
    Transforms one raw date file into one indexed date file.
    Filters for target symbols, scales prices, adds symbol_id,
    and sorts chronologically.
    """
    # Build the CASE statement for symbol_id
    case_parts = [f"WHEN symbol = '{s}' THEN {i}" for s, i in mapping.items()]
    case_statement = f"CASE {' '.join(case_parts)} ELSE NULL END::USMALLINT"

    # Build the symbols list for the WHERE clause
    symbol_list_str = ", ".join([f"'{s}'" for s in symbols])

    query = f"""
        COPY (
            SELECT 
                * REPLACE (
                    (price * {scale})::UBIGINT AS price,
                    (bid_px_00 * {scale})::UBIGINT AS bid_px_00,
                    (ask_px_00 * {scale})::UBIGINT AS ask_px_00,
                    (bid_px_01 * {scale})::UBIGINT AS bid_px_01,
                    (ask_px_01 * {scale})::UBIGINT AS ask_px_01,
                    (bid_px_02 * {scale})::UBIGINT AS bid_px_02,
                    (ask_px_02 * {scale})::UBIGINT AS ask_px_02,
                    (bid_px_03 * {scale})::UBIGINT AS bid_px_03,
                    (ask_px_03 * {scale})::UBIGINT AS ask_px_03,
                    (bid_px_04 * {scale})::UBIGINT AS bid_px_04,
                    (ask_px_04 * {scale})::UBIGINT AS ask_px_04,
                    (bid_px_05 * {scale})::UBIGINT AS bid_px_05,
                    (ask_px_05 * {scale})::UBIGINT AS ask_px_05,
                    (bid_px_06 * {scale})::UBIGINT AS bid_px_06,
                    (ask_px_06 * {scale})::UBIGINT AS ask_px_06,
                    (bid_px_07 * {scale})::UBIGINT AS bid_px_07,
                    (ask_px_07 * {scale})::UBIGINT AS ask_px_07,
                    (bid_px_08 * {scale})::UBIGINT AS bid_px_08,
                    (ask_px_08 * {scale})::UBIGINT AS ask_px_08,
                    (bid_px_09 * {scale})::UBIGINT AS bid_px_09,
                    (ask_px_09 * {scale})::UBIGINT AS ask_px_09
                ),
                {case_statement} AS symbol_id
            FROM read_parquet('{input_path}')
            WHERE symbol IN ({symbol_list_str})
            ORDER BY ts_event, symbol_id
        ) TO '{output_path}' (FORMAT PARQUET);
    """
    con.execute(query)


def main():
    con = duckdb.connect()

    # Configure DuckDB to use more memory if available for sorting
    con.execute("SET preserve_insertion_order=false;")

    if not os.path.exists(BASE_OUTPUT_DIR):
        os.makedirs(BASE_OUTPUT_DIR)

    # Match files like 2025-07-13.parquet
    search_pattern = os.path.join(RAW_INPUT_DIR, "202?-??-??.parquet")
    input_files = sorted(glob.glob(search_pattern))

    if not input_files:
        print(f"No files found in {RAW_INPUT_DIR}")
        return

    print(f"Found {len(input_files)} daily files. Starting processing...")

    for file_path in input_files:
        filename = os.path.basename(file_path)
        output_path = os.path.join(BASE_OUTPUT_DIR, f"indexed_{filename}")

        print(f" -> Processing and Sorting {filename}...")
        try:
            process_daily_file(
                con, file_path, output_path, SYMBOLS, SYMBOL_TO_ID, SCALE_FACTOR
            )
        except Exception as e:
            print(f"    !! Error processing {filename}: {e}")

    print(
        f"\nSuccess! One chronologically sorted file per date is in: {BASE_OUTPUT_DIR}"
    )


if __name__ == "__main__":
    main()
