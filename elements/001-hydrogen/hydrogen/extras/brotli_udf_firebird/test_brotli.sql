-- Firebird Brotli Decompression UDR Test
--
-- This test file verifies the BROTLI_DECOMPRESS UDR function.
-- Requires: brotli_decfn UDR installed in Firebird UDR plugin directory
-- Usage:     isql-fb -user SYSDBA -password <pass> hydrogen_test.fdb < test_brotli.sql
--
-- Test data uses the same Brotli quality 11 compressed samples as the
-- PostgreSQL, MySQL, and SQLite extras for cross-engine consistency.

-- Register the UDR function (if not already created by migration 1000)
CREATE OR ALTER FUNCTION BROTLI_DECOMPRESS(compressed BLOB)
RETURNS BLOB
EXTERNAL NAME 'brotli_decfn!brotli_decompress'
ENGINE UDR;

-- Test 1: Short string decompression (12 bytes: "Hello World!")
-- Same base64-encoded Brotli data as PostgreSQL/SQLite extras
SELECT '=== Test 1: Short String ===' AS test FROM RDB$DATABASE;

SELECT
    CAST(BROTLI_DECOMPRESS(BASE64_DECODE('jwWASGVsbG8gV29ybGQhAw==')) AS VARCHAR(100))
    AS decompressed_short
FROM RDB$DATABASE;

-- Verify length
SELECT
    CHAR_LENGTH(CAST(BROTLI_DECOMPRESS(BASE64_DECODE('jwWASGVsbG8gV29ybGQhAw==')) AS VARCHAR(100)))
    AS length_short
FROM RDB$DATABASE;

-- Test 2: Medium string (JSON config, ~97 bytes)
-- This verifies Brotli decompression of larger data
SELECT '=== Test 2: Medium String (JSON) ===' AS test FROM RDB$DATABASE;

-- Base64-encoded Brotli quality 11 compressed JSON
-- Original: {"server":{"port":8080,"host":"localhost"},"features":["brotli","json"],"version":"1.0.0"}
SELECT
    CAST(BROTLI_DECOMPRESS(
        BASE64_DECODE(
            'G2AAUI3TFfOihHUfodIJmlvql4e87VO3GURFfHAzrXVYuvo6xoHDYDcPAw3iA18MVr3wM9REk8IUFxkZzVl9oIEfXWz2r8Zzyd3rlX8caRmFdQl0'
        )
    ) AS VARCHAR(200))
    AS decompressed_medium
FROM RDB$DATABASE;

-- Test 3: NULL handling
SELECT '=== Test 3: NULL Handling ===' AS test FROM RDB$DATABASE;

SELECT BROTLI_DECOMPRESS(NULL) FROM RDB$DATABASE;

-- Test 4: Invalid data (should raise an error)
SELECT '=== Test 4: Invalid Data ===' AS test FROM RDB$DATABASE;

-- Not valid Brotli data — expect error
-- This is expected to fail; comment out if running in strict mode
-- SELECT BROTLI_DECOMPRESS(BASE64_DECODE('notValidBrotliData==')) FROM RDB$DATABASE;

COMMENT ON TEST 'Test 4 intentionally skipped in automated runs; Brotli error handling is verified via Phase 7 Test 37';
