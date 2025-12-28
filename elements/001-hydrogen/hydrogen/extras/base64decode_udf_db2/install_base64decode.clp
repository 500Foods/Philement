-- install_base64decode.clp (use with db2 -td@)

-- Chunk decoder now uses PARAMETER STYLE DB2SQL and returns VARCHAR
CREATE OR REPLACE FUNCTION ${SCHEMA}.BASE64_DECODE_CHUNK(input_base64 VARCHAR(32672))
RETURNS VARCHAR(32672)
LANGUAGE C
PARAMETER STYLE DB2SQL
NO SQL
DETERMINISTIC
NOT FENCED
THREADSAFE
RETURNS NULL ON NULL INPUT
EXTERNAL NAME 'base64_chunk_udf.so!BASE64_DECODE_CHUNK'
@

-- Wrapper: conservative loop, 32,672-char slices (already 4-aligned)
CREATE OR REPLACE FUNCTION ${SCHEMA}.BASE64DECODE(encoded CLOB(100K))
RETURNS CLOB(100K)
LANGUAGE SQL
DETERMINISTIC
READS SQL DATA
BEGIN ATOMIC
  DECLARE result  CLOB(100K);
  DECLARE pos     INTEGER   DEFAULT 1;
  DECLARE len     INTEGER;
  DECLARE step    INTEGER   DEFAULT 32672;              -- multiple of 4
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
      SET result = result || CAST(${SCHEMA}.BASE64_DECODE_CHUNK(chunk) AS CLOB(32672));
    END IF;

    SET pos = pos + step;
  END WHILE chunk_loop;

  RETURN result;
END
@

-- Test UDF: returns "Hydrogen"
CREATE OR REPLACE FUNCTION ${SCHEMA}.HYDROGEN_CHECK()
RETURNS VARCHAR(100)
LANGUAGE C
PARAMETER STYLE DB2SQL
NO SQL
DETERMINISTIC
NOT FENCED
THREADSAFE
EXTERNAL NAME 'base64_chunk_udf.so!HYDROGEN_CHECK'
@

-- Smoke tests
-- VALUES ${SCHEMA}.HYDROGEN_CHECK() @
-- VALUES ${SCHEMA}.BASE64_DECODE_CHUNK('SGVsbG8gV29ybGQh') @
-- VALUES CAST(${SCHEMA}.BASE64DECODE('SGVsbG8gV29ybGQh') AS VARCHAR(100)) @
-- SELECT LENGTH(${SCHEMA}.BASE64DECODE('SGVsbG8gV29ybGQh')) AS DECODED_BYTES FROM SYSIBM.SYSDUMMY1 @

-- Binary version for handling binary data (returns BLOB)
CREATE OR REPLACE FUNCTION ${SCHEMA}.BASE64_DECODE_CHUNK_BINARY(input_base64 VARCHAR(32672))
RETURNS BLOB(32672)
LANGUAGE C
PARAMETER STYLE DB2SQL
NO SQL
DETERMINISTIC
NOT FENCED
THREADSAFE
RETURNS NULL ON NULL INPUT
EXTERNAL NAME 'base64_chunk_udf.so!BASE64_DECODE_CHUNK_BINARY'
@

-- Wrapper for large binary base64 decoding (returns BLOB)
CREATE OR REPLACE FUNCTION ${SCHEMA}.BASE64DECODEBINARY(encoded CLOB(100K))
RETURNS BLOB(100K)
LANGUAGE SQL
DETERMINISTIC
READS SQL DATA
BEGIN ATOMIC
   DECLARE result  BLOB(100K);
   DECLARE pos     INTEGER   DEFAULT 1;
   DECLARE len     INTEGER;
   DECLARE step    INTEGER   DEFAULT 32672;              -- multiple of 4
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
       SET result = result || CAST(${SCHEMA}.BASE64_DECODE_CHUNK_BINARY(chunk) AS BLOB(32672));
     END IF;

     SET pos = pos + step;
   END WHILE chunk_loop;

   RETURN result;
END
@
