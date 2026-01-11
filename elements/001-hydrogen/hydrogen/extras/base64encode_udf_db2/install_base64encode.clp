-- install_base64encode.clp (use with db2 -td@)

-- Chunk encoder now uses PARAMETER STYLE DB2SQL and returns VARCHAR
CREATE OR REPLACE FUNCTION ${SCHEMA}.BASE64_ENCODE_CHUNK(input_data VARCHAR(32672))
RETURNS VARCHAR(32672)
LANGUAGE C
PARAMETER STYLE DB2SQL
NO SQL
DETERMINISTIC
NOT FENCED
THREADSAFE
RETURNS NULL ON NULL INPUT
EXTERNAL NAME 'base64_encode_udf.so!BASE64_ENCODE_CHUNK'
@

-- Simple wrapper: for small inputs, just call the chunk directly
-- For larger inputs, use chunked processing
CREATE OR REPLACE FUNCTION ${SCHEMA}.BASE64ENCODE(input_data CLOB(100K))
RETURNS CLOB(100K)
LANGUAGE SQL
DETERMINISTIC
READS SQL DATA
BEGIN ATOMIC
  DECLARE result  CLOB(100K);
  DECLARE pos     INTEGER   DEFAULT 1;
  DECLARE len     INTEGER;
  DECLARE step    INTEGER   DEFAULT 24000;              -- multiple of 3, under chunk limit
  DECLARE piece   VARCHAR(24000);
  
  SET len = LENGTH(input_data);
  SET result = CAST('' AS CLOB(1K));

  -- Small input optimization: encode directly
  IF len <= 24000 THEN
    RETURN ${SCHEMA}.BASE64_ENCODE_CHUNK(CAST(input_data AS VARCHAR(24000)));
  END IF;

  -- Chunked processing for large inputs
  chunk_loop:
  WHILE pos <= len DO
    SET piece = CAST(SUBSTR(input_data, pos, step) AS VARCHAR(24000));
    IF piece IS NULL OR LENGTH(piece) = 0 THEN
      LEAVE chunk_loop;
    END IF;

    SET result = result || CAST(${SCHEMA}.BASE64_ENCODE_CHUNK(piece) AS CLOB(32672));
    SET pos = pos + step;
  END WHILE chunk_loop;

  RETURN result;
END
@

-- Binary version for encoding binary data (BLOB input)
CREATE OR REPLACE FUNCTION ${SCHEMA}.BASE64_ENCODE_CHUNK_BINARY(input_data BLOB(32672))
RETURNS VARCHAR(32672)
LANGUAGE C
PARAMETER STYLE DB2SQL
NO SQL
DETERMINISTIC
NOT FENCED
THREADSAFE
RETURNS NULL ON NULL INPUT
EXTERNAL NAME 'base64_encode_udf.so!BASE64_ENCODE_CHUNK_BINARY'
@

-- Simple wrapper for binary base64 encoding (takes BLOB, returns CLOB)
CREATE OR REPLACE FUNCTION ${SCHEMA}.BASE64ENCODEBINARY(input_data BLOB(100K))
RETURNS CLOB(100K)
LANGUAGE SQL
DETERMINISTIC
READS SQL DATA
BEGIN ATOMIC
   DECLARE result  CLOB(100K);
   DECLARE pos     INTEGER   DEFAULT 1;
   DECLARE len     INTEGER;
   DECLARE step    INTEGER   DEFAULT 24000;              -- multiple of 3, under chunk limit
   DECLARE piece   BLOB(24000);
   
   SET len = LENGTH(input_data);
   SET result = CAST('' AS CLOB(1K));

   -- Small input optimization: encode directly
   IF len <= 24000 THEN
     RETURN ${SCHEMA}.BASE64_ENCODE_CHUNK_BINARY(CAST(input_data AS BLOB(24000)));
   END IF;

   -- Chunked processing for large inputs
   chunk_loop:
   WHILE pos <= len DO
     SET piece = CAST(SUBSTR(input_data, pos, step) AS BLOB(24000));
     IF piece IS NULL OR LENGTH(piece) = 0 THEN
       LEAVE chunk_loop;
     END IF;

     SET result = result || CAST(${SCHEMA}.BASE64_ENCODE_CHUNK_BINARY(piece) AS CLOB(32672));
     SET pos = pos + step;
   END WHILE chunk_loop;

   RETURN result;
END
@

-- Smoke tests
-- VALUES ${SCHEMA}.BASE64_ENCODE_CHUNK('Hello World!') @
-- VALUES CAST(${SCHEMA}.BASE64ENCODE('Hello World!') AS VARCHAR(100)) @
-- VALUES ${SCHEMA}.BASE64ENCODE('Hello World!') @
