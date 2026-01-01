-- Migration: acuranzo_1135.lua
-- QueryRef #044 - Get Filtered Lookup: AI Models

-- NOTE: This is one of the very few migration files that creates a separate
--       query for each database engine, as the query is quite complex and
--       requires different approaches for different database engines, primarily
--       due to how the different databases engines handle JSON data.

-- db2
-- mysql
-- postgresql
-- sqlite

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-31 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1135"
cfg.QUERY_REF = "044"
cfg.QUERY_NAME = "Get Filtered Lookup: AI Models"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine ~= 'db2' then table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}${QUERIES} (
                ${QUERIES_INSERT}
            )
            WITH next_query_id AS (
                SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
                FROM ${SCHEMA}${QUERIES}
            )
            SELECT
                new_query_id                                                        AS query_id,
                ${QUERY_REF}                                                        AS query_ref,
                ${STATUS_ACTIVE}                                                    AS query_status_a27,
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        key_idx,
                        value_txt,
                        value_int,
                        JSON_OBJECT(
                            KEY 'Name' VALUE JSON_VALUE(collection, '$.Name'),
                            KEY 'Model' VALUE JSON_VALUE(collection, '$.Model'),
                            KEY 'Icon' VALUE JSON_VALUE(collection, '$.Icon'),
                            KEY 'Authority' VALUE JSON_VALUE(collection, '$.Authority'),
                            KEY 'Location' VALUE JSON_VALUE(collection, '$.Location'),
                            KEY 'Country' VALUE JSON_VALUE(collection, '$.Country'),
                            KEY 'Engine' VALUE JSON_VALUE(collection, '$.Engine'),
                            KEY 'Support Prompts' VALUE JSON_VALUE(collection, '$.Support Prompts'),
                            KEY 'Demo' VALUE JSON_VALUE(collection, '$.Demo')
                        ) AS collection,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    FROM
                        ${SCHEMA}lookups
                    WHERE
                        (lookup_id = :LOOKUPID)
                        AND (status_a1 = 1)
                        AND (
                            (valid_after IS NULL)
                            OR (valid_after <= ${NOW})
                        )
                        AND (
                            (valid_until IS NULL)
                            OR (valid_until >= ${NOW})
                        )
                    ORDER BY
                        sort_seq,
                        JSON_VALUE(collection, '$.Name')
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query retrieves a list of AI models from the lookups table,
                    but filters the JSON data to hide sensitive information like AI
                    model API keys and other details not relevant to the client.

                    ## Parameters

                    - `:LOOKUPID`: The lookup ID to retrieve the AI models for.

                    ## Returns

                    - key_idx: The unique identifier for the lookup entry.
                    - value_txt: The text value associated with the lookup entry.
                    - value_int: The integer value associated with the lookup entry.
                    - collection: A JSON object containing filtered details about the AI model.
                    - valid_after: The timestamp after which the lookup entry is valid.
                    - valid_until: The timestamp until which the lookup entry is valid.
                    - created_id: The ID of the user who created the lookup entry.
                    - created_at: The timestamp when the lookup entry was created.
                    - updated_id: The ID of the user who last updated the lookup entry.
                    - updated_at: The timestamp when the lookup entry was last updated.

                    ## Tables

                    - `${SCHEMA}lookups`: Stores lookup keys

                    ## Notes

                    - The filter is constructed as fields explicity added, rather than
                    excluding values. This means that in future, it may have to be
                    updated if other fields get added to the JSON collection and
                    need to get shared with the client applications.

                ]==]
                                                                                    AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Poulate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine ~= 'mysql' then table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}${QUERIES} (
                ${QUERIES_INSERT}
            )
            WITH next_query_id AS (
                SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
                FROM ${SCHEMA}${QUERIES}
            )
            SELECT
                new_query_id                                                        AS query_id,
                ${QUERY_REF}                                                        AS query_ref,
                ${STATUS_ACTIVE}                                                    AS query_status_a27,
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        key_idx,
                        value_txt,
                        value_int,
                        JSON_OBJECT(
                            'Name', collection->>'Name',
                            'Model', collection->>'Model',
                            'Icon', collection->>'Icon',
                            'Authority', collection->>'Authority',
                            'Location', collection->>'Location',
                            'Country', collection->>'Country',
                            'Engine', collection->>'Engine',
                            'Support Prompts', collection->>'Support Prompts',
                            'Demo', collection->>'Demo'
                        ) AS collection,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    FROM
                        ${SCHEMA}lookups
                    WHERE
                        (lookup_id = :LOOKUPID)
                        AND (status_a1 = 1)
                        AND (
                            (valid_after IS NULL)
                            OR (valid_after <= ${NOW})
                        )
                        AND (
                            (valid_until IS NULL)
                            OR (valid_until >= ${NOW})
                        )
                    ORDER BY
                        sort_seq,
                        collection->>'Name'
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query retrieves a list of AI models from the lookups table,
                    but filters the JSON data to hide sensitive information like AI
                    model API keys and other details not relevant to the client.

                    ## Parameters

                    - `:LOOKUPID`: The lookup ID to retrieve the AI models for.

                    ## Returns

                    - key_idx: The unique identifier for the lookup entry.
                    - value_txt: The text value associated with the lookup entry.
                    - value_int: The integer value associated with the lookup entry.
                    - collection: A JSON object containing filtered details about the AI model.
                    - valid_after: The timestamp after which the lookup entry is valid.
                    - valid_until: The timestamp until which the lookup entry is valid.
                    - created_id: The ID of the user who created the lookup entry.
                    - created_at: The timestamp when the lookup entry was created.
                    - updated_id: The ID of the user who last updated the lookup entry.
                    - updated_at: The timestamp when the lookup entry was last updated.

                    ## Tables

                    - `${SCHEMA}lookups`: Stores lookup keys

                    ## Notes

                    - The filter is constructed as fields explicity added, rather than
                    excluding values. This means that in future, it may have to be
                    updated if other fields get added to the JSON collection and
                    need to get shared with the client applications.

                ]==]
                                                                                    AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Poulate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine ~= 'postgresql' then table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}${QUERIES} (
                ${QUERIES_INSERT}
            )
            WITH next_query_id AS (
                SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
                FROM ${SCHEMA}${QUERIES}
            )
            SELECT
                new_query_id                                                        AS query_id,
                ${QUERY_REF}                                                        AS query_ref,
                ${STATUS_ACTIVE}                                                    AS query_status_a27,
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        key_idx,
                        value_txt,
                        value_int,
                        jsonb_build_object(
                            'Name', collection->'Name',
                            'Model', collection->'Model',
                            'Icon', collection->'Icon',
                            'Authority', collection->'Authority',
                            'Location', collection->'Location',
                            'Country', collection->'Country',
                            'Engine', collection->'Engine',
                            'Support Prompts', collection->'Support Prompts',
                            'Demo', collection->'Demo'
                        ) AS collection,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    FROM
                        ${SCHEMA}lookups
                    WHERE
                        (lookup_id =:LOOKUPID)
                        AND (status_a1 = 1)
                        AND (
                            (valid_after IS NULL)
                            OR (valid_after <= ${NOW})
                        )
                        AND (
                            (valid_until IS NULL)
                            OR (valid_until >= ${NOW})
                        )
                    ORDER BY
                        sort_seq,
                        collection->'Name'
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query retrieves a list of AI models from the lookups table,
                    but filters the JSON data to hide sensitive information like AI
                    model API keys and other details not relevant to the client.

                    ## Parameters

                    - `:LOOKUPID`: The lookup ID to retrieve the AI models for.

                    ## Returns

                    - key_idx: The unique identifier for the lookup entry.
                    - value_txt: The text value associated with the lookup entry.
                    - value_int: The integer value associated with the lookup entry.
                    - collection: A JSON object containing filtered details about the AI model.
                    - valid_after: The timestamp after which the lookup entry is valid.
                    - valid_until: The timestamp until which the lookup entry is valid.
                    - created_id: The ID of the user who created the lookup entry.
                    - created_at: The timestamp when the lookup entry was created.
                    - updated_id: The ID of the user who last updated the lookup entry.
                    - updated_at: The timestamp when the lookup entry was last updated.

                    ## Tables

                    - `${SCHEMA}lookups`: Stores lookup keys

                    ## Notes

                    - The filter is constructed as fields explicity added, rather than
                    excluding values. This means that in future, it may have to be
                    updated if other fields get added to the JSON collection and
                    need to get shared with the client applications.

                ]==]
                                                                                    AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Poulate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine ~= 'sqlite' then table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}${QUERIES} (
                ${QUERIES_INSERT}
            )
            WITH next_query_id AS (
                SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
                FROM ${SCHEMA}${QUERIES}
            )
            SELECT
                new_query_id                                                        AS query_id,
                ${QUERY_REF}                                                        AS query_ref,
                ${STATUS_ACTIVE}                                                    AS query_status_a27,
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        key_idx,
                        value_txt,
                        value_int,
                        json_object(
                            'Name', collection ->> 'Name',
                            'Model', collection ->> 'Model',
                            'Icon', collection ->> 'Icon',
                            'Authority', collection ->> 'Authority',
                            'Location', collection ->> 'Location',
                            'Country', collection ->> 'Country',
                            'Engine', collection ->> 'Engine',
                            'Support Prompts', collection ->> 'Support Prompts',
                            'Demo', collection ->> 'Demo'
                        ) AS collection,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    FROM
                        ${SCHEMA}lookups
                    WHERE
                        (lookup_id = :LOOKUPID)
                        AND (status_lua_1 = 1)
                        AND (
                            (valid_after IS NULL)
                            OR (valid_after <= ${NOW})
                        )
                        AND (
                            (valid_until IS NULL)
                            OR (valid_until >= ${NOW})
                        )
                    ORDER BY
                        sort_seq,
                        collection ->> 'Name'
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query retrieves a list of AI models from the lookups table,
                    but filters the JSON data to hide sensitive information like AI
                    model API keys and other details not relevant to the client.

                    ## Parameters

                    - `:LOOKUPID`: The lookup ID to retrieve the AI models for.

                    ## Returns

                    - key_idx: The unique identifier for the lookup entry.
                    - value_txt: The text value associated with the lookup entry.
                    - value_int: The integer value associated with the lookup entry.
                    - collection: A JSON object containing filtered details about the AI model.
                    - valid_after: The timestamp after which the lookup entry is valid.
                    - valid_until: The timestamp until which the lookup entry is valid.
                    - created_id: The ID of the user who created the lookup entry.
                    - created_at: The timestamp when the lookup entry was created.
                    - updated_id: The ID of the user who last updated the lookup entry.
                    - updated_at: The timestamp when the lookup entry was last updated.

                    ## Tables

                    - `${SCHEMA}lookups`: Stores lookup keys

                    ## Notes

                    - The filter is constructed as fields explicity added, rather than
                    excluding values. This means that in future, it may have to be
                    updated if other fields get added to the JSON collection and
                    need to get shared with the client applications.

                ]==]
                                                                                    AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Poulate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE query_ref = ${QUERY_REF};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
