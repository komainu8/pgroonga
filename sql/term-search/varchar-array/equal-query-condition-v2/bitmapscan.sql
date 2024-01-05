CREATE TABLE tags (
  names varchar[]
);

INSERT INTO tags VALUES (ARRAY['PostgreSQL', 'PG']);
INSERT INTO tags VALUES (ARRAY['Groonga', 'grn', 'groonga']);
INSERT INTO tags VALUES (ARRAY['PGroonga', 'pgrn', 'sql']);

CREATE INDEX pgroonga_index ON tags
  USING pgroonga (names pgroonga_varchar_array_term_search_ops_v2);

SET enable_seqscan = off;
SET enable_indexscan = off;
SET enable_bitmapscan = on;

\pset format unaligned
EXPLAIN (COSTS OFF)
SELECT names
  FROM tags
 WHERE names &=~ pgroonga_condition('grn OR sql', index_name => 'pgroonga_index')
\g |sed -r -e "s/('.+'|ROW.+)::pgroonga/pgroonga/g"
\pset format aligned

SELECT names
  FROM tags
 WHERE names &=~ pgroonga_condition('grn OR sql', index_name => 'pgroonga_index');

DROP TABLE tags;
