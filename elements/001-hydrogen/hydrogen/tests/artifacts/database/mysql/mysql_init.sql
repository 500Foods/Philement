-- CREATE DATABASE canvas;
 USE canvas;
CREATE TABLE IF NOT EXISTS canvas.queries (
    QUERY_NUMBER INT NOT NULL,
    QUERY_CODE VARCHAR(1000) NOT NULL
);
DELETE FROM canvas.queries;
INSERT INTO canvas.queries VALUES (2000, 'select * from canvas.queries;');
INSERT INTO canvas.queries VALUES (3000, 'select * from canvas.queries;');
INSERT INTO canvas.queries VALUES (4000, 'select * from canvas.queries;');
INSERT INTO canvas.queries VALUES (5000, 'select * from canvas.queries;');
SELECT * FROM canvas.queries;

