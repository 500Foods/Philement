/*
 * Firebase SQL expression parse and in-process eval (FB_* + literals).
 */

#ifndef DATABASE_ENGINE_FIREBASE_SQL_EXPR_H
#define DATABASE_ENGINE_FIREBASE_SQL_EXPR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    FIREBASE_EXPR_NULL = 0,
    FIREBASE_EXPR_INTEGER,
    FIREBASE_EXPR_NUMBER,
    FIREBASE_EXPR_STRING,
    FIREBASE_EXPR_IDENT,
    FIREBASE_EXPR_CALL,
    FIREBASE_EXPR_ADD
} FirebaseExprKind;

typedef struct FirebaseExpr FirebaseExpr;

struct FirebaseExpr {
    FirebaseExprKind kind;
    char* text;
    FirebaseExpr** args;
    size_t arg_count;
};

typedef enum {
    FIREBASE_VAL_NULL = 0,
    FIREBASE_VAL_INT,
    FIREBASE_VAL_DOUBLE,
    FIREBASE_VAL_TEXT,
    FIREBASE_VAL_BYTES
} FirebaseValKind;

typedef struct FirebaseValue {
    FirebaseValKind kind;
    long long i;
    double d;
    char* data;
    size_t length;
} FirebaseValue;

typedef bool (*FirebaseExprLookup)(const char* name, FirebaseValue* out, void* userdata);

FirebaseExpr* firebase_expr_alloc(FirebaseExprKind kind);
void firebase_expr_free(FirebaseExpr* expr);
bool firebase_expr_add_arg(FirebaseExpr* expr, FirebaseExpr* arg);
FirebaseExpr* firebase_expr_parse(const char** cursor, char** error);
FirebaseExpr* firebase_expr_parse_primary(const char** cursor, char** error);

void firebase_value_init(FirebaseValue* value);
void firebase_value_free(FirebaseValue* value);
bool firebase_value_set_null(FirebaseValue* value);
bool firebase_value_set_int(FirebaseValue* value, long long number);
bool firebase_value_set_double(FirebaseValue* value, double number);
bool firebase_value_copy_text(FirebaseValue* value, const char* text);
bool firebase_value_take_data(FirebaseValue* value, FirebaseValKind kind, char* data, size_t length);
bool firebase_value_copy(FirebaseValue* dest, const FirebaseValue* src);
char* firebase_value_as_text(const FirebaseValue* value);
bool firebase_value_as_int(const FirebaseValue* value, long long* out);
bool firebase_value_is_null(const FirebaseValue* value);
size_t firebase_value_byte_size(const FirebaseValue* value);
bool firebase_value_exceeds_max(const FirebaseValue* value);
bool firebase_value_equals(const FirebaseValue* left, const FirebaseValue* right);

bool firebase_expr_eval(const FirebaseExpr* expr, FirebaseExprLookup lookup, void* userdata,
                        FirebaseValue* out, char** error);
bool firebase_expr_eval_call(const FirebaseExpr* expr, FirebaseExprLookup lookup, void* userdata,
                             FirebaseValue* out, char** error);
bool firebase_expr_set_eval_error(char** error, const char* message);

#endif /* DATABASE_ENGINE_FIREBASE_SQL_EXPR_H */
