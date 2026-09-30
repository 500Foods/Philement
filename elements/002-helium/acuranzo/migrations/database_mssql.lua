-- database_mssql.lua
-- SQL Server (MSSQL) dialect macros for Helium migrations

-- luacheck: no max line length

-- CHANGELOG
-- 1.2.0 - 2026-09-30 - base64_decode and base64_encode use UTF-8 bytes, same as the other engines
-- 1.1.0 - 2026-09-30 - Added Phase 4 T-SQL helper function bodies: base64_decode, base64_encode, sha256_b64, brotli_decompress (CLR)
-- 1.0.0 - 2026-09-29 - Initial MSSQL dialect (Phase 2 of MSSQL.md)

-- NOTES
-- Requires: msodbcsql18 ODBC driver 18+ and unixODBC on Linux.
-- ${SCHEMA} is a dot-prefixed schema name (e.g. testms.).
-- All text types use NVARCHAR/NCCHAR for Unicode support.
-- JSON functions use native SQL Server 2016+ JSON_VALUE / OPENJSON.
-- Brotli decompression via CLR (extras/brotli_udf_mssql/ C# assembly).
-- sha256_b64 via HASHBYTES('SHA2_256') + XML base64 encoding (Phase 4).
-- base64_decode via XML bytes interpreted as UTF-8 (SQL Server 2019+).
-- ISJSON() available in SQL Server 2016+ for JSON validation.

return {
    -- Types
    CHAR_2 = "NCHAR(2)",
    CHAR_20 = "NCHAR(20)",
    CHAR_50 = "NCHAR(50)",
    CHAR_128 = "NCHAR(128)",
    DATE = "DATE",
    DATETIME = "DATETIME2",
    FLOAT = "REAL",
    FLOAT_BIG = "FLOAT",
    INSERT_KEY_START = "-- ",
    INSERT_KEY_END = "",
    INSERT_KEY_RETURN = "RETURNING ",
    INTEGER = "INT",
    INTEGER_BIG = "BIGINT",
    INTEGER_SMALL = "SMALLINT",
    JRS = "JSON_VALUE(",
    JRM = ", ",
    JRE = ")",
    NOW = "SYSUTCDATETIME()",
    PRIMARY = "PRIMARY KEY",
    REORG = "-- REORG TABLE",
    SERIAL = "INT IDENTITY(1,1)",
    SESSION_SECS = "DATEDIFF(SECOND, :SESSION_START, SYSUTCDATETIME())",
    SIZE_COLLECTION = "LEN(collection)",
    SIZE_FLOAT = "4",
    SIZE_FLOAT_BIG = "8",
    SIZE_INTEGER = "4",
    SIZE_INTEGER_BIG = "8",
    SIZE_INTEGER_SMALL = "2",
    SIZE_TIMESTAMP = "8",
    TEXT = "NVARCHAR(255)",
    TEXT_BIG = "NVARCHAR(MAX)",
    TIME = "TIME",
    TIMESTAMP = "DATETIME2",
    TIMESTAMP_TZ = "DATETIMEOFFSET",
    TRFS = "DATEADD(SECOND, 0 + ",
    TRFE = ", ${NOW})",
    TRFMS = "DATEADD(MINUTE, 0 + ",
    TRFME = ", ${NOW})",
    TRMS = "DATEADD(MINUTE, 0 - ",
    TRME = ", ${NOW})",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "NVARCHAR(20)",
    VARCHAR_50 = "NVARCHAR(50)",
    VARCHAR_64 = "NVARCHAR(64)",
    VARCHAR_100 = "NVARCHAR(100)",
    VARCHAR_128 = "NVARCHAR(128)",
    VARCHAR_500 = "NVARCHAR(500)",

    -- DUMMY_TABLE: SELECT 1 needs no FROM in SQL Server
    DUMMY_TABLE = "",

    -- Password hash: testms.sha256_b64('0', 'password')
    -- Returns base64-encoded SHA256 hash
    -- Phase 4: requires CLR assembly for sha256_b64 function
    -- Usage: ${SHA256_HASH_START}'0'${SHA256_HASH_MID}'${HYDROGEN_DEMO_ADMIN_PASS}'${SHA256_HASH_END}
    SHA256_HASH_START = "${SCHEMA}sha256_b64(",
    SHA256_HASH_MID = ", ",
    SHA256_HASH_END = ")",

    -- Base64 decode via scalar function (Phase 4: CLR or built-in)
    -- Usage: ${BASE64_START}'base64data'${BASE64_END}
    BASE64_START = "${SCHEMA}base64_decode(",
    BASE64_END = ")",

    -- DB2-style extras that MSSQL does not need but must define to avoid ${UNSUBSTITUTED}
    BASE64ENCODE_START = "${SCHEMA}base64_encode(",
    BASE64ENCODE_END = ")",
    BASE64ENCODEBINARY_START = "${SCHEMA}base64_encode_binary(",
    BASE64ENCODEBINARY_END = ")",

    -- Brotli decompression: Phase 4 (CREATE ASSEMBLY + CREATE FUNCTION from extras)
    -- Requires: extras/brotli_udf_mssql/ with compiled C# assembly
    -- Phase 5: Compression disabled until CLR assembly is deployed
    BROTLI_DECOMPRESS_FUNCTION = "-- Phase 4: CREATE ASSEMBLY brotli_assembly + CREATE FUNCTION brotli_decompress (CLR)",
    COMPRESS_START = nil,
    COMPRESS_END = nil,

    -- DROP_CHECK: raise if rows exist
    DROP_CHECK = "IF EXISTS(SELECT 1 FROM ${SCHEMA}${TABLE}) THROW 51000, 'Refusing to drop table ${SCHEMA}${TABLE} – it contains data', 1;",

    -- Date/Time formatting (not used in migrations but defined for completeness)
    DATETIME_FORMAT = "CONVERT(VARCHAR(30), ${NOW}, 120)",
    TIMESTAMP_FORMAT = "CONVERT(VARCHAR(30), ${NOW}, 120)",

    JSON = "NVARCHAR(MAX)",
    JIS = "${SCHEMA}json_ingest(",
    JIE = ")",
    JSON_INGEST_START = "${SCHEMA}json_ingest(",
    JSON_INGEST_END = ")",

    -- Schema ingest: SQL Server JSON_VALUE accepts $ref/$id/$schema natively.
    -- These macros alias json_ingest; no separate function object is required.
    JSON_INGEST_SCHEMA_START = "${SCHEMA}json_ingest(",
    JSON_INGEST_SCHEMA_END = ")",
    JSON_INGEST_SCHEMA_FUNCTION = "",

    -- json_ingest T-SQL function: validates and normalizes JSON, escaping
    -- control characters inside strings. Uses ISJSON() for fast-path validation.
    -- SQL Server 2016+ supports ISJSON(); SQL Server 2022 adds ISJSON with path.
    JSON_INGEST_FUNCTION = [[
        CREATE OR ALTER FUNCTION ${SCHEMA}json_ingest(@s NVARCHAR(MAX))
        RETURNS NVARCHAR(MAX)
        AS
        BEGIN
            DECLARE @out NVARCHAR(MAX) = '';
            DECLARE @i INT = 1;
            DECLARE @L INT = LEN(@s);
            DECLARE @ch NCHAR(1);
            DECLARE @in_str BIT = 0;
            DECLARE @esc BIT = 0;

            -- fast path: already valid JSON
            IF ISJSON(@s) = 1
                RETURN @s;

            WHILE @i <= @L
            BEGIN
                SET @ch = SUBSTRING(@s, @i, 1);

                IF @esc = 1
                BEGIN
                    SET @out = @out + @ch;
                    SET @esc = 0;
                END
                ELSE IF @ch = '\'
                BEGIN
                    SET @out = @out + @ch;
                    SET @esc = 1;
                END
                ELSE IF @ch = '"'
                BEGIN
                    SET @out = @out + @ch;
                    SET @in_str = 1 - @in_str;
                END
                ELSE IF @in_str = 1 AND @ch = CHAR(10)
                BEGIN
                    SET @out = @out + '\n';
                END
                ELSE IF @in_str = 1 AND @ch = CHAR(13)
                BEGIN
                    SET @out = @out + '\r';
                END
                ELSE IF @in_str = 1 AND @ch = CHAR(9)
                BEGIN
                    SET @out = @out + '\t';
                END
                ELSE
                BEGIN
                    SET @out = @out + @ch;
                END

                SET @i = @i + 1;
            END

            -- ensure result is JSON; NULL if still invalid
            IF ISJSON(@out) = 0
                RETURN NULL;

            RETURN @out;
        END
    ]],

    -- base64_decode: XML base64 to bytes, read as UTF-8, stored as NVARCHAR.
    -- SQL Server has no native base64 decode. The XML value() method yields
    -- the raw bytes. A Windows code-page database collation would read those
    -- bytes as that code page. Inserting them into a
    -- UTF-8 column interprets the bytes as UTF-8, matching PostgreSQL
    -- CONVERT_FROM(..., 'UTF8') and MySQL utf8mb4. Invalid base64 returns
    -- the original string. A user-defined function cannot CATCH a bad UTF-8
    -- sequence; migration text is valid UTF-8.
    BASE64_DECODE_FUNCTION = [[
        CREATE OR ALTER FUNCTION ${SCHEMA}base64_decode(@s NVARCHAR(MAX))
        RETURNS NVARCHAR(MAX)
        AS
        BEGIN
            DECLARE @result NVARCHAR(MAX) = '';
            IF @s IS NULL OR LEN(@s) = 0
                RETURN @result;
            DECLARE @clean NVARCHAR(MAX) = REPLACE(REPLACE(@s, CHAR(13), ''), CHAR(10), '');
            DECLARE @bin VARBINARY(MAX) = CAST(N'<x>' + @clean + N'</x>' AS XML).value('(/x)[1]', 'VARBINARY(MAX)');
            IF @bin IS NULL
                RETURN @s;
            DECLARE @utf8 TABLE (v VARCHAR(MAX) COLLATE Latin1_General_100_CI_AS_SC_UTF8);
            INSERT INTO @utf8(v) VALUES (@bin);
            SELECT @result = v FROM @utf8;
            RETURN @result;
        END
    ]],

    -- base64_encode: UTF-8 bytes of the string, then XML base64.
    -- CAST(nvarchar AS varbinary) is UTF-16LE. The other engines encode
    -- UTF-8. Same collation sha256_b64 uses (SQL Server 2019+).
    BASE64_ENCODE_FUNCTION = [[
        CREATE OR ALTER FUNCTION ${SCHEMA}base64_encode(@s NVARCHAR(MAX))
        RETURNS NVARCHAR(MAX)
        AS
        BEGIN
            DECLARE @result NVARCHAR(MAX) = '';
            IF @s IS NULL OR LEN(@s) = 0
                RETURN @result;
            DECLARE @bin VARBINARY(MAX) = CAST(CAST(@s AS VARCHAR(MAX)) COLLATE Latin1_General_100_CI_AS_SC_UTF8 AS VARBINARY(MAX));
            SET @result = CAST(N'' AS XML).value('xs:base64Binary(sql:variable("@bin"))', 'NVARCHAR(MAX)');
            RETURN @result;
        END
    ]],

    -- base64_encode_binary: encode VARBINARY as base64 string (for brotli output).
    BASE64_ENCODE_BINARY_FUNCTION = [[
        CREATE OR ALTER FUNCTION ${SCHEMA}base64_encode_binary(@b VARBINARY(MAX))
        RETURNS NVARCHAR(MAX)
        AS
        BEGIN
            IF @b IS NULL OR DATALENGTH(@b) = 0
                RETURN '';
            RETURN CAST(N'' AS XML).value('xs:base64Binary(sql:variable("@b"))', 'NVARCHAR(MAX)');
        END
    ]],

    -- sha256_b64: SHA-256 hash of UTF-8 concatenated inputs, then base64-encoded.
    -- SQL Server's HASHBYTES('SHA2_256', ...) hashes bytes as sent.
    -- To match SQLite's base64(HASHBYTES('SHA2_256', CONCAT(N'0', N'password')))
    -- we must hash UTF-8 bytes. We cast to VARCHAR with a UTF-8 collation
    -- (Latin1_General_100_CI_AS_SC_UTF8 on SQL Server 2019+) to get UTF-8,
    -- then HASHBYTES, then base64-encode via XML.
    SHA256_B64_FUNCTION = [[
        CREATE OR ALTER FUNCTION ${SCHEMA}sha256_b64(@prefix NVARCHAR(10), @password NVARCHAR(MAX))
        RETURNS NVARCHAR(MAX)
        AS
        BEGIN
            -- Concatenate with UTF-8 encoding to match SQLite/other engines
            -- Latin1_General_100_CI_AS_SC_UTF8 is available in SQL Server 2019+
            DECLARE @combined NVARCHAR(MAX) = @prefix + ISNULL(@password, '');
            -- Cast to VARCHAR with UTF-8 collation to get UTF-8 bytes
            DECLARE @utf8 VARBINARY(MAX) = CAST(CAST(@combined AS VARCHAR(MAX)) COLLATE Latin1_General_100_CI_AS_SC_UTF8 AS VARBINARY(MAX));
            -- SHA-256 hash
            DECLARE @hash VARBINARY(32) = HASHBYTES('SHA2_256', @utf8);
            -- Base64 encode the hash
            RETURN CAST(N'' AS XML).value('xs:base64Binary(sql:variable("@hash"))', 'NVARCHAR(MAX)');
        END
    ]],

    JSON_VALUE_FUNCTION = "-- SQL Server has native JSON_VALUE; no UDR required",

    -- Timezone conversion: SQL Server uses AT TIME ZONE natively (no UDF needed)
    CONVERT_TZ_FUNCTION = "-- SQL Server uses AT TIME ZONE for timezone conversion",
}
