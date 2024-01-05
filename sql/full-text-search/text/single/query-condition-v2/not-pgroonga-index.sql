CREATE TABLE memos (
  id integer,
  content text
);

INSERT INTO memos VALUES (1, 'PostgreSQL is a RDBMS.');
INSERT INTO memos VALUES (2, 'Groonga is fast full text search engine.');
INSERT INTO memos VALUES (3, 'PGroonga is a PostgreSQL extension that uses Groonga.');

CREATE INDEX btree_index ON memos (content);

SELECT id, content
  FROM memos
 WHERE content &@~
       pgroonga_condition('Groonga', index_name => 'btree_index');

DROP TABLE memos;
