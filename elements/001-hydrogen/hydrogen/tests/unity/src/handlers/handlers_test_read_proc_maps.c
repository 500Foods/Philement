/*
 * Unity Test File: handlers_read_proc_maps
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/handlers/handlers.h>
#include <limits.h>
#include <string.h>

/* Complete private layout from handlers.c for Unity access */
struct CoreMapping {
    unsigned long start;
    unsigned long end;
    unsigned long offset;
    char perms[5];
    char path[PATH_MAX];
    int is_stack;
    int is_vdso;
};

void test_read_proc_maps_null_out_zero_max(void);
void test_read_proc_maps_parses_self(void);
void test_read_proc_maps_respects_max(void);

void setUp(void) {}
void tearDown(void) {}

void test_read_proc_maps_null_out_zero_max(void) {
    TEST_ASSERT_EQUAL_UINT(0, handlers_read_proc_maps(NULL, 0));
}

void test_read_proc_maps_parses_self(void) {
    CoreMapping maps[64];
    memset(maps, 0, sizeof(maps));
    size_t n = handlers_read_proc_maps(maps, 64);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_TRUE(maps[0].end > maps[0].start);
    TEST_ASSERT_TRUE(maps[0].perms[0] == 'r' || maps[0].perms[0] == '-');
}

void test_read_proc_maps_respects_max(void) {
    CoreMapping maps[2];
    memset(maps, 0, sizeof(maps));
    size_t n = handlers_read_proc_maps(maps, 2);
    TEST_ASSERT_EQUAL_UINT(2, n);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_read_proc_maps_null_out_zero_max);
    RUN_TEST(test_read_proc_maps_parses_self);
    RUN_TEST(test_read_proc_maps_respects_max);
    return UNITY_END();
}
