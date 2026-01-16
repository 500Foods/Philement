-- SQLite Convert TZ Test
-- This test file demonstrates the convert_tz function
-- Shows current time and timezone conversion

-- Load the convert_tz extension
.load ./convert_tz

-- Test 1: Extension loading
SELECT '=== Test 1: Extension Loading ===' AS test;
SELECT 'Extension loaded successfully' AS status;

-- Test 2: Current UTC time
SELECT '=== Test 2: Current UTC Time ===' AS test;
SELECT datetime('now') AS current_utc_time;

-- Test 3: Convert current time to America/Vancouver
SELECT '=== Test 3: Convert to America/Vancouver ===' AS test;
SELECT datetime(convert_tz(datetime('now'),'UTC','America/Vancouver')) AS vancouver_time;

-- Test 4: Basic timezone conversion examples
SELECT '=== Test 4: Basic Conversion Examples ===' AS test;
SELECT convert_tz('2024-01-01 12:00:00', 'UTC', 'America/New_York') AS utc_to_ny;
SELECT convert_tz('2024-07-01 15:30:00', 'America/New_York', 'Europe/London') AS ny_to_london;
SELECT convert_tz('2024-12-25 00:00:00', 'UTC', 'Asia/Tokyo') AS utc_to_tokyo;

-- Test 5: Millisecond precision
SELECT '=== Test 5: Millisecond Precision ===' AS test;
SELECT convert_tz('2024-01-01 12:00:00.123', 'UTC', 'America/Vancouver') AS with_milliseconds;

-- Test 6: NULL handling
SELECT '=== Test 6: NULL Handling ===' AS test;
SELECT convert_tz(NULL, 'UTC', 'America/Vancouver') AS null_input;
SELECT convert_tz('2024-01-01 12:00:00', NULL, 'America/Vancouver') AS null_from_tz;
SELECT convert_tz('2024-01-01 12:00:00', 'UTC', NULL) AS null_to_tz;