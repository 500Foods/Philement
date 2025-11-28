-- Brotli decompression extension for PostgreSQL
-- Version 1.0

-- Create the brotli_decompress function
CREATE OR REPLACE FUNCTION brotli_decompress(compressed bytea)
RETURNS text
AS 'MODULE_PATHNAME', 'brotli_decompress'
LANGUAGE C STRICT IMMUTABLE;

COMMENT ON FUNCTION brotli_decompress(bytea) IS 
'Decompresses Brotli-compressed data. Used by Helium migration system for large base64-encoded strings.';