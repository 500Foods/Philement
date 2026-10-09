#!/usr/bin/env bash
# database_reset_lib/firebird.sh — Firebird reset backend for database_reset.sh.
#
# This library is sourced by extras/database_reset.sh after common.sh.
# It provides counting and schema-reset operations for Firebird via isql-fb.
#
# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from database_reset.sh
# TEST_VERSION: 1.0.0

# shellcheck disable=SC2154,SC2034,SC2310,SC2311,SC2312 # CONN_* globals set in database_reset.sh; error handling in || and command substitution

firebird_isql() {
    local sqlfile="$1"
    local dest="$2"
    local lock err rc
    if ! command -v isql-fb >/dev/null 2>&1; then
        warn "Error: isql-fb not found"
        return 1
    fi
    lock="$(make_tmpdir)"
    err="$(make_tmp)"
    set +e
    FIREBIRD_LOCK="${lock}" FIREBIRD_TMP="${lock}" \
        ISC_USER="${CONN_USER}" ISC_PASSWORD="${CONN_PASS}" \
        isql-fb -q -b -pag 0 -ch UTF8 "${CONN_DATABASE}" -i "${sqlfile}" \
        >"${dest}" 2>"${err}"
    rc=$?
    set -e
    if [[ "${rc}" -ne 0 ]]; then
        scrub_file "${err}" "${CONN_PASS}" >&2
        scrub_file "${dest}" "${CONN_PASS}" >&2
        return 1
    fi
    if grep -q -E "Statement failed|SQLSTATE|Dynamic SQL Error" "${dest}" "${err}"; then
        scrub_file "${dest}" "${CONN_PASS}" >&2
        scrub_file "${err}" "${CONN_PASS}" >&2
        return 1
    fi
    return 0
}

count_firebird() {
    local sqlfile dest n
    sqlfile="$(make_tmp)"
    dest="$(make_tmp)"
    cat > "${sqlfile}" <<'EOF'
SET HEADING OFF;
SELECT COUNT(*) FROM RDB$RELATIONS WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0;
EOF
    firebird_isql "${sqlfile}" "${dest}" || return 1
    n="$(read_count "${dest}")" || {
        scrub_file "${dest}" "${CONN_PASS}" >&2
        return 1
    }
    printf '%s\n' "${n}"
}

reset_firebird() {
    local sqlfile dest n
    sqlfile="$(make_tmp)"
    dest="$(make_tmp)"
    cat > "${sqlfile}" <<'EOF'
SET TERM ^ ;
EXECUTE BLOCK AS
    DECLARE i INTEGER;
    DECLARE stmt VARCHAR(500);
    DECLARE rname VARCHAR(63);
    DECLARE cname VARCHAR(63);
BEGIN
    FOR SELECT TRIM(rc.RDB$CONSTRAINT_NAME), TRIM(rc.RDB$RELATION_NAME)
        FROM RDB$RELATION_CONSTRAINTS rc
        JOIN RDB$RELATIONS rel ON rel.RDB$RELATION_NAME = rc.RDB$RELATION_NAME
        WHERE rc.RDB$CONSTRAINT_TYPE = 'FOREIGN KEY'
          AND COALESCE(rel.RDB$SYSTEM_FLAG, 0) = 0
        INTO :cname, :rname
    DO
    BEGIN
        stmt = 'ALTER TABLE ' || :rname || ' DROP CONSTRAINT ' || :cname;
        EXECUTE STATEMENT stmt;
    WHEN ANY DO
    BEGIN
    END
    END

    i = 0;
    WHILE (i < 6) DO
    BEGIN
        i = i + 1;

        FOR SELECT 'DROP TRIGGER ' || TRIM(RDB$TRIGGER_NAME)
            FROM RDB$TRIGGERS
            WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
              AND TRIM(RDB$TRIGGER_NAME) NOT STARTING WITH 'RDB$'
            INTO :stmt
        DO
        BEGIN
            EXECUTE STATEMENT stmt;
        WHEN ANY DO
        BEGIN
        END
        END

        FOR SELECT 'DROP PROCEDURE ' || TRIM(RDB$PROCEDURE_NAME)
            FROM RDB$PROCEDURES
            WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
            INTO :stmt
        DO
        BEGIN
            EXECUTE STATEMENT stmt;
        WHEN ANY DO
        BEGIN
        END
        END

        FOR SELECT 'DROP FUNCTION ' || TRIM(RDB$FUNCTION_NAME)
            FROM RDB$FUNCTIONS
            WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
            INTO :stmt
        DO
        BEGIN
            EXECUTE STATEMENT stmt;
        WHEN ANY DO
        BEGIN
        END
        END

        FOR SELECT 'DROP VIEW ' || TRIM(RDB$RELATION_NAME)
            FROM RDB$RELATIONS
            WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
              AND RDB$VIEW_BLR IS NOT NULL
            INTO :stmt
        DO
        BEGIN
            EXECUTE STATEMENT stmt;
        WHEN ANY DO
        BEGIN
        END
        END

        FOR SELECT 'DROP TABLE ' || TRIM(RDB$RELATION_NAME)
            FROM RDB$RELATIONS
            WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
              AND RDB$VIEW_BLR IS NULL
              AND TRIM(RDB$RELATION_NAME) NOT STARTING WITH 'RDB$'
            INTO :stmt
        DO
        BEGIN
            EXECUTE STATEMENT stmt;
        WHEN ANY DO
        BEGIN
        END
        END
    END

    FOR SELECT 'DROP SEQUENCE ' || TRIM(RDB$GENERATOR_NAME)
        FROM RDB$GENERATORS
        WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
          AND TRIM(RDB$GENERATOR_NAME) NOT STARTING WITH 'RDB$'
        INTO :stmt
    DO
    BEGIN
        EXECUTE STATEMENT stmt;
    WHEN ANY DO
    BEGIN
    END
    END

    FOR SELECT 'DROP EXCEPTION ' || TRIM(RDB$EXCEPTION_NAME)
        FROM RDB$EXCEPTIONS
        WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
        INTO :stmt
    DO
    BEGIN
        EXECUTE STATEMENT stmt;
    WHEN ANY DO
    BEGIN
    END
    END

    FOR SELECT 'DROP DOMAIN ' || TRIM(RDB$FIELD_NAME)
        FROM RDB$FIELDS
        WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
          AND RDB$FIELD_NAME NOT STARTING WITH 'RDB$'
        INTO :stmt
    DO
    BEGIN
        EXECUTE STATEMENT stmt;
    WHEN ANY DO
    BEGIN
    END
    END
END ^
SET TERM ; ^
EOF
    assert_no_drop_database "$(cat "${sqlfile}")" || return 1
    firebird_isql "${sqlfile}" "${dest}" || return 1
    n="$(count_firebird)" || return 1
    [[ "${n}" -eq 0 ]]
}
