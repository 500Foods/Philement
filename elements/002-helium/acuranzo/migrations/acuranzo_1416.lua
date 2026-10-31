-- Migration: acuranzo_1416.lua
-- Lookup 072 - AEM Metric

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-30 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1416"
cfg.LOOKUP_ID = "072"
cfg.LOOKUP_NAME = "AEM Metric"

-- ----------------------------------------------------------------------------
-- Forward: Populate Lookup 072 - AEM Metric
-- ----------------------------------------------------------------------------
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
            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (0, ${LOOKUP_ID}, 1, '${LOOKUP_NAME}', 0, 0, '', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 1, 1, 'followers', 1, 1,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - followers

                        Gauge. X account follower count.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 2, 1, 'following', 2, 2,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - following

                        Gauge. X account following count.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 3, 1, 'posts_published', 3, 3,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - posts_published

                        Event. One X post was published. value_num is 1. status_id is in dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 4, 1, 'post_likes', 4, 4,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - post_likes

                        Gauge. Like count on one X post. status_id is in dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 5, 1, 'post_reposts', 5, 5,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - post_reposts

                        Gauge. Repost count on one X post. status_id is in dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 6, 1, 'post_replies', 6, 6,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - post_replies

                        Gauge. Reply count on one X post. status_id is in dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 7, 1, 'post_quotes', 7, 7,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - post_quotes

                        Gauge. Quote count on one X post. status_id is in dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 8, 1, 'post_bookmarks', 8, 8,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - post_bookmarks

                        Gauge. Bookmark count on one X post. status_id is in dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 9, 1, 'post_impressions', 9, 9,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - post_impressions

                        Gauge. Impression count on one X post, from the official API only.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 10, 1, 'feed_hits', 10, 10,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - feed_hits

                        Counter. Requests for one RSS feed in the bin. blog is in dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 11, 1, 'feed_unique_ips', 11, 11,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - feed_unique_ips

                        Counter. Distinct IPs for one RSS feed in the bin. No raw IP is stored.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 12, 1, 'pageviews', 12, 12,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - pageviews

                        Counter. Page views in the bin. path and blog are in dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 13, 1, 'uniques', 13, 13,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - uniques

                        Counter. Anonymous uniques in the bin. No raw IP is stored.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 14, 1, 'ttfb_ms', 14, 14,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - ttfb_ms

                        Gauge. Time to first byte, in milliseconds.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 15, 1, 'page_serve_ms', 15, 15,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - page_serve_ms

                        Gauge. Page serve time, in milliseconds.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 16, 1, 'subscribers', 16, 16,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - subscribers

                        Gauge. Opted-in email subscriber level.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 17, 1, 'send_count', 17, 17,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - send_count

                        Counter. Email sends in the bin. campaign is in dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 18, 1, 'opens', 18, 18,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - opens

                        Counter. Email opens in the bin. campaign is in dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 19, 1, 'clicks', 19, 19,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - clicks

                        Counter. Email clicks in the bin. campaign is in dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 20, 1, 'outage', 20, 20,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - outage

                        Event. A service was down. Detail is in value_text or dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 21, 1, 'capacity_change', 21, 21,
                    '',
                    [==[
                        # ${LOOKUP_ID} - ${LOOKUP_NAME} - capacity_change

                        Event. Capacity changed. Detail is in value_text or dims.
                    ]==],
                    '{}',
                    ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME} in ${TABLE} table'                                                            AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            Seeds Lookup ${LOOKUP_ID} for `metric_a72`.
            Keys: followers, following, posts_published, post_likes, post_reposts, post_replies, post_quotes, post_bookmarks, post_impressions, feed_hits, feed_unique_ips, pageviews, uniques, ttfb_ms, page_serve_ms, subscribers, send_count, opens, clicks, outage, capacity_change.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- ----------------------------------------------------------------------------
-- Reverse: Remove Lookup 072 - AEM Metric
-- ----------------------------------------------------------------------------
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
            WHERE lookup_id = 0
            AND key_idx = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx IN (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Lookup ${LOOKUP_ID} from ${TABLE} Table'                                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Lookup ${LOOKUP_ID} from ${TABLE} Table

            Deletes the family header and the keys this migration inserted.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries
end
