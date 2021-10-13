CREATE TABLE tags (
  id integer,
  user_name text,
  name text
);

CREATE USER alice NOLOGIN;
GRANT ALL ON TABLE tags TO alice;

INSERT INTO tags VALUES (1, 'nonexistent', 'PostgreSQL');
INSERT INTO tags VALUES (2, 'alice', 'Groonga');
INSERT INTO tags VALUES (3, 'alice', 'PGroonga');
INSERT INTO tags VALUES (4, 'alice', 'pglogical');

ALTER TABLE tags ENABLE ROW LEVEL SECURITY;
CREATE POLICY tags_myself ON tags USING (user_name = current_user);

SET enable_seqscan = on;
SET enable_indexscan = off;
SET enable_bitmapscan = off;

SET SESSION AUTHORIZATION alice;
EXPLAIN (COSTS OFF)
SELECT name
  FROM tags
 WHERE name &^| ARRAY['gro', 'pos']
 ORDER BY id;

SELECT name
  FROM tags
 WHERE name &^| ARRAY['gro', 'pos']
 ORDER BY id;
RESET SESSION AUTHORIZATION;

DROP TABLE tags;

DROP USER alice;