-- Migration: acuranzo_1410.lua
-- Login GeoIP: tokens/actions geo_city, geo_region, geo_country
--
-- Traefik (traefikgeoip2 plugin) sets X-GeoIP2-City / -Region / -Country
-- on requests to Hydrogen. Hydrogen trusts them only from
-- Network.TrustedProxies and passes GEOCITY / GEOREGION / GEOCOUNTRY
-- (empty string when absent) to QueryRef #004 and #013. Empty -> NULL.
-- QueryRef #159 (Login History) returns the three columns; Lookup 059
-- key 17 (profile-manager/login-history) gains a Location column.
-- Does not edit 1095 / 1104 / 1406; replaces their stored SQL here.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-28 - Add geo columns; update QueryRef #004, #013, #159

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1410"

local sql4_old = [[
                    INSERT INTO ${SCHEMA}actions (
                        action_id,
                        action_type_a24,
                        app_version,
                        feature_a21,
                        action,
                        action_msecs,
                        ip_address,
                        created_id,
                        created_at
                    )
                    WITH next_action_id AS (
                        SELECT COALESCE(MAX(action_id), 0) + 1 AS new_action_id
                        FROM ${SCHEMA}actions
                    )
                    SELECT
                        new_action_id,
                        2,
                        :APPVERSION,
                        100,
                        :LOGINID,
                        :LOGINTIMER,
                        :IPADDRESS,
                        :LOGINLOGID,
                        ${NOW}
                    FROM next_action_id]]

local sql4_new = [[
                    INSERT INTO ${SCHEMA}actions (
                        action_id,
                        action_type_a24,
                        app_version,
                        feature_a21,
                        action,
                        action_msecs,
                        ip_address,
                        geo_city,
                        geo_region,
                        geo_country,
                        created_id,
                        created_at
                    )
                    WITH next_action_id AS (
                        SELECT COALESCE(MAX(action_id), 0) + 1 AS new_action_id
                        FROM ${SCHEMA}actions
                    )
                    SELECT
                        new_action_id,
                        2,
                        :APPVERSION,
                        100,
                        :LOGINID,
                        :LOGINTIMER,
                        :IPADDRESS,
                        NULLIF(:GEOCITY, ''),
                        NULLIF(:GEOREGION, ''),
                        NULLIF(:GEOCOUNTRY, ''),
                        :LOGINLOGID,
                        ${NOW}
                    FROM next_action_id]]

local sql13_old = [[
                    INSERT INTO ${SCHEMA}tokens (
                        token_hash,
                        account_id,
                        system_id,
                        app_id,
                        app_version,
                        ip_address,
                        valid_after,
                        valid_until
                    )
                    VALUES
                    (
                        :TOKENHASH,
                        :ACCOUNTID,
                        :SYSTEMID,
                        :APPID,
                        :APPVERSION,
                        :IPADDRESS,
                        ${NOW},
                        ${TRFS} :JWTDURATION ${TRFE}
                    )]]

local sql13_new = [[
                    INSERT INTO ${SCHEMA}tokens (
                        token_hash,
                        account_id,
                        system_id,
                        app_id,
                        app_version,
                        ip_address,
                        geo_city,
                        geo_region,
                        geo_country,
                        valid_after,
                        valid_until
                    )
                    VALUES
                    (
                        :TOKENHASH,
                        :ACCOUNTID,
                        :SYSTEMID,
                        :APPID,
                        :APPVERSION,
                        :IPADDRESS,
                        NULLIF(:GEOCITY, ''),
                        NULLIF(:GEOREGION, ''),
                        NULLIF(:GEOCOUNTRY, ''),
                        ${NOW},
                        ${TRFS} :JWTDURATION ${TRFE}
                    )]]

local sql159_old = [[
                    SELECT * FROM (
                        SELECT
                            a.action_id,
                            a.action_type_a24,
                            CASE a.action_type_a24
                                WHEN 2 THEN 'Login Attempt'
                                WHEN 3 THEN 'Login Successful'
                                WHEN 4 THEN 'Login Passed'
                                ELSE 'Other'
                            END AS action_type_name,
                            a.action AS login_id,
                            a.ip_address,
                            a.action_msecs,
                            a.app_version,
                            a.created_at
                          FROM ${SCHEMA}actions a
                         WHERE a.feature_a21 = 100
                           AND (
                                (a.action_type_a24 IN (3, 4) AND a.account_id = :ACCOUNTID)
                             OR (a.action_type_a24 = 2 AND LOWER(a.action) IN (
                                    SELECT LOWER(ac.contact) FROM ${SCHEMA}account_contacts ac
                                     WHERE ac.account_id = :ACCOUNTID
                                    UNION
                                    SELECT LOWER(acc.name) FROM ${SCHEMA}accounts acc
                                     WHERE acc.account_id = :ACCOUNTID
                                ))
                           )
                        UNION ALL
                        SELECT
                            NULL AS action_id,
                            0 AS action_type_a24,
                            'Token Issued' AS action_type_name,
                            acc.name AS login_id,
                            TRIM(t.ip_address) AS ip_address,
                            NULL AS action_msecs,
                            TRIM(t.app_version) AS app_version,
                            t.valid_after AS created_at
                          FROM ${SCHEMA}tokens t
                          LEFT JOIN ${SCHEMA}accounts acc ON acc.account_id = t.account_id
                         WHERE t.account_id = :ACCOUNTID
                    ) h
                     ORDER BY h.created_at DESC
                     LIMIT 200]]

local sql159_new = [[
                    SELECT * FROM (
                        SELECT
                            a.action_id,
                            a.action_type_a24,
                            CASE a.action_type_a24
                                WHEN 2 THEN 'Login Attempt'
                                WHEN 3 THEN 'Login Successful'
                                WHEN 4 THEN 'Login Passed'
                                ELSE 'Other'
                            END AS action_type_name,
                            a.action AS login_id,
                            a.ip_address,
                            a.geo_city,
                            a.geo_region,
                            a.geo_country,
                            a.action_msecs,
                            a.app_version,
                            a.created_at
                          FROM ${SCHEMA}actions a
                         WHERE a.feature_a21 = 100
                           AND (
                                (a.action_type_a24 IN (3, 4) AND a.account_id = :ACCOUNTID)
                             OR (a.action_type_a24 = 2 AND LOWER(a.action) IN (
                                    SELECT LOWER(ac.contact) FROM ${SCHEMA}account_contacts ac
                                     WHERE ac.account_id = :ACCOUNTID
                                    UNION
                                    SELECT LOWER(acc.name) FROM ${SCHEMA}accounts acc
                                     WHERE acc.account_id = :ACCOUNTID
                                ))
                           )
                        UNION ALL
                        SELECT
                            NULL AS action_id,
                            0 AS action_type_a24,
                            'Token Issued' AS action_type_name,
                            acc.name AS login_id,
                            TRIM(t.ip_address) AS ip_address,
                            t.geo_city,
                            t.geo_region,
                            t.geo_country,
                            NULL AS action_msecs,
                            TRIM(t.app_version) AS app_version,
                            t.valid_after AS created_at
                          FROM ${SCHEMA}tokens t
                          LEFT JOIN ${SCHEMA}accounts acc ON acc.account_id = t.account_id
                         WHERE t.account_id = :ACCOUNTID
                    ) h
                     ORDER BY h.created_at DESC
                     LIMIT 200]]

-- Forward
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
            ALTER TABLE ${SCHEMA}tokens
                ADD COLUMN geo_city ${VARCHAR_100};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}tokens
                ADD COLUMN geo_region ${VARCHAR_100};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}tokens
                ADD COLUMN geo_country ${VARCHAR_100};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}actions
                ADD COLUMN geo_city ${VARCHAR_100};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}actions
                ADD COLUMN geo_region ${VARCHAR_100};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}actions
                ADD COLUMN geo_country ${VARCHAR_100};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql4_new .. [[
            ]==]
            WHERE query_ref = 4
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql13_new .. [[
            ]==]
            WHERE query_ref = 13
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql159_new .. [[
            ]==]
            WHERE query_ref = 159
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Read-only Recent Activity table on Profile > Security > Login History. Lookup 059 key 17 (acuranzo_1408). Rows from QueryRef 159, pre-formatted client-side.",
  "table": "login-history",
  "title": "Login History",
  "queryRef": 159,
  "readonly": true,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": false,
  "initialSort": [
    {
      "column": "when_txt",
      "dir": "desc"
    }
  ],
  "columns": {
    "row_id": {
      "title": "#",
      "field": "row_id",
      "coltype": "index",
      "visible": false,
      "sort": true,
      "filter": false,
      "editable": false,
      "calculated": false,
      "primaryKey": true,
      "description": "Client-side row number"
    },
    "when_txt": {
      "title": "When",
      "field": "when_txt",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login event time (24h, PCT)",
      "width": 170,
      "bottomCalc": "count",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": {
        "thousand": ""
      }
    },
    "result_txt": {
      "title": "Result",
      "field": "result_txt",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login event type (Attempt / Success / Passed)",
      "width": 100
    },
    "login_id": {
      "title": "Login ID",
      "field": "login_id",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login ID",
      "width": 160
    },
    "ip_address": {
      "title": "IP",
      "field": "ip_address",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "IP",
      "width": 130
    },
    "location_txt": {
      "title": "Location",
      "field": "location_txt",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "City, region, country from Traefik GeoIP (X-GeoIP2-*) at login time",
      "width": 200
    },
    "action_msecs": {
      "title": "Duration (ms)",
      "field": "action_msecs",
      "coltype": "integer",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login duration in milliseconds",
      "width": 110
    },
    "app_version": {
      "title": "App Version",
      "field": "app_version",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "App Version"
    }
  }
}
]===]
                ${JSON_INGEST_END}
            WHERE lookup_id = 59
            AND key_idx = 17;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Login GeoIP columns + QueryRef #004/#013/#159'                                                            AS name,
        [=[
            # Forward Migration ${MIGRATION}: Login GeoIP

            Adds nullable geo_city, geo_region, geo_country (VARCHAR_100)
            to tokens and actions. QueryRef #004 and #013 now require
            STRING GEOCITY, GEOREGION, GEOCOUNTRY (Hydrogen sends empty
            string when no trusted geo headers). QueryRef #159 returns them.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- Reverse
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
            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql4_old .. [[
            ]==]
            WHERE query_ref = 4
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql13_old .. [[
            ]==]
            WHERE query_ref = 13
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql159_old .. [[
            ]==]
            WHERE query_ref = 159
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Read-only Recent Activity table on Profile > Security > Login History. Lookup 059 key 17 (acuranzo_1408). Rows from QueryRef 159, pre-formatted client-side.",
  "table": "login-history",
  "title": "Login History",
  "queryRef": 159,
  "readonly": true,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": false,
  "initialSort": [
    {
      "column": "when_txt",
      "dir": "desc"
    }
  ],
  "columns": {
    "row_id": {
      "title": "#",
      "field": "row_id",
      "coltype": "index",
      "visible": false,
      "sort": true,
      "filter": false,
      "editable": false,
      "calculated": false,
      "primaryKey": true,
      "description": "Client-side row number"
    },
    "when_txt": {
      "title": "When",
      "field": "when_txt",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login event time (24h, PCT)",
      "width": 170,
      "bottomCalc": "count",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": {
        "thousand": ""
      }
    },
    "result_txt": {
      "title": "Result",
      "field": "result_txt",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login event type (Attempt / Success / Passed)",
      "width": 100
    },
    "login_id": {
      "title": "Login ID",
      "field": "login_id",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login ID",
      "width": 160
    },
    "ip_address": {
      "title": "IP",
      "field": "ip_address",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "IP",
      "width": 130
    },
    "action_msecs": {
      "title": "Duration (ms)",
      "field": "action_msecs",
      "coltype": "integer",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login duration in milliseconds",
      "width": 110
    },
    "app_version": {
      "title": "App Version",
      "field": "app_version",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "App Version"
    }
  }
}
]===]
                ${JSON_INGEST_END}
            WHERE lookup_id = 59
            AND key_idx = 17;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}tokens
                DROP COLUMN geo_city;

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}tokens
                DROP COLUMN geo_region;

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}tokens
                DROP COLUMN geo_country;

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}actions
                DROP COLUMN geo_city;

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}actions
                DROP COLUMN geo_region;

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}actions
                DROP COLUMN geo_country;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop Login GeoIP columns'                                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop Login GeoIP

            Restores QueryRef #004, #013, #159 SQL, then drops the columns.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
return queries end
