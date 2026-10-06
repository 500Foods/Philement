/*
 * Unity Test File: nats_subject_build
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/nats/nats_subject.h>

void test_nats_subject_build_valid(void);
void test_nats_subject_build_rejects(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_nats_subject_build_valid(void) {
    char *cache = nats_subject_build("philement", "cache.invalidate");
    char *state = nats_subject_build("philement", "instance.app_state");
    char *jobs = nats_subject_build("philement", "jobs.refresh-all");

    TEST_ASSERT_EQUAL_STRING("cluster.philement.cache.invalidate", cache);
    TEST_ASSERT_EQUAL_STRING("cluster.philement.instance.app_state", state);
    TEST_ASSERT_EQUAL_STRING("cluster.philement.jobs.refresh-all", jobs);

    free(cache);
    free(state);
    free(jobs);
}

void test_nats_subject_build_rejects(void) {
    TEST_ASSERT_NULL(nats_subject_build(NULL, "cache.invalidate"));
    TEST_ASSERT_NULL(nats_subject_build("", "cache.invalidate"));
    TEST_ASSERT_NULL(nats_subject_build("philement", NULL));
    TEST_ASSERT_NULL(nats_subject_build("philement", ""));
    TEST_ASSERT_NULL(nats_subject_build("phi.lement", "cache.invalidate"));
    TEST_ASSERT_NULL(nats_subject_build("philement", "cluster.cache"));
    TEST_ASSERT_NULL(nats_subject_build("philement", "cache invalidate"));
    TEST_ASSERT_NULL(nats_subject_build("philement", "cache.*"));
    TEST_ASSERT_NULL(nats_subject_build("philement", "cache.>"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_subject_build_valid);
    RUN_TEST(test_nats_subject_build_rejects);
    return UNITY_END();
}
