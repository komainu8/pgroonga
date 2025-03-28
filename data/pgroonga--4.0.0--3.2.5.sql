-- Downgrade SQL

CREATE SCHEMA pgroonga;

/* v1 */
CREATE OPERATOR CLASS pgroonga.text_full_text_search_ops FOR TYPE text
    USING pgroonga AS
        OPERATOR 6 ~~,
        OPERATOR 7 ~~*,
        OPERATOR 8 %%,
        OPERATOR 9 @@,
        OPERATOR 12 &@,
        OPERATOR 13 &?, -- For backward compatibility
        OPERATOR 28 &@~;

CREATE OPERATOR CLASS pgroonga.text_array_full_text_search_ops FOR TYPE text[]
    USING pgroonga AS
        OPERATOR 8 %% (text[], text),
        OPERATOR 9 @@ (text[], text),
        OPERATOR 12 &@ (text[], text),
        OPERATOR 13 &? (text[], text), -- For backward compatibility
        OPERATOR 28 &@~ (text[], text);

CREATE OPERATOR CLASS pgroonga.varchar_full_text_search_ops FOR TYPE varchar
    USING pgroonga AS
        OPERATOR 8 %%,
        OPERATOR 9 @@,
        OPERATOR 12 &@,
        OPERATOR 13 &?, -- For backward compatibility
        OPERATOR 28 &@~;

CREATE OPERATOR CLASS pgroonga.varchar_ops FOR TYPE varchar
    USING pgroonga AS
        OPERATOR 1 < (text, text),
        OPERATOR 2 <= (text, text),
        OPERATOR 3 = (text, text),
        OPERATOR 4 >= (text, text),
        OPERATOR 5 > (text, text);

CREATE OPERATOR CLASS pgroonga.varchar_array_ops FOR TYPE varchar[]
    USING pgroonga AS
        OPERATOR 8 %% (varchar[], varchar),
        OPERATOR 23 &> (varchar[], varchar);

CREATE OPERATOR CLASS pgroonga.bool_ops FOR TYPE bool
    USING pgroonga AS
        OPERATOR 1 <,
        OPERATOR 2 <=,
        OPERATOR 3 =,
        OPERATOR 4 >=,
        OPERATOR 5 >;

CREATE OPERATOR CLASS pgroonga.int2_ops FOR TYPE int2
    USING pgroonga AS
        OPERATOR 1 <,
        OPERATOR 2 <=,
        OPERATOR 3 =,
        OPERATOR 4 >=,
        OPERATOR 5 >;

CREATE OPERATOR CLASS pgroonga.int4_ops FOR TYPE int4
    USING pgroonga AS
        OPERATOR 1 <,
        OPERATOR 2 <=,
        OPERATOR 3 =,
        OPERATOR 4 >=,
        OPERATOR 5 >;

CREATE OPERATOR CLASS pgroonga.int8_ops FOR TYPE int8
    USING pgroonga AS
        OPERATOR 1 <,
        OPERATOR 2 <=,
        OPERATOR 3 =,
        OPERATOR 4 >=,
        OPERATOR 5 >;

CREATE OPERATOR CLASS pgroonga.float4_ops FOR TYPE float4
    USING pgroonga AS
        OPERATOR 1 <,
        OPERATOR 2 <=,
        OPERATOR 3 =,
        OPERATOR 4 >=,
        OPERATOR 5 >;

CREATE OPERATOR CLASS pgroonga.float8_ops FOR TYPE float8
    USING pgroonga AS
        OPERATOR 1 <,
        OPERATOR 2 <=,
        OPERATOR 3 =,
        OPERATOR 4 >=,
        OPERATOR 5 >;

CREATE OPERATOR CLASS pgroonga.timestamp_ops FOR TYPE timestamp
    USING pgroonga AS
        OPERATOR 1 <,
        OPERATOR 2 <=,
        OPERATOR 3 =,
        OPERATOR 4 >=,
        OPERATOR 5 >;

CREATE OPERATOR CLASS pgroonga.timestamptz_ops FOR TYPE timestamptz
    USING pgroonga AS
        OPERATOR 1 <,
        OPERATOR 2 <=,
        OPERATOR 3 =,
        OPERATOR 4 >=,
        OPERATOR 5 >;

CREATE OPERATOR CLASS pgroonga.jsonb_ops FOR TYPE jsonb
    USING pgroonga AS
        OPERATOR 9 @@ (jsonb, text),
        OPERATOR 11 @>,
        OPERATOR 12 &@ (jsonb, text),
        OPERATOR 13 &? (jsonb, text), -- For backward compatibility
        OPERATOR 15 &` (jsonb, text),
        OPERATOR 28 &@~ (jsonb, text);

CREATE OPERATOR CLASS pgroonga.text_regexp_ops FOR TYPE text
    USING pgroonga AS
        OPERATOR 6 ~~,
        OPERATOR 7 ~~*,
        OPERATOR 10 @~,
        OPERATOR 22 &~;

CREATE OPERATOR CLASS pgroonga.varchar_regexp_ops FOR TYPE varchar
    USING pgroonga AS
        OPERATOR 10 @~,
        OPERATOR 22 &~;

-- v2
CREATE OPERATOR CLASS pgroonga.text_full_text_search_ops_v2 FOR TYPE text
    USING pgroonga AS
        OPERATOR 6 ~~,
        OPERATOR 7 ~~*,
        OPERATOR 8 %%, -- For backward compatibility
        OPERATOR 9 @@, -- For backward compatibility
        OPERATOR 12 &@,
        OPERATOR 13 &?, -- For backward compatibility
        OPERATOR 14 &~?, -- For backward compatibility
        OPERATOR 15 &`,
        OPERATOR 18 &@| (text, text[]),
        OPERATOR 19 &?| (text, text[]), -- For backward compatibility
        OPERATOR 26 &@> (text, text[]), -- For backward compatibility
        OPERATOR 27 &?> (text, text[]), -- For backward compatibility
        OPERATOR 28 &@~,
        OPERATOR 29 &@*,
        OPERATOR 30 &@~| (text, text[]);

CREATE OPERATOR CLASS pgroonga.text_array_full_text_search_ops_v2 FOR TYPE text[]
    USING pgroonga AS
        OPERATOR 8 %% (text[], text), -- For backward compatibility
        OPERATOR 9 @@ (text[], text), -- For backward compatibility
        OPERATOR 12 &@ (text[], text),
        OPERATOR 13 &? (text[], text), -- For backward compatibility
        OPERATOR 14 &~? (text[], text), -- For backward compatibility
        OPERATOR 15 &` (text[], text),
        OPERATOR 18 &@| (text[], text[]),
        OPERATOR 19 &?| (text[], text[]), -- For backward compatibility
        OPERATOR 28 &@~ (text[], text),
        OPERATOR 29 &@* (text[], text),
        OPERATOR 30 &@~| (text[], text[]);

CREATE OPERATOR CLASS pgroonga.text_term_search_ops_v2 FOR TYPE text
    USING pgroonga AS
        OPERATOR 1 <,
        OPERATOR 2 <=,
        OPERATOR 3 =,
        OPERATOR 4 >=,
        OPERATOR 5 >,
        OPERATOR 16 &^,
        OPERATOR 17 &^~,
        OPERATOR 20 &^| (text, text[]),
        OPERATOR 21 &^~| (text, text[]);

CREATE OPERATOR CLASS pgroonga.text_array_term_search_ops_v2 FOR TYPE text[]
    USING pgroonga AS
        OPERATOR 16 &^ (text[], text),
        OPERATOR 17 &^~ (text[], text),
        OPERATOR 20 &^| (text[], text[]),
        OPERATOR 21 &^~| (text[], text[]),
        OPERATOR 24 &^> (text[], text), -- For backward compatibility
        OPERATOR 25 &^~> (text[], text); -- For backward compatibility

CREATE OPERATOR CLASS pgroonga.text_regexp_ops_v2 FOR TYPE text
    USING pgroonga AS
        OPERATOR 6 ~~,
        OPERATOR 7 ~~*,
        OPERATOR 10 @~, -- For backward compatibility
        OPERATOR 22 &~;

CREATE OPERATOR CLASS pgroonga.varchar_full_text_search_ops_v2 FOR TYPE varchar
    USING pgroonga AS
        OPERATOR 8 %%, -- For backward compatibility
        OPERATOR 9 @@, -- For backward compatibility
        OPERATOR 12 &@,
        OPERATOR 13 &?, -- For backward compatibility
        OPERATOR 14 &~?, -- For backward compatibility
        OPERATOR 15 &`,
        OPERATOR 18 &@| (text, text[]),
        OPERATOR 19 &?| (text, text[]), -- For backward compatibility
        OPERATOR 26 &@> (text, text[]), -- For backward compatibility
        OPERATOR 27 &?> (text, text[]), -- For backward compatibility
        OPERATOR 28 &@~,
        OPERATOR 29 &@*,
        OPERATOR 30 &@~| (text, text[]);

CREATE OPERATOR CLASS pgroonga.varchar_array_term_search_ops_v2
    FOR TYPE varchar[]
    USING pgroonga AS
        OPERATOR 8 %% (varchar[], varchar), -- For backward compatibility
        OPERATOR 23 &> (varchar[], varchar);

CREATE OPERATOR CLASS pgroonga.varchar_regexp_ops_v2 FOR TYPE varchar
    USING pgroonga AS
        OPERATOR 10 @~, -- For backward compatibility
        OPERATOR 22 &~;

CREATE OPERATOR CLASS pgroonga.jsonb_ops_v2 FOR TYPE jsonb
    USING pgroonga AS
        OPERATOR 9 @@ (jsonb, text), -- For backward compatibility
        OPERATOR 11 @>,
        OPERATOR 12 &@ (jsonb, text),
        OPERATOR 13 &? (jsonb, text), -- For backward compatibility
        OPERATOR 15 &` (jsonb, text),
        OPERATOR 28 &@~ (jsonb, text);

CREATE FUNCTION pgroonga.score("row" record)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'pgroonga_score'
	LANGUAGE C
	VOLATILE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.table_name(indexName cstring)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_table_name'
	LANGUAGE C
	STABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.command(groongaCommand text)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_command'
	LANGUAGE C
	VOLATILE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.command(groongaCommand text, arguments text[])
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_command'
	LANGUAGE C
	VOLATILE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.query_expand(tableName cstring,
				      termColumnName text,
				      synonymsColumnName text,
				      query text)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_query_expand'
	LANGUAGE C
	STABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.snippet_html(target text, keywords text[], width integer DEFAULT 200)
	RETURNS text[]
	AS 'MODULE_PATHNAME', 'pgroonga_snippet_html'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.highlight_html(target text, keywords text[])
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_highlight_html'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_positions_byte(target text, keywords text[])
	RETURNS integer[2][]
	AS 'MODULE_PATHNAME', 'pgroonga_match_positions_byte'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_positions_character(target text, keywords text[])
	RETURNS integer[2][]
	AS 'MODULE_PATHNAME', 'pgroonga_match_positions_character'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.query_extract_keywords(query text)
	RETURNS text[]
	AS 'MODULE_PATHNAME', 'pgroonga_query_extract_keywords'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.flush(indexName cstring)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_flush'
	LANGUAGE C
	VOLATILE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.command_escape_value(value text)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_command_escape_value'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.query_escape(query text)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_query_escape'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.escape(value text)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_escape_string'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.escape(value text, special_characters text)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_escape_string'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.escape(value boolean)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_escape_boolean'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.escape(value int2)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_escape_int2'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.escape(value int4)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_escape_int4'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.escape(value int8)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_escape_int8'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.escape(value float4)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_escape_float8'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.escape(value float8)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_escape_float8'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.escape(value timestamp)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_escape_timestamptz'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.escape(value timestamptz)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_escape_timestamptz'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;


/* v1 */
CREATE FUNCTION pgroonga.match_term(target text, term text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_term_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_term(target text[], term text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_term_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_term(target varchar, term varchar)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_term_varchar'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_term(target varchar[], term varchar)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_term_varchar_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_query(text, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_query_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_query(text[], text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_query_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_query(varchar, varchar)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_query_varchar'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_regexp(text, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_regexp_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_regexp(varchar, varchar)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_regexp_varchar'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

/* v2 */
CREATE FUNCTION pgroonga.match_text(text, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_text_array(text[], text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_varchar(varchar, varchar)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_varchar'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.contain_varchar_array(varchar[], varchar)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_contain_varchar_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_jsonb(jsonb, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_jsonb'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.query_text(text, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_query_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.query_text_array(text[], text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_query_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.query_varchar(varchar, varchar)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_query_varchar'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.query_jsonb(jsonb, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_query_jsonb'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.similar_text(text, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_similar_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.similar_text_array(text[], text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_similar_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.similar_varchar(varchar, varchar)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_similar_varchar'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.prefix_text(text, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_prefix_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.prefix_text_array(text[], text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_prefix_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.prefix_rk_text(text, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_prefix_rk_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.prefix_rk_text_array(text[], text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_prefix_rk_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.script_text(text, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_script_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.script_text_array(text[], text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_script_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.script_varchar(varchar, varchar)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_script_varchar'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.script_jsonb(jsonb, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_script_jsonb'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_in_text(text, text[])
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_in_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_in_text_array(text[], text[])
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_in_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_in_varchar(varchar, varchar[])
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_in_varchar'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.query_in_text(text, text[])
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_query_in_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.query_in_text_array(text[], text[])
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_query_in_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.query_in_varchar(varchar, varchar[])
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_query_in_varchar'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.prefix_in_text(text, text[])
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_prefix_in_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.prefix_in_text_array(text[], text[])
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_prefix_in_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.prefix_rk_in_text(text, text[])
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_prefix_rk_in_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.prefix_rk_in_text_array(text[], text[])
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_prefix_rk_in_text_array'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.regexp_text(text, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_regexp_text'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.regexp_varchar(varchar, varchar)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_regexp_varchar'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga.match_script_jsonb(jsonb, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'pgroonga_match_script_jsonb'
	LANGUAGE C
	IMMUTABLE
	STRICT
	PARALLEL SAFE;
