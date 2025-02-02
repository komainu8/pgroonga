-- Downgrade SQL

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
