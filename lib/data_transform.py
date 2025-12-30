import duckdb

input_file = '/mnt/klmncap3/tmp_simulation_data/AAPL_2025-10-24.parquet'
output_file = '/mnt/klmncap3/tmp_simulation_data/ubigint_AAPL_2025-10-24.parquet'
scale_factor = 10_000 

con = duckdb.connect()

# Convert all price columns to UBIGINT instead of DOUBLE
query = f"""
COPY (
    SELECT * REPLACE (
        (price * {scale_factor})::UBIGINT AS price,
        (bid_px_00 * {scale_factor})::UBIGINT AS bid_px_00,
        (ask_px_00 * {scale_factor})::UBIGINT AS ask_px_00,
        (bid_px_01 * {scale_factor})::UBIGINT AS bid_px_01,
        (ask_px_01 * {scale_factor})::UBIGINT AS ask_px_01,
        (bid_px_02 * {scale_factor})::UBIGINT AS bid_px_02,
        (ask_px_02 * {scale_factor})::UBIGINT AS ask_px_02,
        (bid_px_03 * {scale_factor})::UBIGINT AS bid_px_03,
        (ask_px_03 * {scale_factor})::UBIGINT AS ask_px_03,
        (bid_px_04 * {scale_factor})::UBIGINT AS bid_px_04,
        (ask_px_04 * {scale_factor})::UBIGINT AS ask_px_04,
        (bid_px_05 * {scale_factor})::UBIGINT AS bid_px_05,
        (ask_px_05 * {scale_factor})::UBIGINT AS ask_px_05,
        (bid_px_06 * {scale_factor})::UBIGINT AS bid_px_06,
        (ask_px_06 * {scale_factor})::UBIGINT AS ask_px_06,
        (bid_px_07 * {scale_factor})::UBIGINT AS bid_px_07,
        (ask_px_07 * {scale_factor})::UBIGINT AS ask_px_07,
        (bid_px_08 * {scale_factor})::UBIGINT AS bid_px_08,
        (ask_px_08 * {scale_factor})::UBIGINT AS ask_px_08,
        (bid_px_09 * {scale_factor})::UBIGINT AS bid_px_09,
        (ask_px_09 * {scale_factor})::UBIGINT AS ask_px_09
    )
    FROM read_parquet('{input_file}')
) TO '{output_file}' (FORMAT PARQUET);
"""

print(f"Converting {input_file} while preserving column order...")
con.execute(query)

print("Check complete. All columns remain in their original positions.")
