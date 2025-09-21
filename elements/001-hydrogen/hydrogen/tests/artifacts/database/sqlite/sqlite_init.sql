CREATE TABLE IF NOT EXISTS queries (
    query_number INTEGER NOT NULL,
    query_code  TEXT NOT NULL
);
DELETE FROM queries;
INSERT INTO queries VALUES (2000,'create table queries;');
INSERT INTO queries VALUES (3000,'create table queries;');
INSERT INTO queries VALUES (4000,'create table queries;');
SELECT * FROM queries;
