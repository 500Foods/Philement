-- schemahelper_decode_test.lua
-- Unit tests for decode_embedded and has_embed in schemahelper_queue.
-- Tests all dialect patterns: SQLite, MySQL, MariaDB, DB2, PostgreSQL.
--
-- CHANGELOG
-- 0.5.7 - 2026-08-24 - Initial tests for all dialect decode patterns.

-- luacheck: globals arg package

local function fail(msg)
	io.stderr:write("TEST FAIL: " .. tostring(msg) .. "\n")
	os.exit(1)
end

local function pass(msg)
	print("  ok: " .. tostring(msg))
end

local function check(cond, msg)
	if cond then
		pass(msg)
	else
		fail(msg)
	end
end

local script_dir
do
	local src = arg[0] or ""
	local d = src:match("^(.*)/[^/]+$") or "."
	script_dir = d
end
package.path = script_dir .. "/?.lua;" .. package.path

local queue = require("schemahelper_queue")

-- Pre-computed test data (generated via: printf 'hello world' | brotli -c | base64 -w0)
local test_plain = "hello world"
local b64_blob = "DwWAaGVsbG8gd29ybGQD"

-- Standalone base64 for CRYPTO_DECODE / FROM_BASE64 / etc.
local plain_b64 = "dGVzdA=="  -- "test"

-- =========================================================================
-- has_embed tests
-- =========================================================================
print("=== has_embed tests ===")

check(queue.has_embed("BROTLI_DECOMPRESS(CRYPTO_DECODE('" .. plain_b64 .. "'))"),
	"has_embed: SQLite brotli+base64")
check(queue.has_embed("BROTLI_DECOMPRESS(FROM_BASE64('" .. plain_b64 .. "'))"),
	"has_embed: MySQL brotli+base64 (uppercase)")
check(queue.has_embed("brotli_decompress(FROM_BASE64('" .. plain_b64 .. "'))"),
	"has_embed: MySQL/MariaDB brotli+base64 (lowercase)")
check(queue.has_embed("BROTLI_DECOMPRESS(BASE64DECODEBINARY('" .. plain_b64 .. "'))"),
	"has_embed: DB2 brotli+base64")
check(queue.has_embed("brotli_decompress(DECODE('" .. plain_b64 .. "'))"),
	"has_embed: PostgreSQL brotli+base64")
check(queue.has_embed("CONVERT_FROM(DECODE('" .. plain_b64 .. "', 'UTF8'))"),
	"has_embed: PostgreSQL CONVERT_FROM+DECODE")
check(queue.has_embed("CRYPTO_DECODE('" .. plain_b64 .. "')"),
	"has_embed: SQLite standalone base64")
check(queue.has_embed("BASE64DECODE('" .. plain_b64 .. "')"),
	"has_embed: DB2 standalone base64")
check(queue.has_embed("FROM_BASE64('" .. plain_b64 .. "')"),
	"has_embed: MySQL standalone base64")
check(queue.has_embed("brotli_decompress(FROM_BASE64('" .. plain_b64 .. "'))"),
	"has_embed: MySQL lowercase brotli+base64 with prefix")
check(queue.has_embed("schema.brotli_decompress(FROM_BASE64('" .. plain_b64 .. "'))"),
	"has_embed: MySQL lowercase brotli with schema prefix")

check(not queue.has_embed("SELECT 1 FROM users"),
	"has_embed: plain SQL returns false")
check(not queue.has_embed("INSERT INTO queue (id) VALUES (1)"),
	"has_embed: INSERT without encode returns false")

-- =========================================================================
-- decode_embedded tests
-- =========================================================================
print("=== decode_embedded tests ===")

-- MySQL lowercase brotli (the fix for this session)
local mysql_lower_result = queue.decode_embedded(
	"brotli_decompress(FROM_BASE64('" .. b64_blob .. "'))")
check(mysql_lower_result == test_plain,
	"decode_embedded: MySQL lowercase brotli(FROM_BASE64) -> '"
		.. test_plain .. "' got '" .. tostring(mysql_lower_result) .. "'")

-- MySQL uppercase brotli (existing pattern)
local mysql_upper_result = queue.decode_embedded(
	"BROTLI_DECOMPRESS(FROM_BASE64('" .. b64_blob .. "'))")
check(mysql_upper_result == test_plain,
	"decode_embedded: MySQL uppercase brotli(FROM_BASE64) -> '"
		.. test_plain .. "' got '" .. tostring(mysql_upper_result) .. "'")

-- MySQL lowercase brotli with schema prefix
local mysql_lower_prefix = queue.decode_embedded(
	"mysql.brotli_decompress(FROM_BASE64('" .. b64_blob .. "'))")
check(mysql_lower_prefix == test_plain,
	"decode_embedded: MySQL lowercase brotli with schema prefix -> '" .. test_plain .. "'")

-- SQLite brotli (existing pattern)
local sqlite_result = queue.decode_embedded(
	"BROTLI_DECOMPRESS(CRYPTO_DECODE('" .. b64_blob .. "'))")
check(sqlite_result == test_plain,
	"decode_embedded: SQLite BROTLI_DECOMPRESS(CRYPTO_DECODE) -> '" .. test_plain .. "'")

-- DB2 brotli (SCHEMA template is expanded to real name before decode)
local db2_result = queue.decode_embedded(
	"myschema.BROTLI_DECOMPRESS(myschema.BASE64DECODEBINARY('" .. b64_blob .. "'))")
check(db2_result == test_plain,
	"decode_embedded: DB2 brotli+base64 -> '" .. test_plain .. "'")

-- PostgreSQL brotli
local pg_result = queue.decode_embedded(
	"brotli_decompress(DECODE('" .. b64_blob .. "', 'base64'))")
check(pg_result == test_plain,
	"decode_embedded: PostgreSQL brotli_decompress(DECODE) -> '" .. test_plain .. "'")

-- PostgreSQL CONVERT_FROM
local pg_conv_result = queue.decode_embedded(
	"CONVERT_FROM(DECODE('" .. plain_b64 .. "', 'base64'), 'UTF8')")
check(pg_conv_result == "test",
	"decode_embedded: PostgreSQL CONVERT_FROM(DECODE) -> 'test'")

-- SQLite standalone base64
local sqlite_b64_result = queue.decode_embedded("CRYPTO_DECODE('" .. plain_b64 .. "')")
check(sqlite_b64_result == "test",
	"decode_embedded: SQLite CRYPTO_DECODE -> 'test'")

-- DB2 standalone base64
local db2_b64_result = queue.decode_embedded("BASE64DECODE('" .. plain_b64 .. "')")
check(db2_b64_result == "test",
	"decode_embedded: DB2 BASE64DECODE -> 'test'")

-- MySQL standalone base64
local mysql_b64_result = queue.decode_embedded("FROM_BASE64('" .. plain_b64 .. "')")
check(mysql_b64_result == "test",
	"decode_embedded: MySQL FROM_BASE64 -> 'test'")

-- No false positive on plain SQL
local plain_result = queue.decode_embedded("SELECT 1 FROM users WHERE id = 1")
check(plain_result == "SELECT 1 FROM users WHERE id = 1",
	"decode_embedded: plain SQL unchanged -> 'SELECT 1 FROM users WHERE id = 1'")

-- No decode on unmatched function
local no_match = queue.decode_embedded("SOMETHING('dGVzdA==')")
check(no_match == "SOMETHING('dGVzdA==')",
	"decode_embedded: unmatched function unchanged")

-- =========================================================================
-- Mixed content test (brotli inside a larger SQL string)
-- =========================================================================
print("=== mixed content tests ===")

local mixed = "CREATE FUNCTION foo() RETURNS TEXT AS $$ "
	.. "brotli_decompress(FROM_BASE64('" .. b64_blob .. "')) "
	.. "RETURN " .. test_plain .. " $$ LANGUAGE plpgsql;"
local mixed_result = queue.decode_embedded(mixed)
check(mixed_result:find(test_plain .. " $$", 1, true) ~= nil,
	"decode_embedded: MySQL lowercase brotli inside larger SQL string")

print("")
print("ALL TESTS PASSED")
