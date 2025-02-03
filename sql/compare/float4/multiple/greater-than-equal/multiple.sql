CREATE TABLE numbers (
  number1 real,
  number2 real
);

INSERT INTO numbers VALUES (2.1,  20.1);
INSERT INTO numbers VALUES (7.1,  70.1);
INSERT INTO numbers VALUES (6.1,  60.1);
INSERT INTO numbers VALUES (4.1,  40.1);
INSERT INTO numbers VALUES (5.1,  50.1);
INSERT INTO numbers VALUES (8.1,  80.1);
INSERT INTO numbers VALUES (1.1,  10.1);
INSERT INTO numbers VALUES (10.1, 100.1);
INSERT INTO numbers VALUES (3.1,  30.1);
INSERT INTO numbers VALUES (9.1,  90.1);

CREATE INDEX grnindex
    ON numbers
 USING pgroonga (number1 pgroonga_float4_ops, number2 pgroonga_float4_ops);

SET enable_seqscan = off;
SET enable_indexscan = on;
SET enable_bitmapscan = off;

EXPLAIN (COSTS OFF)
SELECT number1, number2
  FROM numbers
 WHERE number1 >= 3 AND number2 >= 50
 ORDER BY number1 ASC;

SELECT number1, number2
  FROM numbers
 WHERE number1 >= 3 AND number2 >= 50
 ORDER BY number1 ASC;

DROP TABLE numbers;
