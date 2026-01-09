import duckdb
import os

INPUT_DIR = "/mnt/klmncap3/tmp_simulation_data"
OUTPUT_DIR = "/mnt/klmncap3/tmp_simulation_data_indexed"
OUTPUT_FILENAME = "combined_indexed_data.parquet"
SYMBOLS = ["AAPL", "TSLA", "NVDA", "MSFT"]

# Define ID mapping
SYMBOL_TO_ID = {symbol: idx for idx, symbol in enumerate(SYMBOLS)}


def consolidate_and_index(con, input_pattern, output_path, mapping):
    """
    Reads all parquet files matching the pattern, injects symbol_id,
    and writes everything into a single output file.
    """
    # Build the CASE statement dynamically
    case_parts = [f"WHEN symbol = '{s}' THEN {i}" for s, i in mapping.items()]
    case_statement = f"CASE {' '.join(case_parts)} ELSE NULL END::USMALLINT"

    # DuckDB handles globbing (wildcards) directly within read_parquet
    query = f"""
        COPY (
            SELECT 
                *,
                {case_statement} AS symbol_id
            FROM read_parquet('{input_pattern}')
        ) TO '{output_path}' (FORMAT PARQUET);
    """

    print(f"Executing consolidation to: {output_path}...")
    con.execute(query)


def main():
    con = duckdb.connect()

    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    # Use a glob pattern for all relevant parquet files
    input_pattern = os.path.join(INPUT_DIR, "ubigint_*.parquet")
    output_path = os.path.join(OUTPUT_DIR, OUTPUT_FILENAME)

    try:
        consolidate_and_index(con, input_pattern, output_path, SYMBOL_TO_ID)
        print(f"\nTask complete. Combined file is located at: {output_path}")
    except Exception as e:
        print(f"!! Error during processing: {e}")


if __name__ == "__main__":
    main()
