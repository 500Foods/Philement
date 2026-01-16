-- Migration: acuranzo_1000.lua
-- Bootstraps the migration system by creating the queries table and supporting user-defined functions

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 5.0.0 - 2026-01-16 - Added Timezone conversion UDF declarations for all database engines
-- 4.0.0 - 2025-11-27 - Added Brotli decompression UDF declarations for all database engines
-- 3.1.0 - 2025-11-23 - Added DROP_CHECK to reverse migration
-- 3.0.0 - 2025-10-30 - Another overhaul (thanks, MySQL) to have an alternate increment mechanism
-- 2.0.0 - 2025-10-18 - Moved to latest migration format
-- 1.1.0 - 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 - 2025-09-13 - Initial creation for queries table with SQLite, PostgreSQL, MySQL, DB2 support.

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1000"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    CREATE TABLE ${SCHEMA}${QUERIES} (
        query_id                ${INTEGER}          NOT NULL,
        query_ref               ${INTEGER}          NOT NULL,
        query_status_a27        ${INTEGER}          NOT NULL,
        query_type_a28          ${INTEGER}          NOT NULL,
        query_dialect_a30       ${INTEGER}          NOT NULL,
        query_queue_a58         ${INTEGER}          NOT NULL,
        query_timeout           ${INTEGER}          NOT NULL,
        code                    ${TEXT_BIG}         NOT NULL,
        name                    ${TEXT}             NOT NULL,
        summary                 ${TEXT_BIG}                 ,
        collection              ${JSON}                     ,
        ${COMMON_CREATE}
        ${PRIMARY}(query_id),                                               -- Primary Key
        ${UNIQUE}(query_ref, query_type_a28)                                -- Unique Column
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: SQLite has no JSON handling peculiarities, so no custom function is defined
--       Defined as macros in individual database_<engine>.lua files
if engine ~= 'sqlite' then table.insert(queries,{sql=[[

    -- Defined in database_<engine>.lua as a macro
    ${JSON_INGEST_FUNCTION}

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: DB2 has no built-in Base64 encoding, so here we're adding the UDF pointing at the
--       UDF we've got in extras/base64_udf_db2 in case it is applied to a different schema
if engine == 'db2' then table.insert(queries,{sql=[[

    CREATE OR REPLACE FUNCTION ${SCHEMA}BASE64_DECODE_CHUNK(input_base64 VARCHAR(32672))
    RETURNS VARCHAR(32672)
    LANGUAGE C
    PARAMETER STYLE DB2SQL
    NO SQL
    DETERMINISTIC
    NOT FENCED
    THREADSAFE
    RETURNS NULL ON NULL INPUT
    EXTERNAL NAME 'base64_chunk_udf.so!BASE64_DECODE_CHUNK'

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: DB2 has no built-in Base64 encoding, so here we're adding the UDF pointing at the
--       UDF we've got in extras/base64_udf_db2 in case it is applied to a different schema
if engine == 'db2' then table.insert(queries,{sql=[[

    CREATE OR REPLACE FUNCTION ${SCHEMA}BASE64DECODE(encoded CLOB(1M))
    RETURNS CLOB(1M)
    LANGUAGE SQL
    DETERMINISTIC
    READS SQL DATA

    BEGIN ATOMIC
        DECLARE result  CLOB(1M);
        DECLARE pos     INTEGER   DEFAULT 1;
        DECLARE len     INTEGER;
        DECLARE step    INTEGER   DEFAULT 32672;
        DECLARE piece   VARCHAR(32672);
        DECLARE aligned INTEGER;
        DECLARE chunk   VARCHAR(32672);
        SET result = CAST('' AS CLOB(1K));

        SET len = LENGTH(encoded);

        chunk_loop:
        WHILE pos <= len DO
            SET piece = CAST(SUBSTR(encoded, pos, step) AS VARCHAR(32672));
            IF piece IS NULL OR LENGTH(piece) = 0 THEN
            LEAVE chunk_loop;
            END IF;

            SET aligned = LENGTH(piece) - MOD(LENGTH(piece), 4);
            IF aligned > 0 THEN
            SET chunk  = SUBSTR(piece, 1, aligned);
            SET result = result || CAST(${SCHEMA}BASE64_DECODE_CHUNK(chunk) AS CLOB(32672));
            END IF;

            SET pos = pos + step;
        END WHILE chunk_loop;

        RETURN result;
    END

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: We do it all again for a binary version of the Base64 decode function
--       so that we can properly handle the output of the Brotli decompress function
if engine == 'db2' then table.insert(queries,{sql=[[

    CREATE OR REPLACE FUNCTION ${SCHEMA}BASE64_DECODE_CHUNK_BINARY(input_base64 VARCHAR(32672))
    RETURNS BLOB(32672)
    LANGUAGE C
    PARAMETER STYLE DB2SQL
    NO SQL
    DETERMINISTIC
    NOT FENCED
    THREADSAFE
    RETURNS NULL ON NULL INPUT
    EXTERNAL NAME 'base64_chunk_udf.so!BASE64_DECODE_CHUNK_BINARY'

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: We do it all again for a binary version of the Base64 decode function
--       so that we can properly handle the output of the Brotli decompress function
if engine == 'db2' then table.insert(queries,{sql=[[

    CREATE OR REPLACE FUNCTION ${SCHEMA}BASE64DECODEBINARY(encoded CLOB(1M))
    RETURNS BLOB(1M)
    LANGUAGE SQL
    DETERMINISTIC
    READS SQL DATA
    BEGIN ATOMIC
    DECLARE result  BLOB(1M);
    DECLARE pos     INTEGER   DEFAULT 1;
    DECLARE len     INTEGER;
    DECLARE step    INTEGER   DEFAULT 32672;
    DECLARE piece   VARCHAR(32672);
    DECLARE aligned INTEGER;
    DECLARE chunk   VARCHAR(32672);
    SET result = CAST('' AS BLOB(1K));
    SET len = LENGTH(encoded);

    chunk_loop:
    WHILE pos <= len DO
        SET piece = CAST(SUBSTR(encoded, pos, step) AS VARCHAR(32672));
        IF piece IS NULL OR LENGTH(piece) = 0 THEN
        LEAVE chunk_loop;
        END IF;

        SET aligned = LENGTH(piece) - MOD(LENGTH(piece), 4);
        IF aligned > 0 THEN
        SET chunk  = SUBSTR(piece, 1, aligned);
        SET result = result || CAST(${SCHEMA}BASE64_DECODE_CHUNK_BINARY(chunk) AS BLOB(32672));
        END IF;

        SET pos = pos + step;
    END WHILE chunk_loop;

    RETURN result;
    END

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: DB2 has no built-in Base64 encoding, so here we're adding the UDF pointing at the
--       UDF we've got in extras/base64encode_udf_db2 in case it is applied to a different schema
if engine == 'db2' then table.insert(queries,{sql=[[

    CREATE OR REPLACE FUNCTION ${SCHEMA}BASE64_ENCODE_CHUNK(input_data VARCHAR(32672))
    RETURNS VARCHAR(32672)
    LANGUAGE C
    PARAMETER STYLE DB2SQL
    NO SQL
    DETERMINISTIC
    NOT FENCED
    THREADSAFE
    RETURNS NULL ON NULL INPUT
    EXTERNAL NAME 'base64_encode_udf.so!BASE64_ENCODE_CHUNK'

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: DB2 wrapper for BASE64ENCODE - simple for small inputs, chunked for large
if engine == 'db2' then table.insert(queries,{sql=[[

    CREATE OR REPLACE FUNCTION ${SCHEMA}BASE64ENCODE(input_data CLOB(1M))
    RETURNS CLOB(1M)
    LANGUAGE SQL
    DETERMINISTIC
    READS SQL DATA

    BEGIN ATOMIC
        DECLARE result  CLOB(1M);
        DECLARE pos     INTEGER   DEFAULT 1;
        DECLARE len     INTEGER;
        DECLARE step    INTEGER   DEFAULT 24000;
        DECLARE piece   VARCHAR(24000);

        SET len = LENGTH(input_data);
        SET result = CAST('' AS CLOB(1K));

        IF len <= 24000 THEN
            RETURN ${SCHEMA}BASE64_ENCODE_CHUNK(CAST(input_data AS VARCHAR(24000)));
        END IF;

        chunk_loop:
        WHILE pos <= len DO
            SET piece = CAST(SUBSTR(input_data, pos, step) AS VARCHAR(24000));
            IF piece IS NULL OR LENGTH(piece) = 0 THEN
            LEAVE chunk_loop;
            END IF;

            SET result = result || CAST(${SCHEMA}BASE64_ENCODE_CHUNK(piece) AS CLOB(32672));
            SET pos = pos + step;
        END WHILE chunk_loop;

        RETURN result;
    END

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: DB2 binary version of BASE64 encode for encoding BLOB data
if engine == 'db2' then table.insert(queries,{sql=[[

    CREATE OR REPLACE FUNCTION ${SCHEMA}BASE64_ENCODE_CHUNK_BINARY(input_data BLOB(32672))
    RETURNS VARCHAR(32672)
    LANGUAGE C
    PARAMETER STYLE DB2SQL
    NO SQL
    DETERMINISTIC
    NOT FENCED
    THREADSAFE
    RETURNS NULL ON NULL INPUT
    EXTERNAL NAME 'base64_encode_udf.so!BASE64_ENCODE_CHUNK_BINARY'

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: DB2 wrapper for BASE64ENCODEBINARY - simple for small inputs, chunked for large
if engine == 'db2' then table.insert(queries,{sql=[[

    CREATE OR REPLACE FUNCTION ${SCHEMA}BASE64ENCODEBINARY(input_data BLOB(1M))
    RETURNS CLOB(1M)
    LANGUAGE SQL
    DETERMINISTIC
    READS SQL DATA
    BEGIN ATOMIC
    DECLARE result  CLOB(1M);
    DECLARE pos     INTEGER   DEFAULT 1;
    DECLARE len     INTEGER;
    DECLARE step    INTEGER   DEFAULT 24000;
    DECLARE piece   BLOB(24000);

    SET len = LENGTH(input_data);
    SET result = CAST('' AS CLOB(1K));

    IF len <= 24000 THEN
        RETURN ${SCHEMA}BASE64_ENCODE_CHUNK_BINARY(CAST(input_data AS BLOB(24000)));
    END IF;

    chunk_loop:
    WHILE pos <= len DO
        SET piece = CAST(SUBSTR(input_data, pos, step) AS BLOB(24000));
        IF piece IS NULL OR LENGTH(piece) = 0 THEN
        LEAVE chunk_loop;
        END IF;

        SET result = result || CAST(${SCHEMA}BASE64_ENCODE_CHUNK_BINARY(piece) AS CLOB(32672));
        SET pos = pos + step;
    END WHILE chunk_loop;

    RETURN result;
    END

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: MySQL UDF for Brotli decompression
--       Requires: libbrotli-dev and brotli_decompress.so in plugin directory
--       Installation handled via extras/brotli_udf_mysql/
if engine == 'mysql' then table.insert(queries,{sql=[[

    DROP FUNCTION IF EXISTS brotli_decompress;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: Brotli decompression functions for all engines
--       MySQL, PostgreSQL, SQLite: Single function declaration
--       Defined as macros in individual database_<engine>.lua files
table.insert(queries,{sql=[[

    ${BROTLI_DECOMPRESS_FUNCTION}

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: Timezone conversion function convert_tz added for SQLite
if engine == 'mysql' then table.insert(queries,{sql=[[

    ${CONVERT_TZ_FUNCTION}

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_APPLIED_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            CREATE TABLE ${SCHEMA}${TABLE} (
                query_id                ${INTEGER},
                query_ref               ${INTEGER}          NOT NULL,
                query_status_a27        ${INTEGER}          NOT NULL,
                query_type_a28          ${INTEGER}          NOT NULL,
                query_dialect_a30       ${INTEGER}          NOT NULL,
                query_queue_a58         ${INTEGER}          NOT NULL,
                query_timeout           ${INTEGER}          NOT NULL,
                query_code              ${TEXT_BIG}         NOT NULL,
                name                    ${VARCHAR_100}      NOT NULL,
                summary                 ${TEXT_BIG}                 ,
                collection              ${JSON}                     ,
                ${COMMON_CREATE}
                ${PRIMARY}(query_id),
                ${UNIQUE}(query_ref)
            );
        ]=]
                                                                            AS code,
        'Create ${TABLE} Table'                                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            This is the first migration that, technically, is run automatically
            when connecting to an empty database and kicks off the migration,
            so long as the database has been configured with AutoMigration: true
            in its config (this is the default if not supplied).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: Here our intent is to drop the main queries table but we're trying to be careful not to
--       drop it if anyting has been added to it that we didn't add to it ourselves via another
--       migration. We'd expect customers to add things regularly, so this is more of a safety
--       check, like with all the migrations to follow, in case it is run against a production
--       database with records added that did not come directly from a migration.
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DELETE FROM ${SCHEMA}${QUERIES}
            WHERE query_type_a28 IN (1000, 1001, 1002, 1003);

            ${SUBQUERY_DELIMITER}

            ${DROP_CHECK};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} Table'                                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                  AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
      ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
      SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
      FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON Table Definition in collection'                               AS code,
        'Diagram Tables: ${SCHEMA}${TABLE}'                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}

            This is the first JSON Diagram code for the ${TABLE} table.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "template",
                        "object_id": "base template",
                        "object_value": "<svg   xmlns=\"http://www.w3.org/2000/svg\"
                                                xmlns:xlink=\"http://www.w3.org/1999/xlink\"
                                                width=\"2520\"
                                                height=\"1980\"
                                                viewBox=\"0 0 2520 1980\">
                                            <!-- Define clipPath for rounded border with 2mm margin -->
                                            <defs>
                                                <clipPath id=\"border-clip\">
                                                <rect x=\"18\" y=\"18\" width=\"2484\" height=\"1944\" rx=\"42.51\" ry=\"42.51\"/>
                                                </clipPath>
                                            </defs>
                                            <!-- Thin black border with 2mm margin -->
                                            <rect x=\"18\" y=\"18\" width=\"2484\" height=\"1944\" fill=\"none\" stroke=\"black\" stroke-width=\"3\" rx=\"42.51\" ry=\"42.51\"/>
                                            <!-- White background within border, clipped to rounded shape -->
                                            <rect x=\"18\" y=\"18\" width=\"2484\" height=\"1944\" fill=\"white\" clip-path=\"url(#border-clip)\"/>
                                            <!-- 1cm (28.3pt) dashed silver grid, offset by 2mm margin -->
                                            <defs>
                                                <pattern id=\"grid\" width=\"84.9\" height=\"84.9\" patternUnits=\"userSpaceOnUse\" x=\"18\" y=\"18\">
                                                    <path d=\"M 84.9 0 L 0 0 0 84.9\" fill=\"none\" stroke=\"black\" stroke-width=\"1.5\" stroke-dasharray=\"6,6\"/>
                                                </pattern>
                                            </defs>
                                            <rect x=\"18\" y=\"18\" width=\"2484\" height=\"1944\" fill=\"url(#grid)\" clip-path=\"url(#border-clip)\"/>
                                            <!-- ERD content placeholder -->
                                            <g id=\"erd-content\" transform=\"translate(0, 0)\">
                                                <!-- ERD SVG content goes here -->
                                            </g>
                                        </svg>"
                    },
                    {
                        "object_type": "table",
                        "object_id": "table.${TABLE}",
                        "object_ref": "${MIGRATION}",
                        "table": [
                            {
                                "name": "query_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "query_ref",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": true
                            },
                            {
                                "name": "query_status_a27",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "query_type_a28",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "query_dialect_a30",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "query_queue_a58",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "query_timeout",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": false
                            },
                            {
                                "name": "query_code",
                                "datatype": "${TEXT_BIG}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "name",
                                "datatype": "${TEXT}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false

                            },
                            {
                                "name": "summary",
                                "datatype": "${TEXT_BIG}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "collection",
                                "datatype": "${JSON}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": false
                            },
                            ${COMMON_DIAGRAM}
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
                                                                            AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
