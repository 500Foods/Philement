-- install_brotli_decompress.clp
-- DB2 CLP script to register Brotli decompression UDFs
-- Called via Makefile with schema substitution

-- Create the decompression UDF (accepts BLOB from BASE64DECODEBINARY)
-- Input: BLOB containing compressed binary data (from BASE64DECODEBINARY)
-- Output: CLOB containing decompressed text data
CREATE OR REPLACE FUNCTION ${SCHEMA}.BROTLI_DECOMPRESS(compressed BLOB(2G))
RETURNS CLOB(2G)
LANGUAGE C
PARAMETER STYLE DB2SQL
NO SQL
DETERMINISTIC
NOT FENCED
THREADSAFE
RETURNS NULL ON NULL INPUT
EXTERNAL NAME 'brotli_decompress.so!BROTLI_DECOMPRESS'
@

-- Combined Base64 decode + Brotli decompress function
-- This is the recommended function for decompressing base64-encoded brotli data
-- as it avoids passing binary data through SQL
-- Input: CLOB containing base64-encoded compressed data
-- Output: CLOB containing decompressed text data
CREATE OR REPLACE FUNCTION ${SCHEMA}.BASE64_BROTLI_DECOMPRESS(encoded CLOB(2G))
RETURNS CLOB(2G)
LANGUAGE C
PARAMETER STYLE DB2SQL
NO SQL
DETERMINISTIC
NOT FENCED
THREADSAFE
RETURNS NULL ON NULL INPUT
EXTERNAL NAME 'brotli_decompress.so!BASE64_BROTLI_DECOMPRESS'
@

-- Create test function
CREATE OR REPLACE FUNCTION ${SCHEMA}.HELIUM_BROTLI_CHECK()
RETURNS VARCHAR(20)
LANGUAGE C
PARAMETER STYLE DB2SQL
NO SQL
DETERMINISTIC
NOT FENCED
THREADSAFE
RETURNS NULL ON NULL INPUT
EXTERNAL NAME 'brotli_decompress.so!HELIUM_BROTLI_CHECK'
@

-- Test the functions
VALUES ${SCHEMA}.HELIUM_BROTLI_CHECK()
@