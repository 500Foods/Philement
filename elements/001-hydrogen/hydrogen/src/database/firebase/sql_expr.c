/*
 * Parse and evaluate Acuranzo INSERT/UPDATE expressions. FB_* functions
 * run in-process; binary decode output stays length-tagged.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "sql_parse.h"
#include "sql_expr.h"
#include "fns_base64.h"
#include "fns_brotli.h"
#include "fns_json.h"
#include "fns_sha256.h"
#include "fns_tz.h"

#include <ctype.h>
#include <errno.h>

FirebaseExpr* firebase_expr_alloc(FirebaseExprKind kind) {
    FirebaseExpr* expr = calloc(1, sizeof(FirebaseExpr));
    if (!expr) {
        return NULL;
    }
    expr->kind = kind;
    return expr;
}

void firebase_expr_free(FirebaseExpr* expr) {
    if (!expr) {
        return;
    }
    free(expr->text);
    if (expr->args) {
        for (size_t i = 0; i < expr->arg_count; i++) {
            firebase_expr_free(expr->args[i]);
        }
        free(expr->args);
    }
    free(expr);
}

bool firebase_expr_add_arg(FirebaseExpr* expr, FirebaseExpr* arg) {
    if (!expr || !arg) {
        firebase_expr_free(arg);
        return false;
    }
    FirebaseExpr** grown = realloc(expr->args, (expr->arg_count + 1) * sizeof(FirebaseExpr*));
    if (!grown) {
        firebase_expr_free(arg);
        return false;
    }
    expr->args = grown;
    expr->args[expr->arg_count] = arg;
    expr->arg_count++;
    return true;
}

bool firebase_expr_set_eval_error(char** error, const char* message) {
    if (error) {
        free(*error);
        *error = strdup(message ? message : "expression failed");
    }
    return false;
}

FirebaseExpr* firebase_expr_parse_primary(const char** cursor, char** error) {
    if (!cursor || !*cursor) {
        firebase_expr_set_eval_error(error, "expression is NULL");
        return NULL;
    }
    firebase_sql_skip(cursor);

    if (firebase_sql_match_keyword(cursor, "NULL")) {
        return firebase_expr_alloc(FIREBASE_EXPR_NULL);
    }

    if (**cursor == '\'') {
        char* inner = NULL;
        if (!firebase_sql_parse_string(cursor, &inner) || !inner) {
            firebase_expr_set_eval_error(error, "unterminated string");
            return NULL;
        }
        FirebaseExpr* expr = firebase_expr_alloc(FIREBASE_EXPR_STRING);
        if (!expr) {
            free(inner);
            firebase_expr_set_eval_error(error, "out of memory");
            return NULL;
        }
        expr->text = inner;
        return expr;
    }

    if (**cursor == '(') {
        (*cursor)++;
        FirebaseExpr* inner = firebase_expr_parse(cursor, error);
        firebase_sql_skip(cursor);
        if (!inner || **cursor != ')') {
            firebase_expr_free(inner);
            firebase_expr_set_eval_error(error, "expected closing parenthesis");
            return NULL;
        }
        (*cursor)++;
        return inner;
    }

    unsigned char first = (unsigned char)**cursor;
    if (isdigit(first) || ((first == '+' || first == '-') && isdigit((unsigned char)(*cursor)[1]))) {
        char* literal = firebase_sql_parse_literal(cursor);
        if (!literal) {
            firebase_expr_set_eval_error(error, "invalid number");
            return NULL;
        }
        FirebaseExprKind kind = strchr(literal, '.') ? FIREBASE_EXPR_NUMBER : FIREBASE_EXPR_INTEGER;
        FirebaseExpr* expr = firebase_expr_alloc(kind);
        if (!expr) {
            free(literal);
            firebase_expr_set_eval_error(error, "out of memory");
            return NULL;
        }
        expr->text = literal;
        return expr;
    }

    char* ident = NULL;
    if (!firebase_sql_parse_ident(cursor, &ident)) {
        firebase_expr_set_eval_error(error, "expected expression");
        return NULL;
    }

    firebase_sql_skip(cursor);
    if (**cursor != '(') {
        FirebaseExpr* expr = firebase_expr_alloc(FIREBASE_EXPR_IDENT);
        if (!expr) {
            free(ident);
            firebase_expr_set_eval_error(error, "out of memory");
            return NULL;
        }
        expr->text = ident;
        return expr;
    }

    (*cursor)++;
    FirebaseExpr* call = firebase_expr_alloc(FIREBASE_EXPR_CALL);
    if (!call) {
        free(ident);
        firebase_expr_set_eval_error(error, "out of memory");
        return NULL;
    }
    call->text = ident;
    firebase_sql_skip(cursor);
    if (**cursor == ')') {
        (*cursor)++;
        return call;
    }
    while (1) {
        FirebaseExpr* arg = firebase_expr_parse(cursor, error);
        if (!arg || !firebase_expr_add_arg(call, arg)) {
            firebase_expr_free(call);
            if (!arg) {
                return NULL;
            }
            firebase_expr_set_eval_error(error, "out of memory");
            return NULL;
        }
        firebase_sql_skip(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }
        if (**cursor == ')') {
            (*cursor)++;
            return call;
        }
        firebase_expr_free(call);
        firebase_expr_set_eval_error(error, "expected comma or closing parenthesis");
        return NULL;
    }
}

FirebaseExpr* firebase_expr_parse(const char** cursor, char** error) {
    return firebase_expr_parse_primary(cursor, error);
}

void firebase_value_init(FirebaseValue* value) {
    if (value) {
        memset(value, 0, sizeof(*value));
    }
}

void firebase_value_free(FirebaseValue* value) {
    if (!value) {
        return;
    }
    free(value->data);
    memset(value, 0, sizeof(*value));
}

bool firebase_value_set_null(FirebaseValue* value) {
    if (!value) {
        return false;
    }
    firebase_value_free(value);
    value->kind = FIREBASE_VAL_NULL;
    return true;
}

bool firebase_value_set_int(FirebaseValue* value, long long number) {
    if (!value) {
        return false;
    }
    firebase_value_free(value);
    value->kind = FIREBASE_VAL_INT;
    value->i = number;
    return true;
}

bool firebase_value_set_double(FirebaseValue* value, double number) {
    if (!value) {
        return false;
    }
    firebase_value_free(value);
    value->kind = FIREBASE_VAL_DOUBLE;
    value->d = number;
    return true;
}

bool firebase_value_copy_text(FirebaseValue* value, const char* text) {
    if (!value) {
        return false;
    }
    firebase_value_free(value);
    if (!text) {
        value->kind = FIREBASE_VAL_NULL;
        return true;
    }
    size_t n = strlen(text);
    char* copy = malloc(n + 1);
    if (!copy) {
        return false;
    }
    memcpy(copy, text, n + 1);
    value->kind = FIREBASE_VAL_TEXT;
    value->data = copy;
    value->length = n;
    return true;
}

bool firebase_value_take_data(FirebaseValue* value, FirebaseValKind kind, char* data, size_t length) {
    if (!value) {
        free(data);
        return false;
    }
    firebase_value_free(value);
    value->kind = kind;
    value->data = data;
    value->length = length;
    return true;
}

char* firebase_value_as_text(const FirebaseValue* value) {
    if (!value || value->kind == FIREBASE_VAL_NULL) {
        return NULL;
    }
    if (value->kind == FIREBASE_VAL_TEXT || value->kind == FIREBASE_VAL_BYTES) {
        char* copy = malloc(value->length + 1);
        if (!copy) {
            return NULL;
        }
        if (value->data && value->length > 0) {
            memcpy(copy, value->data, value->length);
        }
        copy[value->length] = '\0';
        return copy;
    }
    char buf[64];
    if (value->kind == FIREBASE_VAL_INT) {
        snprintf(buf, sizeof(buf), "%lld", value->i);
    } else {
        snprintf(buf, sizeof(buf), "%.15g", value->d);
    }
    return strdup(buf);
}

bool firebase_value_as_int(const FirebaseValue* value, long long* out) {
    if (!value || !out || value->kind == FIREBASE_VAL_NULL) {
        return false;
    }
    if (value->kind == FIREBASE_VAL_INT) {
        *out = value->i;
        return true;
    }
    if (value->kind == FIREBASE_VAL_DOUBLE) {
        *out = (long long)value->d;
        return true;
    }
    if ((value->kind == FIREBASE_VAL_TEXT || value->kind == FIREBASE_VAL_BYTES) && value->data) {
        char* end = NULL;
        errno = 0;
        long long parsed = strtoll(value->data, &end, 10);
        if (errno != 0 || end == value->data) {
            return false;
        }
        *out = parsed;
        return true;
    }
    return false;
}

bool firebase_value_is_null(const FirebaseValue* value) {
    return !value || value->kind == FIREBASE_VAL_NULL;
}

size_t firebase_value_byte_size(const FirebaseValue* value) {
    if (!value || value->kind == FIREBASE_VAL_NULL) {
        return 0;
    }
    if (value->kind == FIREBASE_VAL_TEXT || value->kind == FIREBASE_VAL_BYTES) {
        return value->length;
    }
    return 0;
}

bool firebase_value_exceeds_max(const FirebaseValue* value) {
    return firebase_value_byte_size(value) > FIREBASE_MAX_FIELD_BYTES;
}

bool firebase_value_equals(const FirebaseValue* left, const FirebaseValue* right) {
    if (firebase_value_is_null(left) && firebase_value_is_null(right)) {
        return true;
    }
    if (firebase_value_is_null(left) || firebase_value_is_null(right)) {
        return false;
    }
    long long li = 0;
    long long ri = 0;
    if (firebase_value_as_int(left, &li) && firebase_value_as_int(right, &ri) &&
        left->kind != FIREBASE_VAL_TEXT && right->kind != FIREBASE_VAL_TEXT &&
        left->kind != FIREBASE_VAL_BYTES && right->kind != FIREBASE_VAL_BYTES) {
        return li == ri;
    }
    if ((left->kind == FIREBASE_VAL_INT || left->kind == FIREBASE_VAL_DOUBLE) &&
        (right->kind == FIREBASE_VAL_INT || right->kind == FIREBASE_VAL_DOUBLE)) {
        return li == ri;
    }
    char* lt = firebase_value_as_text(left);
    char* rt = firebase_value_as_text(right);
    bool same = lt && rt && strcmp(lt, rt) == 0;
    free(lt);
    free(rt);
    return same;
}

bool firebase_expr_eval(const FirebaseExpr* expr, FirebaseExprLookup lookup, void* userdata,
                        FirebaseValue* out, char** error) {
    if (!out) {
        return false;
    }
    firebase_value_init(out);
    if (!expr) {
        return firebase_expr_set_eval_error(error, "expression is NULL");
    }
    if (expr->kind == FIREBASE_EXPR_NULL) {
        return firebase_value_set_null(out);
    }
    if (expr->kind == FIREBASE_EXPR_INTEGER) {
        if (!expr->text) {
            return firebase_expr_set_eval_error(error, "invalid integer");
        }
        char* end = NULL;
        errno = 0;
        long long number = strtoll(expr->text, &end, 10);
        if (errno != 0 || !end || end == expr->text) {
            return firebase_expr_set_eval_error(error, "invalid integer");
        }
        return firebase_value_set_int(out, number);
    }
    if (expr->kind == FIREBASE_EXPR_NUMBER) {
        if (!expr->text) {
            return firebase_expr_set_eval_error(error, "invalid number");
        }
        char* end = NULL;
        errno = 0;
        double number = strtod(expr->text, &end);
        if (errno != 0 || !end || end == expr->text) {
            return firebase_expr_set_eval_error(error, "invalid number");
        }
        return firebase_value_set_double(out, number);
    }
    if (expr->kind == FIREBASE_EXPR_STRING) {
        return firebase_value_copy_text(out, expr->text ? expr->text : "");
    }
    if (expr->kind == FIREBASE_EXPR_IDENT) {
        if (!lookup || !expr->text) {
            return firebase_expr_set_eval_error(error, "column reference is not allowed here");
        }
        if (!lookup(expr->text, out, userdata)) {
            return firebase_value_set_null(out);
        }
        return true;
    }
    if (expr->kind == FIREBASE_EXPR_CALL) {
        return firebase_expr_eval_call(expr, lookup, userdata, out, error);
    }
    return firebase_expr_set_eval_error(error, "unsupported expression");
}

bool firebase_sql_where_add(FirebaseSqlWhere* where, FirebaseSqlPredicate pred) {
    if (!where) {
        free(pred.column);
        firebase_expr_free(pred.value);
        if (pred.in_values) {
            for (size_t i = 0; i < pred.in_count; i++) {
                firebase_expr_free(pred.in_values[i]);
            }
            free(pred.in_values);
        }
        return false;
    }
    FirebaseSqlPredicate* grown = realloc(where->predicates,
                                          (where->count + 1) * sizeof(FirebaseSqlPredicate));
    if (!grown) {
        free(pred.column);
        firebase_expr_free(pred.value);
        if (pred.in_values) {
            for (size_t i = 0; i < pred.in_count; i++) {
                firebase_expr_free(pred.in_values[i]);
            }
            free(pred.in_values);
        }
        return false;
    }
    where->predicates = grown;
    where->predicates[where->count] = pred;
    where->count++;
    return true;
}

bool firebase_sql_parse_where(const char** cursor, FirebaseSqlWhere* where, FirebaseSqlStatement* stmt) {
    if (!firebase_sql_match_keyword(cursor, "WHERE")) {
        return firebase_sql_set_error(stmt, "WHERE is required");
    }
    do {
        FirebaseSqlPredicate pred = {0};
        if (!firebase_sql_parse_ident(cursor, &pred.column)) {
            return firebase_sql_set_error(stmt, "WHERE: missing column");
        }
        if (firebase_sql_match_keyword(cursor, "IS")) {
            if (!firebase_sql_match_keyword(cursor, "NULL")) {
                free(pred.column);
                return firebase_sql_set_error(stmt, "WHERE: expected NULL after IS");
            }
            pred.op = FIREBASE_PRED_IS_NULL;
        } else if (firebase_sql_match_keyword(cursor, "IN")) {
            firebase_sql_skip(cursor);
            if (**cursor != '(') {
                free(pred.column);
                return firebase_sql_set_error(stmt, "WHERE: expected ( after IN");
            }
            (*cursor)++;
            pred.op = FIREBASE_PRED_IN;
            while (1) {
                char* err = NULL;
                FirebaseExpr* item = firebase_expr_parse(cursor, &err);
                if (!item) {
                    free(pred.column);
                    firebase_sql_set_error(stmt, err ? err : "WHERE: invalid IN value");
                    free(err);
                    return false;
                }
                FirebaseExpr** grown = realloc(pred.in_values, (pred.in_count + 1) * sizeof(FirebaseExpr*));
                if (!grown) {
                    firebase_expr_free(item);
                    free(pred.column);
                    return firebase_sql_set_error(stmt, "WHERE: out of memory");
                }
                pred.in_values = grown;
                pred.in_values[pred.in_count] = item;
                pred.in_count++;
                firebase_sql_skip(cursor);
                if (**cursor == ',') {
                    (*cursor)++;
                    continue;
                }
                if (**cursor == ')') {
                    (*cursor)++;
                    break;
                }
                free(pred.column);
                if (pred.in_values) {
                    for (size_t i = 0; i < pred.in_count; i++) {
                        firebase_expr_free(pred.in_values[i]);
                    }
                    free(pred.in_values);
                }
                return firebase_sql_set_error(stmt, "WHERE: expected comma or ) in IN list");
            }
        } else {
            firebase_sql_skip(cursor);
            if (**cursor != '=') {
                free(pred.column);
                return firebase_sql_set_error(stmt, "WHERE: expected =, IS, or IN");
            }
            (*cursor)++;
            pred.op = FIREBASE_PRED_EQ;
            char* err = NULL;
            pred.value = firebase_expr_parse(cursor, &err);
            if (!pred.value) {
                free(pred.column);
                firebase_sql_set_error(stmt, err ? err : "WHERE: invalid value");
                free(err);
                return false;
            }
            free(err);
        }
        if (!firebase_sql_where_add(where, pred)) {
            return firebase_sql_set_error(stmt, "WHERE: out of memory");
        }
    } while (firebase_sql_match_keyword(cursor, "AND"));
    return true;
}

bool firebase_sql_parse_insert(const char** cursor, FirebaseSqlStatement* stmt) {
    stmt->kind = FIREBASE_SQL_KIND_INSERT;
    if (!firebase_sql_match_keyword(cursor, "INTO")) {
        return firebase_sql_set_error(stmt, "INSERT: expected INTO");
    }
    if (!firebase_sql_parse_ident(cursor, &stmt->insert.table)) {
        return firebase_sql_set_error(stmt, "INSERT: missing table name");
    }
    firebase_sql_skip(cursor);
    if (**cursor != '(') {
        stmt->kind = FIREBASE_SQL_KIND_UNSUPPORTED;
        return true;
    }
    FirebaseSqlKey cols = {0};
    if (!firebase_sql_parse_ident_list(cursor, &cols)) {
        return firebase_sql_set_error(stmt, "INSERT: invalid column list");
    }
    stmt->insert.columns = cols.columns;
    stmt->insert.column_count = cols.count;
    if (!firebase_sql_match_keyword(cursor, "VALUES")) {
        stmt->kind = FIREBASE_SQL_KIND_UNSUPPORTED;
        return true;
    }
    while (1) {
        firebase_sql_skip(cursor);
        if (**cursor != '(') {
            return firebase_sql_set_error(stmt, "INSERT: expected value row");
        }
        (*cursor)++;
        FirebaseSqlInsertRow row = {0};
        while (1) {
            char* err = NULL;
            FirebaseExpr* expr = firebase_expr_parse(cursor, &err);
            if (!expr) {
                if (row.values) {
                    for (size_t i = 0; i < row.count; i++) {
                        firebase_expr_free(row.values[i]);
                    }
                    free(row.values);
                }
                firebase_sql_set_error(stmt, err ? err : "INSERT: invalid value");
                free(err);
                return false;
            }
            FirebaseExpr** grown = realloc(row.values, (row.count + 1) * sizeof(FirebaseExpr*));
            if (!grown) {
                firebase_expr_free(expr);
                free(row.values);
                return firebase_sql_set_error(stmt, "INSERT: out of memory");
            }
            row.values = grown;
            row.values[row.count] = expr;
            row.count++;
            firebase_sql_skip(cursor);
            if (**cursor == ',') {
                (*cursor)++;
                continue;
            }
            if (**cursor == ')') {
                (*cursor)++;
                break;
            }
            for (size_t i = 0; i < row.count; i++) {
                firebase_expr_free(row.values[i]);
            }
            free(row.values);
            return firebase_sql_set_error(stmt, "INSERT: expected comma or )");
        }
        if (row.count != stmt->insert.column_count) {
            for (size_t i = 0; i < row.count; i++) {
                firebase_expr_free(row.values[i]);
            }
            free(row.values);
            return firebase_sql_set_error(stmt, "INSERT: column/value count mismatch");
        }
        FirebaseSqlInsertRow* grown = realloc(stmt->insert.rows,
                                              (stmt->insert.row_count + 1) * sizeof(FirebaseSqlInsertRow));
        if (!grown) {
            for (size_t i = 0; i < row.count; i++) {
                firebase_expr_free(row.values[i]);
            }
            free(row.values);
            return firebase_sql_set_error(stmt, "INSERT: out of memory");
        }
        stmt->insert.rows = grown;
        stmt->insert.rows[stmt->insert.row_count] = row;
        stmt->insert.row_count++;
        firebase_sql_skip(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }
        break;
    }
    return true;
}

bool firebase_sql_parse_update(const char** cursor, FirebaseSqlStatement* stmt) {
    stmt->kind = FIREBASE_SQL_KIND_UPDATE;
    if (!firebase_sql_parse_ident(cursor, &stmt->update.table)) {
        return firebase_sql_set_error(stmt, "UPDATE: missing table name");
    }
    if (!firebase_sql_match_keyword(cursor, "SET")) {
        return firebase_sql_set_error(stmt, "UPDATE: expected SET");
    }
    while (1) {
        FirebaseSqlAssign assign = {0};
        if (!firebase_sql_parse_ident(cursor, &assign.column)) {
            return firebase_sql_set_error(stmt, "UPDATE: missing column");
        }
        firebase_sql_skip(cursor);
        if (**cursor != '=') {
            free(assign.column);
            return firebase_sql_set_error(stmt, "UPDATE: expected =");
        }
        (*cursor)++;
        char* err = NULL;
        assign.value = firebase_expr_parse(cursor, &err);
        if (!assign.value) {
            free(assign.column);
            firebase_sql_set_error(stmt, err ? err : "UPDATE: invalid value");
            free(err);
            return false;
        }
        free(err);
        FirebaseSqlAssign* grown = realloc(stmt->update.sets,
                                           (stmt->update.set_count + 1) * sizeof(FirebaseSqlAssign));
        if (!grown) {
            free(assign.column);
            firebase_expr_free(assign.value);
            return firebase_sql_set_error(stmt, "UPDATE: out of memory");
        }
        stmt->update.sets = grown;
        stmt->update.sets[stmt->update.set_count] = assign;
        stmt->update.set_count++;
        firebase_sql_skip(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }
        break;
    }
    return firebase_sql_parse_where(cursor, &stmt->update.where, stmt);
}

bool firebase_sql_parse_delete(const char** cursor, FirebaseSqlStatement* stmt) {
    stmt->kind = FIREBASE_SQL_KIND_DELETE;
    if (!firebase_sql_match_keyword(cursor, "FROM")) {
        return firebase_sql_set_error(stmt, "DELETE: expected FROM");
    }
    if (!firebase_sql_parse_ident(cursor, &stmt->del.table)) {
        return firebase_sql_set_error(stmt, "DELETE: missing table name");
    }
    return firebase_sql_parse_where(cursor, &stmt->del.where, stmt);
}

bool firebase_expr_eval_call(const FirebaseExpr* expr, FirebaseExprLookup lookup, void* userdata,
                             FirebaseValue* out, char** error) {
    if (!expr || !expr->text || !out) {
        return firebase_expr_set_eval_error(error, "invalid function call");
    }

    FirebaseValue* args = NULL;
    if (expr->arg_count > 0) {
        args = calloc(expr->arg_count, sizeof(FirebaseValue));
        if (!args) {
            return firebase_expr_set_eval_error(error, "out of memory");
        }
    }
    for (size_t i = 0; i < expr->arg_count; i++) {
        if (!firebase_expr_eval(expr->args[i], lookup, userdata, &args[i], error)) {
            for (size_t j = 0; j <= i; j++) {
                firebase_value_free(&args[j]);
            }
            free(args);
            return false;
        }
    }

    bool ok = false;
    const char* name = expr->text;
    if (strcasecmp(name, "FB_NOW") == 0) {
        if (expr->arg_count != 0) {
            firebase_expr_set_eval_error(error, "FB_NOW takes no arguments");
        } else {
            char* now = firebase_now();
            ok = now && firebase_value_take_data(out, FIREBASE_VAL_TEXT, now, strlen(now));
            if (!ok) {
                free(now);
                firebase_expr_set_eval_error(error, "FB_NOW failed");
            }
        }
    } else if (strcasecmp(name, "FB_BASE64_DECODE") == 0) {
        if (expr->arg_count != 1 || firebase_value_is_null(&args[0])) {
            firebase_expr_set_eval_error(error, "FB_BASE64_DECODE expects one string");
        } else {
            char* text = firebase_value_as_text(&args[0]);
            size_t out_len = 0;
            unsigned char* decoded = text ? firebase_base64_decode(text, &out_len) : NULL;
            free(text);
            ok = decoded && firebase_value_take_data(out, FIREBASE_VAL_BYTES, (char*)decoded, out_len);
            if (!ok) {
                free(decoded);
                firebase_expr_set_eval_error(error, "FB_BASE64_DECODE failed");
            }
        }
    } else if (strcasecmp(name, "FB_BASE64_ENCODE") == 0) {
        if (expr->arg_count != 1 || firebase_value_is_null(&args[0])) {
            firebase_expr_set_eval_error(error, "FB_BASE64_ENCODE expects one argument");
        } else {
            const unsigned char* data = (const unsigned char*)(args[0].data ? args[0].data : "");
            size_t len = args[0].length;
            char* encoded = firebase_base64_encode(data, len);
            ok = encoded && firebase_value_take_data(out, FIREBASE_VAL_TEXT, encoded, strlen(encoded));
            if (!ok) {
                free(encoded);
                firebase_expr_set_eval_error(error, "FB_BASE64_ENCODE failed");
            }
        }
    } else if (strcasecmp(name, "FB_BROTLI_DECOMPRESS") == 0) {
        if (expr->arg_count != 1 || firebase_value_is_null(&args[0])) {
            firebase_expr_set_eval_error(error, "FB_BROTLI_DECOMPRESS expects one argument");
        } else {
            const unsigned char* data = (const unsigned char*)(args[0].data ? args[0].data : "");
            size_t len = args[0].length;
            size_t out_len = 0;
            char* plain = firebase_brotli_decompress(data, len, &out_len);
            ok = plain && firebase_value_take_data(out, FIREBASE_VAL_TEXT, plain, out_len);
            if (!ok) {
                free(plain);
                firebase_expr_set_eval_error(error, "FB_BROTLI_DECOMPRESS failed");
            }
        }
    } else if (strcasecmp(name, "FB_SHA256_B64") == 0) {
        if (expr->arg_count != 2 || firebase_value_is_null(&args[0]) || firebase_value_is_null(&args[1])) {
            firebase_expr_set_eval_error(error, "FB_SHA256_B64 expects two strings");
        } else {
            char* a = firebase_value_as_text(&args[0]);
            char* b = firebase_value_as_text(&args[1]);
            char* hash = (a && b) ? firebase_sha256_b64(a, b) : NULL;
            free(a);
            free(b);
            ok = hash && firebase_value_take_data(out, FIREBASE_VAL_TEXT, hash, strlen(hash));
            if (!ok) {
                free(hash);
                firebase_expr_set_eval_error(error, "FB_SHA256_B64 failed");
            }
        }
    } else if (strcasecmp(name, "FB_JSON_INGEST") == 0) {
        if (expr->arg_count != 1) {
            firebase_expr_set_eval_error(error, "FB_JSON_INGEST expects one argument");
        } else if (firebase_value_is_null(&args[0])) {
            ok = firebase_value_set_null(out);
        } else {
            char* text = firebase_value_as_text(&args[0]);
            char* ingested = text ? firebase_json_ingest(text) : NULL;
            free(text);
            ok = ingested && firebase_value_take_data(out, FIREBASE_VAL_TEXT, ingested, strlen(ingested));
            if (!ok) {
                free(ingested);
                firebase_expr_set_eval_error(error, "FB_JSON_INGEST failed");
            }
        }
    } else if (strcasecmp(name, "FB_JSON_VALUE") == 0) {
        if (expr->arg_count != 2) {
            firebase_expr_set_eval_error(error, "FB_JSON_VALUE expects two arguments");
        } else if (firebase_value_is_null(&args[0]) || firebase_value_is_null(&args[1])) {
            ok = firebase_value_set_null(out);
        } else {
            char* json_text = firebase_value_as_text(&args[0]);
            char* path = firebase_value_as_text(&args[1]);
            char* extracted = (json_text && path) ? firebase_json_value(json_text, path) : NULL;
            free(json_text);
            free(path);
            if (!extracted) {
                ok = firebase_value_set_null(out);
            } else {
                ok = firebase_value_take_data(out, FIREBASE_VAL_TEXT, extracted, strlen(extracted));
                if (!ok) {
                    free(extracted);
                    firebase_expr_set_eval_error(error, "FB_JSON_VALUE failed");
                }
            }
        }
    } else if (strcasecmp(name, "FB_CONVERT_TZ") == 0) {
        if (expr->arg_count != 3) {
            firebase_expr_set_eval_error(error, "FB_CONVERT_TZ expects three arguments");
        } else if (firebase_value_is_null(&args[0]) || firebase_value_is_null(&args[1]) ||
                   firebase_value_is_null(&args[2])) {
            ok = firebase_value_set_null(out);
        } else {
            char* dt = firebase_value_as_text(&args[0]);
            char* from = firebase_value_as_text(&args[1]);
            char* to = firebase_value_as_text(&args[2]);
            char* converted = (dt && from && to) ? firebase_convert_tz(dt, from, to) : NULL;
            free(dt);
            free(from);
            free(to);
            if (!converted) {
                ok = firebase_value_set_null(out);
            } else {
                ok = firebase_value_take_data(out, FIREBASE_VAL_TEXT, converted, strlen(converted));
                if (!ok) {
                    free(converted);
                    firebase_expr_set_eval_error(error, "FB_CONVERT_TZ failed");
                }
            }
        }
    } else if (strcasecmp(name, "FB_TIME_ADD") == 0 || strcasecmp(name, "FB_SESSION_SECS") == 0) {
        firebase_expr_set_eval_error(error, "FB_TIME_ADD/FB_SESSION_SECS are not implemented yet");
    } else {
        firebase_expr_set_eval_error(error, "unknown function");
    }

    if (args) {
        for (size_t i = 0; i < expr->arg_count; i++) {
            firebase_value_free(&args[i]);
        }
        free(args);
    }
    return ok;
}
