/*
 * Unity Test File: update_queue_limits_from_config Function Tests
 * This file contains comprehensive unit tests for the update_queue_limits_from_config() function
 * from src/utils/utils.c
 *
 * Coverage Goals:
 * - Test NULL config handling (early return)
 * - Test that all 8 queue memory structures have their limits updated
 * - Test that max_blocks, block_limit, and early_init are set correctly
 * - Test with different max_queue_blocks values
 * - Test calling multiple times with different configs
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include the header for the function being tested and queue globals
#include <src/utils/utils_queue.h>

// Forward declaration for the function being tested
void update_queue_limits_from_config(const AppConfig *config);

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Reset all 8 queue memory structures to early-init state before each test
    // so tests are independent of constructor execution order and prior tests
    init_queue_memory(&log_queue_memory, NULL);
    init_queue_memory(&webserver_queue_memory, NULL);
    init_queue_memory(&websocket_queue_memory, NULL);
    init_queue_memory(&mdns_server_queue_memory, NULL);
    init_queue_memory(&print_queue_memory, NULL);
    init_queue_memory(&database_queue_memory, NULL);
    init_queue_memory(&mail_relay_queue_memory, NULL);
    init_queue_memory(&notify_queue_memory, NULL);
}

void tearDown(void) {
    // Leave queues in a clean state after each test
    init_queue_memory(&log_queue_memory, NULL);
    init_queue_memory(&webserver_queue_memory, NULL);
    init_queue_memory(&websocket_queue_memory, NULL);
    init_queue_memory(&mdns_server_queue_memory, NULL);
    init_queue_memory(&print_queue_memory, NULL);
    init_queue_memory(&database_queue_memory, NULL);
    init_queue_memory(&mail_relay_queue_memory, NULL);
    init_queue_memory(&notify_queue_memory, NULL);
}

// Function prototypes for test functions
void test_update_queue_limits_from_config_null_config(void);
void test_update_queue_limits_from_config_all_queues_log(void);
void test_update_queue_limits_from_config_all_queues_webserver(void);
void test_update_queue_limits_from_config_all_queues_websocket(void);
void test_update_queue_limits_from_config_all_queues_mdns_server(void);
void test_update_queue_limits_from_config_all_queues_print(void);
void test_update_queue_limits_from_config_all_queues_database(void);
void test_update_queue_limits_from_config_all_queues_mail_relay(void);
void test_update_queue_limits_from_config_all_queues_notify(void);
void test_update_queue_limits_from_config_early_init_cleared(void);
void test_update_queue_limits_from_config_different_values(void);
void test_update_queue_limits_from_config_multiple_calls(void);
void test_update_queue_limits_from_config_preserves_block_data(void);

//=============================================================================
// NULL Config Test
//=============================================================================

void test_update_queue_limits_from_config_null_config(void) {
    // Test with NULL config - should return immediately without changes
    update_queue_limits_from_config(NULL);

    // All queues should remain in early-init state (unchanged)
    TEST_ASSERT_EQUAL(EARLY_MAX_QUEUE_BLOCKS, log_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(EARLY_MAX_QUEUE_BLOCKS, webserver_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(EARLY_MAX_QUEUE_BLOCKS, websocket_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(EARLY_MAX_QUEUE_BLOCKS, mdns_server_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(EARLY_MAX_QUEUE_BLOCKS, print_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(EARLY_MAX_QUEUE_BLOCKS, database_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(EARLY_MAX_QUEUE_BLOCKS, mail_relay_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(EARLY_MAX_QUEUE_BLOCKS, notify_queue_memory.limits.max_blocks);
}

//=============================================================================
// All Queues Updated Tests
//=============================================================================

void test_update_queue_limits_from_config_all_queues_log(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    config.resources.max_queue_blocks = 500;

    update_queue_limits_from_config(&config);

    TEST_ASSERT_EQUAL(500, log_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(500, log_queue_memory.limits.block_limit);
}

void test_update_queue_limits_from_config_all_queues_webserver(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    config.resources.max_queue_blocks = 500;

    update_queue_limits_from_config(&config);

    // Verify webserver queue is updated (not just log queue)
    TEST_ASSERT_EQUAL(500, webserver_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(500, webserver_queue_memory.limits.block_limit);
}

void test_update_queue_limits_from_config_all_queues_websocket(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    config.resources.max_queue_blocks = 500;

    update_queue_limits_from_config(&config);

    TEST_ASSERT_EQUAL(500, websocket_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(500, websocket_queue_memory.limits.block_limit);
}

void test_update_queue_limits_from_config_all_queues_mdns_server(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    config.resources.max_queue_blocks = 500;

    update_queue_limits_from_config(&config);

    TEST_ASSERT_EQUAL(500, mdns_server_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(500, mdns_server_queue_memory.limits.block_limit);
}

void test_update_queue_limits_from_config_all_queues_print(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    config.resources.max_queue_blocks = 500;

    update_queue_limits_from_config(&config);

    TEST_ASSERT_EQUAL(500, print_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(500, print_queue_memory.limits.block_limit);
}

void test_update_queue_limits_from_config_all_queues_database(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    config.resources.max_queue_blocks = 500;

    update_queue_limits_from_config(&config);

    TEST_ASSERT_EQUAL(500, database_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(500, database_queue_memory.limits.block_limit);
}

void test_update_queue_limits_from_config_all_queues_mail_relay(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    config.resources.max_queue_blocks = 500;

    update_queue_limits_from_config(&config);

    TEST_ASSERT_EQUAL(500, mail_relay_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(500, mail_relay_queue_memory.limits.block_limit);
}

void test_update_queue_limits_from_config_all_queues_notify(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    config.resources.max_queue_blocks = 500;

    update_queue_limits_from_config(&config);

    TEST_ASSERT_EQUAL(500, notify_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(500, notify_queue_memory.limits.block_limit);
}

//=============================================================================
// Early Init Flag Tests
//=============================================================================

void test_update_queue_limits_from_config_early_init_cleared(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    config.resources.max_queue_blocks = 500;

    // All queues start with early_init=1 (from init_queue_memory with NULL)
    TEST_ASSERT_EQUAL(1, log_queue_memory.limits.early_init);
    TEST_ASSERT_EQUAL(1, webserver_queue_memory.limits.early_init);
    TEST_ASSERT_EQUAL(1, notify_queue_memory.limits.early_init);

    update_queue_limits_from_config(&config);

    // After update, early_init should be cleared for all queues
    TEST_ASSERT_EQUAL(0, log_queue_memory.limits.early_init);
    TEST_ASSERT_EQUAL(0, webserver_queue_memory.limits.early_init);
    TEST_ASSERT_EQUAL(0, websocket_queue_memory.limits.early_init);
    TEST_ASSERT_EQUAL(0, mdns_server_queue_memory.limits.early_init);
    TEST_ASSERT_EQUAL(0, print_queue_memory.limits.early_init);
    TEST_ASSERT_EQUAL(0, database_queue_memory.limits.early_init);
    TEST_ASSERT_EQUAL(0, mail_relay_queue_memory.limits.early_init);
    TEST_ASSERT_EQUAL(0, notify_queue_memory.limits.early_init);
}

//=============================================================================
// Different Values Tests
//============================================================================

void test_update_queue_limits_from_config_different_values(void) {
    // Test with a different max_queue_blocks value
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    config.resources.max_queue_blocks = 2000;

    update_queue_limits_from_config(&config);

    TEST_ASSERT_EQUAL(2000, log_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(2000, webserver_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(2000, websocket_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(2000, mdns_server_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(2000, print_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(2000, database_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(2000, mail_relay_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(2000, notify_queue_memory.limits.max_blocks);

    // Verify block_limit equals max_queue_blocks as well
    TEST_ASSERT_EQUAL(2000, log_queue_memory.limits.block_limit);
    TEST_ASSERT_EQUAL(2000, database_queue_memory.limits.block_limit);
    TEST_ASSERT_EQUAL(2000, notify_queue_memory.limits.block_limit);
}

//=============================================================================
// Multiple Calls Tests
//=============================================================================

void test_update_queue_limits_from_config_multiple_calls(void) {
    AppConfig config1;
    memset(&config1, 0, sizeof(AppConfig));
    config1.resources.max_queue_blocks = 500;

    AppConfig config2;
    memset(&config2, 0, sizeof(AppConfig));
    config2.resources.max_queue_blocks = 1500;

    // First call with 500
    update_queue_limits_from_config(&config1);
    TEST_ASSERT_EQUAL(500, log_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(500, webserver_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(500, notify_queue_memory.limits.max_blocks);

    // Second call with 1500 - should update even though early_init is now 0
    update_queue_limits_from_config(&config2);
    TEST_ASSERT_EQUAL(1500, log_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(1500, webserver_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(1500, websocket_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(1500, mdns_server_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(1500, print_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(1500, database_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(1500, mail_relay_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(1500, notify_queue_memory.limits.max_blocks);

    // early_init should still be 0 (already cleared by first call)
    TEST_ASSERT_EQUAL(0, log_queue_memory.limits.early_init);
    TEST_ASSERT_EQUAL(0, notify_queue_memory.limits.early_init);
}

void test_update_queue_limits_from_config_preserves_block_data(void) {
    // Set some block data on queues before updating limits
    log_queue_memory.block_count = 5;
    log_queue_memory.total_allocation = 1024;
    log_queue_memory.entry_count = 3;
    log_queue_memory.block_sizes[0] = 256;
    log_queue_memory.block_sizes[1] = 512;

    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    config.resources.max_queue_blocks = 100;

    update_queue_limits_from_config(&config);

    // Block tracking data should be preserved (only limits change)
    TEST_ASSERT_EQUAL(5, log_queue_memory.block_count);
    TEST_ASSERT_EQUAL(1024, log_queue_memory.total_allocation);
    TEST_ASSERT_EQUAL(3, log_queue_memory.entry_count);
    TEST_ASSERT_EQUAL(256, log_queue_memory.block_sizes[0]);
    TEST_ASSERT_EQUAL(512, log_queue_memory.block_sizes[1]);

    // But limits should be updated
    TEST_ASSERT_EQUAL(100, log_queue_memory.limits.max_blocks);
    TEST_ASSERT_EQUAL(100, log_queue_memory.limits.block_limit);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // NULL config test
    RUN_TEST(test_update_queue_limits_from_config_null_config);

    // All queues updated tests
    RUN_TEST(test_update_queue_limits_from_config_all_queues_log);
    RUN_TEST(test_update_queue_limits_from_config_all_queues_webserver);
    RUN_TEST(test_update_queue_limits_from_config_all_queues_websocket);
    RUN_TEST(test_update_queue_limits_from_config_all_queues_mdns_server);
    RUN_TEST(test_update_queue_limits_from_config_all_queues_print);
    RUN_TEST(test_update_queue_limits_from_config_all_queues_database);
    RUN_TEST(test_update_queue_limits_from_config_all_queues_mail_relay);
    RUN_TEST(test_update_queue_limits_from_config_all_queues_notify);

    // Early init flag tests
    RUN_TEST(test_update_queue_limits_from_config_early_init_cleared);

    // Different values tests
    RUN_TEST(test_update_queue_limits_from_config_different_values);

    // Multiple calls tests
    RUN_TEST(test_update_queue_limits_from_config_multiple_calls);
    RUN_TEST(test_update_queue_limits_from_config_preserves_block_data);

    return UNITY_END();
}
