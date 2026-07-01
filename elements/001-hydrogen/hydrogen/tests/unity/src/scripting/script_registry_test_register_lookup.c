/*
 * Unity Test File: script_registry_test_register_lookup.c
 *
 * Phase 7 of the LUA_PLAN. Validates the in-memory script registry:
 *   - create returns non-NULL
 *   - destroy is safe with NULL and idempotent
 *   - register/lookup round-trip
 *   - register replaces an existing entry
 *   - lookup of an unknown name returns NULL
 *   - count reflects inserts and replaces
 *   - count on NULL is 0
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/scripting/script_registry.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_create_returns_non_null(void);
void test_destroy_null_is_safe(void);
void test_destroy_idempotent(void);
void test_register_then_lookup(void);
void test_register_replaces_existing(void);
void test_lookup_unknown_returns_null(void);
void test_count_reflects_inserts_and_replaces(void);
void test_count_on_null_is_zero(void);

void setUp(void) {
}

void tearDown(void) {
}

// create() returns a non-NULL registry
void test_create_returns_non_null(void) {
    ScriptRegistry* reg = script_registry_create();
    TEST_ASSERT_NOT_NULL(reg);
    script_registry_destroy(reg);
}

// destroy(NULL) is safe
void test_destroy_null_is_safe(void) {
    script_registry_destroy(NULL);
    TEST_ASSERT_TRUE(1);
}

// Calling destroy twice is safe when the caller NULLs the pointer
void test_destroy_idempotent(void) {
    ScriptRegistry* reg = script_registry_create();
    TEST_ASSERT_NOT_NULL(reg);
    script_registry_destroy(reg);
    reg = NULL;
    script_registry_destroy(reg);
    TEST_ASSERT_NULL(reg);
}

// Round-trip: register then lookup returns the same source
void test_register_then_lookup(void) {
    ScriptRegistry* reg = script_registry_create();
    TEST_ASSERT_NOT_NULL(reg);
    TEST_ASSERT_TRUE(script_registry_register(reg, "greet", "return 1"));
    const char* src = script_registry_lookup(reg, "greet");
    TEST_ASSERT_NOT_NULL(src);
    TEST_ASSERT_EQUAL_STRING("return 1", src);
    script_registry_destroy(reg);
}

// Re-registering the same name replaces the prior source
void test_register_replaces_existing(void) {
    ScriptRegistry* reg = script_registry_create();
    TEST_ASSERT_NOT_NULL(reg);
    TEST_ASSERT_TRUE(script_registry_register(reg, "x", "first"));
    TEST_ASSERT_TRUE(script_registry_register(reg, "x", "second"));
    TEST_ASSERT_EQUAL_size_t(1, script_registry_count(reg));
    TEST_ASSERT_EQUAL_STRING("second", script_registry_lookup(reg, "x"));
    script_registry_destroy(reg);
}

// Looking up a name that was never registered returns NULL
void test_lookup_unknown_returns_null(void) {
    ScriptRegistry* reg = script_registry_create();
    TEST_ASSERT_NOT_NULL(reg);
    TEST_ASSERT_NULL(script_registry_lookup(reg, "missing"));
    script_registry_destroy(reg);
}

// Count tracks inserts; replaces do not increment count
void test_count_reflects_inserts_and_replaces(void) {
    ScriptRegistry* reg = script_registry_create();
    TEST_ASSERT_NOT_NULL(reg);
    TEST_ASSERT_EQUAL_size_t(0, script_registry_count(reg));
    script_registry_register(reg, "a", "1");
    script_registry_register(reg, "b", "2");
    script_registry_register(reg, "c", "3");
    TEST_ASSERT_EQUAL_size_t(3, script_registry_count(reg));
    script_registry_register(reg, "a", "1-replaced");
    TEST_ASSERT_EQUAL_size_t(3, script_registry_count(reg));
    script_registry_destroy(reg);
}

// count(NULL) is 0
void test_count_on_null_is_zero(void) {
    TEST_ASSERT_EQUAL_size_t(0, script_registry_count(NULL));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_returns_non_null);
    RUN_TEST(test_destroy_null_is_safe);
    RUN_TEST(test_destroy_idempotent);
    RUN_TEST(test_register_then_lookup);
    RUN_TEST(test_register_replaces_existing);
    RUN_TEST(test_lookup_unknown_returns_null);
    RUN_TEST(test_count_reflects_inserts_and_replaces);
    RUN_TEST(test_count_on_null_is_zero);

    return UNITY_END();
}
