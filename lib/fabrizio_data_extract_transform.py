import duckdb
import os
import glob

# --- Configuration ---
RAW_INPUT_DIR = '/mnt/klmncap/mbp10_database/nyse'
BASE_OUTPUT_DIR = '/mnt/klmncap3/tmp_simulation_data'
SYMBOLS = ['AAPL', 'TSLA', 'NVDA', 'MSFT']
SCALE_FACTOR = 10_000
# ---------------------

def run_pipeline(con, input_path, symbol, output_path, scale):
    """
    Extracts and Transforms in ONE step. 
    No temporary files are created, so no deletions are needed.
    """
    query = f"""
        COPY (
            SELECT * REPLACE (
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
            )
            FROM read_parquet('{input_path}')
            WHERE symbol = '{symbol}'
        ) TO '{output_path}' (FORMAT PARQUET);
    """
    con.execute(query)


def main():
    con = duckdb.connect()
    
    if not os.path.exists(BASE_OUTPUT_DIR):
        os.makedirs(BASE_OUTPUT_DIR)

    search_pattern = os.path.join(RAW_INPUT_DIR, "????-??-??.parquet")
    input_files = glob.glob(search_pattern)

    for file_path in input_files:
        date_str = os.path.splitext(os.path.basename(file_path))[0]
        print(f"Processing date: {date_str}")

        for symbol in SYMBOLS:
            final_file = os.path.join(BASE_OUTPUT_DIR, f"ubigint_{symbol}_{date_str}.parquet")

            try:
                print(f"  -> Processing {symbol}...")
                run_pipeline(con, file_path, symbol, final_file, SCALE_FACTOR)
            except Exception as e:
                print(f"  !! Error with {symbol} on {date_str}: {e}")

if __name__ == "__main__":
    main()

