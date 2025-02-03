CREATE TABLE ids (
  id real
);

INSERT INTO ids VALUES (2.1);
INSERT INTO ids VALUES (7.1);
INSERT INTO ids VALUES (6.1);
INSERT INTO ids VALUES (4.1);
INSERT INTO ids VALUES (5.1);
INSERT INTO ids VALUES (8.1);
INSERT INTO ids VALUES (1.1);
INSERT INTO ids VALUES (10.1);
INSERT INTO ids VALUES (3.1);
INSERT INTO ids VALUES (9.1);

CREATE INDEX grnindex ON ids USING pgroonga (id pgroonga_float4_ops);

SET enable_seqscan = off;
SET enable_indexscan = on;

SELECT id
  FROM ids
  ORDER BY id ASC
  LIMIT 5;

DROP TABLE ids;
