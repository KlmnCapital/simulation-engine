import duckdb
import os
import glob

INPUT_DIR = "/mnt/klmncap3/tmp_simulation_data"
OUTPUT_DIR = "/mnt/klmncap3/tmp_simulation_data_indexed"
SYMBOLS = ["AAPL", "TSLA", "NVDA", "MSFT"]

# Define your ID mapping here
SYMBOL_TO_ID = {symbol: idx for idx, symbol in enumerate(SYMBOLS)}


def add_symbol_id(con, input_path, output_path, mapping):
    """
    Reads the parquet file, injects the symbol_id based on the mapping,
    and casts it to UINT16.
    """
    # Build the CASE statement dynamically
    case_parts = [f"WHEN symbol = '{s}' THEN {i}" for s, i in mapping.items()]
    case_statement = f"CASE {' '.join(case_parts)} ELSE NULL END::USMALLINT"

    query = f"""
        COPY (
            SELECT 
                *,
                {case_statement} AS symbol_id
            FROM read_parquet('{input_path}')
        ) TO '{output_path}' (FORMAT PARQUET);
    """
    con.execute(query)


def main():
    con = duckdb.connect()

    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    # Search for the files created by your previous script
    search_pattern = os.path.join(INPUT_DIR, "ubigint_*.parquet")
    input_files = glob.glob(search_pattern)

    if not input_files:
        print(f"No files found in {INPUT_DIR} matching 'ubigint_*.parquet'")
        return

    for file_path in input_files:
        filename = os.path.basename(file_path)
        output_path = os.path.join(OUTPUT_DIR, filename)

        print(f"Processing: {filename}...")
        try:
            add_symbol_id(con, file_path, output_path, SYMBOL_TO_ID)
        except Exception as e:
            print(f"  !! Error processing {filename}: {e}")

    print(f"\nTask complete. Files are located in: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
