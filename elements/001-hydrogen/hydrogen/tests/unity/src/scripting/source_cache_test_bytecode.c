/*
 * Unity Test File: source_cache_test_bytecode.c
 *
 * Phase 21 of the LUA_PLAN. Validates bytecode caching in the
 * SourceCache data structure:
 *
 *   - put_bytecode/get_bytecode round-trip
 *   - NULL safety on all new functions
 *   - replace-on-put_bytecode
 *   - bytecode and source can coexist
 *   - bytecode cleared when source replaced
 *   - NULL/zero len bytecode handled gracefully
 *   - get_bytecode returns NULL when no bytecode cached
 */

 // Project header + Unity
 #include <src/hydrogen.h>
 #include <unity.h>

 // Standard includes
 #include <string.h>

 // Module under test
 #include <src/scripting/source_cache.h>

 // Forward declarations (required for -Wmissing-prototypes)
 void test_bytecode_create_destroy(void);
 void test_bytecode_put_get_round_trip(void);
 void test_bytecode_null_safety(void);
 void test_bytecode_replace_on_put(void);
 void test_bytecode_clear_on_zero_len(void);
 void test_bytecode_get_missing_returns_null(void);
 void test_bytecode_coexists_with_source(void);
 void test_bytecode_cleared_when_source_replaced(void);
 void test_bytecode_zero_len_clears(void);

 void setUp(void) {
 }

 void tearDown(void) {
 }

 void test_bytecode_create_destroy(void) {
     SourceCache* cache = source_cache_create();
     TEST_ASSERT_NOT_NULL(cache);
     source_cache_destroy(cache);

     source_cache_destroy(NULL);
 }

 void test_bytecode_put_get_round_trip(void) {
     SourceCache* cache = source_cache_create();

     uint8_t bytecode[] = {0x1B, 0x4C, 0x75, 0x61, 0x50, 0x00, 0x01, 0x04};
     TEST_ASSERT_TRUE(source_cache_put_bytecode(cache, "G", "S", bytecode, sizeof(bytecode)));

     size_t len = 0;
     const void* result = source_cache_get_bytecode(cache, "G", "S", &len);
     TEST_ASSERT_NOT_NULL(result);
     TEST_ASSERT_EQUAL(sizeof(bytecode), len);
     TEST_ASSERT_EQUAL_UINT8_ARRAY(bytecode, result, sizeof(bytecode));

     source_cache_destroy(cache);
 }

 void test_bytecode_null_safety(void) {
     SourceCache* cache = source_cache_create();
     const uint8_t bytecode[] = {0x1B, 0x4C, 0x75, 0x61};

     TEST_ASSERT_FALSE(source_cache_put_bytecode(NULL, "G", "S", bytecode, 4));
     TEST_ASSERT_FALSE(source_cache_put_bytecode(cache, NULL, "S", bytecode, 4));
     TEST_ASSERT_FALSE(source_cache_put_bytecode(cache, "G", NULL, bytecode, 4));
     TEST_ASSERT_FALSE(source_cache_put_bytecode(cache, "G", "S", NULL, 4));

     size_t len = 0;
     TEST_ASSERT_NULL(source_cache_get_bytecode(NULL, "G", "S", &len));
     TEST_ASSERT_EQUAL(0, len);
     TEST_ASSERT_NULL(source_cache_get_bytecode(cache, NULL, "S", &len));
     TEST_ASSERT_NULL(source_cache_get_bytecode(cache, "G", NULL, &len));

     source_cache_destroy(cache);
 }

 void test_bytecode_replace_on_put(void) {
     SourceCache* cache = source_cache_create();

     uint8_t bytecode1[] = {0x01, 0x02, 0x03};
     TEST_ASSERT_TRUE(source_cache_put_bytecode(cache, "G", "S", bytecode1, sizeof(bytecode1)));

     uint8_t bytecode2[] = {0xAA, 0xBB, 0xCC, 0xDD};
     TEST_ASSERT_TRUE(source_cache_put_bytecode(cache, "G", "S", bytecode2, sizeof(bytecode2)));

     size_t len = 0;
     const void* result = source_cache_get_bytecode(cache, "G", "S", &len);
     TEST_ASSERT_NOT_NULL(result);
     TEST_ASSERT_EQUAL(sizeof(bytecode2), len);
     TEST_ASSERT_EQUAL_UINT8_ARRAY(bytecode2, result, sizeof(bytecode2));

     source_cache_destroy(cache);
 }

 void test_bytecode_get_missing_returns_null(void) {
     SourceCache* cache = source_cache_create();

     size_t len = 0;
     TEST_ASSERT_NULL(source_cache_get_bytecode(cache, "G", "MISSING", &len));
     TEST_ASSERT_EQUAL(0, len);

     source_cache_destroy(cache);
 }

 void test_bytecode_coexists_with_source(void) {
     SourceCache* cache = source_cache_create();

     TEST_ASSERT_TRUE(source_cache_put(cache, "G", "S", "local x = 1"));
     TEST_ASSERT_TRUE(source_cache_put_bytecode(cache, "G", "S", (uint8_t*)"bytecode", 8));

     const char* source = source_cache_get(cache, "G", "S");
     TEST_ASSERT_NOT_NULL(source);
     TEST_ASSERT_EQUAL_STRING("local x = 1", source);

     size_t len = 0;
     const void* bytecode = source_cache_get_bytecode(cache, "G", "S", &len);
     TEST_ASSERT_NOT_NULL(bytecode);
     TEST_ASSERT_EQUAL(8, len);

     source_cache_destroy(cache);
 }

 void test_bytecode_cleared_when_source_replaced(void) {
     SourceCache* cache = source_cache_create();

     TEST_ASSERT_TRUE(source_cache_put(cache, "G", "S", "v1"));
     TEST_ASSERT_TRUE(source_cache_put_bytecode(cache, "G", "S", (uint8_t*)"old", 3));

     TEST_ASSERT_TRUE(source_cache_put(cache, "G", "S", "v2"));

     size_t len = 0;
     const void* bytecode = source_cache_get_bytecode(cache, "G", "S", &len);
     TEST_ASSERT_NULL(bytecode);
     TEST_ASSERT_EQUAL(0, len);

     source_cache_destroy(cache);
 }

 void test_bytecode_clear_on_zero_len(void) {
     SourceCache* cache = source_cache_create();

     TEST_ASSERT_TRUE(source_cache_put(cache, "G", "S", "source"));
     TEST_ASSERT_TRUE(source_cache_put_bytecode(cache, "G", "S", (uint8_t*)"bytecode", 8));

     size_t len = 0;
     const void* bytecode = source_cache_get_bytecode(cache, "G", "S", &len);
     TEST_ASSERT_NOT_NULL(bytecode);

     TEST_ASSERT_TRUE(source_cache_put_bytecode(cache, "G", "S", NULL, 0));

     bytecode = source_cache_get_bytecode(cache, "G", "S", &len);
     TEST_ASSERT_NULL(bytecode);
     TEST_ASSERT_EQUAL(0, len);

     source_cache_destroy(cache);
 }

 int main(void) {
     UNITY_BEGIN();

     RUN_TEST(test_bytecode_create_destroy);
     RUN_TEST(test_bytecode_put_get_round_trip);
     RUN_TEST(test_bytecode_null_safety);
     RUN_TEST(test_bytecode_replace_on_put);
     RUN_TEST(test_bytecode_get_missing_returns_null);
     RUN_TEST(test_bytecode_coexists_with_source);
     RUN_TEST(test_bytecode_cleared_when_source_replaced);
     RUN_TEST(test_bytecode_clear_on_zero_len);

     return UNITY_END();
 }