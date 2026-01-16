-- Migration: acuranzo_1137.lua
-- QueryRef #046 - Get Main Menu

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-31 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1137"
cfg.QUERY_REF = "046"
cfg.QUERY_NAME = "Get Main Menu"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
                        modulegroup.value_txt grpname,
                        modulegroup.key_idx grpnum,
                        module.value_txt modname,
                        module.key_idx modnum,
                        modulegroup.sort_seq grpsort,
                        module.sort_seq modsort,
                        1234 entries,
                        module.collection collection
                    FROM
                        ${SCHEMA}lookups module
                    LEFT OUTER JOIN
                        ${SCHEMA}lookups modulegroup
                        ON module.value_int = modulegroup.key_idx
                    WHERE
                        (module.lookup_id = 42)
                        AND (module.status_a1 = 1)
                        AND (modulegroup.lookup_id = 48)
                        AND (modulegroup.status_a1 = 1)
                    ORDER BY
                        modulegroup.sort_seq,
                        modulegroup.value_txt,
                        module.sort_seq,
                        module.value_txt
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query retrieves the Main Menu for the app using the same convention
                    as the GLM dataase with "Modules" and "Module Groups".

                    ## Parameters

                    - No parameters at this stage

                    ## Returns

                    - grpname: The name of the module group
                    - grpnum: The key index of the module group
                    - modname: The name of the module
                    - modnum: The key index of the module
                    - grpsort: The sort sequence of the module group
                    - modsort: The sort sequence of the module
                    - entries: The number of entries in the module (hardcoded to 1234 for now)
                    - collection: The collection of the module

                    ## Tables

                    - `${SCHEMA}lookups`: Stores lookup keys

                    ## Notes

                    - This is kind of old-style in that the GLM menu system was written decades ago
                    - This joins two differnet lookups to get our grouped menu items (one level grouping only)
                    - This tends to be plenty sufficient for our purposes, but we might consider other
                      optinos in future to deal with handling "favourite" items and that kind of thing. Maybe.
                    - At the moment this returns everything as we've not gotten around to limiting it to
                      the modules that the user has access to. Getting there.

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

]]})
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
