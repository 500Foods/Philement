INSERT INTO demo.numbers (numbers)
WITH digits(i) AS (VALUES (0),(1),(2),(3),(4),(5),(6),(7),(8),(9))
SELECT thousands.i * 1000 + hundreds.i * 100 + tens.i * 10 + units.i
FROM digits units, digits tens, digits hundreds, digits thousands;
