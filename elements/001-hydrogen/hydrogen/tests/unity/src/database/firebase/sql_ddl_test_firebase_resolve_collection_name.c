/*
 * Unity tests for firebase_resolve_collection_name().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/types.h>
#include <src/database/firebase/sql_ddl.h>

void test_firebase_resolve_collection_name_variants(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_resolve_collection_name_variants(void) {
    TEST_ASSERT_NULL(firebase_resolve_collection_name(NULL, NULL));
    TEST_ASSERT_NULL(firebase_resolve_collection_name(NULL, ""));

    char* name = firebase_resolve_collection_name(NULL, "queries");
    TEST_ASSERT_EQUAL_STRING("queries", name);
    free(name);

    FirebaseConnection fb = {0};
    fb.schema = (char*)"testfb";
    name = firebase_resolve_collection_name(&fb, "testfb_queries");
    TEST_ASSERT_EQUAL_STRING("testfb_queries", name);
    free(name);

    name = firebase_resolve_collection_name(&fb, "accounts");
    TEST_ASSERT_EQUAL_STRING("testfb_accounts", name);
    free(name);

    fb.schema = (char*)"";
    name = firebase_resolve_collection_name(&fb, "queries");
    TEST_ASSERT_EQUAL_STRING("queries", name);
    free(name);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_resolve_collection_name_variants);
    return UNITY_END();
}
