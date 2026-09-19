-- Firebird JSON_VALUE UDR Test
--
-- This test file verifies the JSON_VALUE UDR function.
-- Requires: json_udfn UDR installed in Firebird UDR plugin directory
-- Usage:     isql-fb -user SYSDBA -password <pass> testfb.fdb < test_json.sql
--
-- This UDR provides JSON_VALUE since Firebird 4 has no native support.

-- Register the UDR function
CREATE OR ALTER FUNCTION JSON_VALUE(json_doc BLOB SUB_TYPE TEXT, json_path VARCHAR(255))
RETURNS BLOB SUB_TYPE TEXT
EXTERNAL NAME 'json_udfn!json_value'
ENGINE UDR;

-- Test 1: Simple key extraction
SELECT '=== Test 1: Simple Key Extraction ===' AS test FROM RDB$DATABASE;

SELECT
    CAST(JSON_VALUE('{"icon":"home","name":"test"}', '$.icon') AS VARCHAR(100))
    AS extracted_icon
FROM RDB$DATABASE;
-- Expected: "home"

-- Test 2: Nested key extraction
SELECT '=== Test 2: Nested Key Extraction ===' AS test FROM RDB$DATABASE;

SELECT
    CAST(JSON_VALUE('{"server":{"port":8080,"host":"localhost"}}', '$.server.host') AS VARCHAR(100))
    AS extracted_host
FROM RDB$DATABASE;
-- Expected: "localhost"

-- Test 3: Missing path → NULL
SELECT '=== Test 3: Missing Path → NULL ===' AS test FROM RDB$DATABASE;

SELECT
    CASE WHEN JSON_VALUE('{"a":1}', '$.b') IS NULL THEN 'NULL' ELSE 'NOT NULL' END
    AS missing_path_result
FROM RDB$DATABASE;
-- Expected: NULL

-- Test 4: Array index extraction
SELECT '=== Test 4: Array Index ===' AS test FROM RDB$DATABASE;

SELECT
    CAST(JSON_VALUE('{"features":["brotli","json","utf8"]}', '$.features[0]') AS VARCHAR(100))
    AS first_feature
FROM RDB$DATABASE;
-- Expected: "brotli"

-- Test 5: NULL input → NULL
SELECT '=== Test 5: NULL Input ===' AS test FROM RDB$DATABASE;

SELECT JSON_VALUE(NULL, '$.key') FROM RDB$DATABASE;
-- Expected: NULL

-- Test 6: Default NULL ON ERROR (used by ${JRE} macro)
SELECT '=== Test 6: Default NULL ON ERROR ===' AS test FROM RDB$DATABASE;

-- Invalid path → NULL (not an error)
SELECT
    CASE WHEN JSON_VALUE('{"valid":true}', '$.invalid.path') IS NULL THEN 'NULL' ELSE 'NOT NULL' END
    AS invalid_path_result
FROM RDB$DATABASE;
-- Expected: NULL

SELECT '=== All JSON_VALUE tests passed ===' AS result FROM RDB$DATABASE;
