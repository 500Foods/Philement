-- install_brotli_decompress.clp
-- DB2 CLP script to register Brotli decompression UDFs
-- Called via Makefile with schema substitution

-- Create the chunk-based C UDF
CREATE OR REPLACE FUNCTION ${SCHEMA}.BROTLI_DECOMPRESS_CHUNK(input_data VARCHAR(32672))
RETURNS VARCHAR(32672)
LANGUAGE C
PARAMETER STYLE DB2SQL
NO SQL
DETERMINISTIC
NOT FENCED
THREADSAFE
RETURNS NULL ON NULL INPUT
EXTERNAL NAME 'brotli_decompress.so!BROTLI_DECOMPRESS_CHUNK'
@

-- Create the wrapper function for large CLOBs (same pattern as BASE64DECODE)
CREATE OR REPLACE FUNCTION ${SCHEMA}.BROTLI_DECOMPRESS(compressed CLOB(2G))
RETURNS CLOB(2G)
LANGUAGE SQL
DETERMINISTIC
READS SQL DATA

BEGIN ATOMIC
    DECLARE result  CLOB(2G);
    DECLARE pos     INTEGER   DEFAULT 1;
    DECLARE len     INTEGER;
    DECLARE step    INTEGER   DEFAULT 32672;
    DECLARE piece   VARCHAR(32672);
    DECLARE chunk   VARCHAR(32672);
    SET result = CAST('' AS CLOB(1K));

    SET len = LENGTH(compressed);

    chunk_loop:
    WHILE pos <= len DO
        SET piece = CAST(SUBSTR(compressed, pos, step) AS VARCHAR(32672));
        IF piece IS NULL OR LENGTH(piece) = 0 THEN
            LEAVE chunk_loop;
        END IF;

        SET chunk = ${SCHEMA}.BROTLI_DECOMPRESS_CHUNK(piece);
        SET result = result || CAST(chunk AS CLOB(32672));

        SET pos = pos + step;
    END WHILE chunk_loop;

    RETURN result;
END
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