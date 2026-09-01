/*
 * Unity Test: mdns_server_threads_test_run_probe.c
 * Tests mdns_server_run_probe function for error conditions and edge cases
 *
 * CHANGELOG
 * 1.0.0 - 2026-09-01 - Initial creation
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

int mdns_server_run_probe(mdns_server_t *server);

void test_mdns_server_run_probe_null_server(void);
void test_mdns_server_run_probe_tiebreak_lose(void);
void test_mdns_server_run_probe_all_claimed(void);

void setUp(void) {
    mdns_server_system_shutdown = 0;
}

void tearDown(void) {
    mdns_server_system_shutdown = 0;
}

void test_mdns_server_run_probe_null_server(void) {
    int rc = mdns_server_run_probe(NULL);
    TEST_ASSERT_EQUAL_INT(-1, rc);
}

void test_mdns_server_run_probe_tiebreak_lose(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.probe_tiebreak_lose = 1;
    server.hostname_claimed = 1;
    server.hostname_attempts = 1;
    server.num_services = 0;
    server.services = NULL;

    int rc = mdns_server_run_probe(&server);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(0, server.probe_tiebreak_lose);
}

void test_mdns_server_run_probe_all_claimed(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname_claimed = 1;
    server.hostname_attempts = 1;
    server.num_services = 0;
    server.services = NULL;

    int rc = mdns_server_run_probe(&server);
    TEST_ASSERT_EQUAL_INT(0, rc);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_run_probe_null_server);
    RUN_TEST(test_mdns_server_run_probe_tiebreak_lose);
    RUN_TEST(test_mdns_server_run_probe_all_claimed);

    return UNITY_END();
}
