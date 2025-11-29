INSERT INTO canvas.queries (
    query_id,
    query_ref,
    query_status_a27,
    query_type_a28,
    query_dialect_a30,
    query_queue_a58,
    query_timeout,
    code,
    name,
    summary,
    collection,
    valid_after,
    valid_until,
    created_id,
    created_at,
    updated_id,
    updated_at
)
WITH next_query_id AS (
    SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
    FROM canvas.queries
)
SELECT
new_query_id                                                        AS query_id,
1000                                                        AS query_ref,
1                                                    AS query_status_a27,
1002                                           AS query_type_a28,
3                                                          AS query_dialect_a30,
0                                                         AS query_queue_a58,
5000                                                          AS query_timeout,
'JSON Table Definition in collection'                               AS code,
'Diagram Tables: canvas.queries'                                 AS name,
cast(from_base64('CiMgRGlhZ3JhbSBNaWdyYXRpb24gMTAwMAoKIyMgRGlhZ3JhbSBUYWJsZXM6IGNhbnZhcy5xdWVyaWVzCgpUaGlzIGlzIHRoZSBmaXJzdCBKU09OIERpYWdyYW0gY29kZSBmb3IgdGhlIHF1ZXJpZXMgdGFibGUuCg==') as char character set utf8mb4)
AS summary,
-- DIAGRAM_START
canvas.json_ingest(
brotli_decompress(from_base64('GzgbALwOcEPGjmh/VQsXonZGxVk46mB4YZtn4w7OoxfT1vdXqcCik3Mtewli02cf675fSTsvHaQfoQajYUaYppbIWeLdSWXNTE5wJc4NTIJYoTXREdmbLpsJs5ttZXT+f/emaMACHqD1eMjnvnnbKqU3anX+P/N2Ulr10BIFEGrVYtnHcLjSVWNhIX9PMAMAxaCytC4DAL0OTXFyV3SIKwWaJN3Kp7LrmYQlF4Kd7IXx0tCXwN+sXxvJKBAucufbiPARpblojxgH0izr8xAfLS1YIADJx6rjTEHQLV1VpsOkEhWieS58rjKfaFIBBsahw8JiQfFXf2YKUi182hglJ1z00/8TmhZ9D1TtRqX0v+DM4gpK4CHze9ZFDI5KLzajR7p0XUxvwMMgze3B+ybkDSWma3pQHDWfRS+qukxdpm0B78VQjE4qtpfiZMGKYH9Jd+6I7nbgDc4wgxRU9Ug3R/YI19vluJ2PZKXQw6Sk0VIauCQ3E4fRoDV0jsYIiIoBdOjpNJ/hdpSU9l13VFDipwSsUS2dpgpJ+vSC6iJxrr2KNgVWd+VOc2bj62o+4xTjS+H1/2vtukY6zml0B7NNlJS6przxjJXRp7GqHnkKZZnh+uMz1CMch6wjt1J2hTpbZWIfwDjiUX6D5TZN9gJ4pmxavTwsWyx+s6tdT+4DIwU8+Ncz4UJkXSWINbPQLw9z8WUQap1fZpI+9T0CjaHeGl9XWCAskcoFi5Dt6p82BBkG7oVcbkAJGA2Jg4B4U/7RMiCa7AeCwi/usp/1/FpXXSwkSjG+DWnhJa4sLBdSBKoOM1GwsMc8ggUZF3TJvtaH2wSanqzBhLRNzRRn73DrTSUai16Tnbz5dcC9A6cCiTdtBvQNmjXl/Q8Nx/pulgCi1PKt7ifEwMlqeSuqmG9RLA7HRiPxOIrF4dgiUZD/xY/3mwTgNZnB4BAPP+/mJdy7F9e3m0tdC+ZzXZMRUfC28VKDMSl2Lrh7N8zJEsAgY1GAGfFBjjXi2caG/FCxCLHFbyPhPmsIoYaY33fGitnf16tUdbsjZfPGt6Byp2n/jvfTbEZ716m9nbdZ0QeIZ9kB'))
)
-- DIAGRAM_END
AS collection,
NULL            AS valid_after ,
NULL            AS valid_until ,
0               AS created_id ,
CURRENT_TIMESTAMP          AS created_at ,
0               AS updated_id ,
CURRENT_TIMESTAMP          AS updated_at
FROM next_query_id;
