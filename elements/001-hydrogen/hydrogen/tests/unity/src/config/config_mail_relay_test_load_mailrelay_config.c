/*
 * Unity Test File: Mail Relay Configuration Tests
 * This file contains unit tests for the load_mailrelay_config function
 * from src/config/config_mail_relay.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_mail_relay.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_mailrelay_config(json_t* root, AppConfig* config);
void cleanup_mailrelay_config(MailRelayConfig* config);
void dump_mailrelay_config(const MailRelayConfig* config);

// Forward declarations for helper functions we'll need
bool initialize_config_defaults(AppConfig* config);

// Forward declarations for test functions
void test_load_mailrelay_config_null_root(void);
void test_load_mailrelay_config_empty_json(void);
void test_load_mailrelay_config_basic_fields(void);
void test_load_mailrelay_config_queue_settings(void);
void test_load_mailrelay_config_events_rules(void);
void test_load_mailrelay_config_admin_recipients(void);
void test_load_mailrelay_config_database_default(void);
void test_cleanup_mailrelay_config_null_pointer(void);
void test_cleanup_mailrelay_config_empty_config(void);
void test_cleanup_mailrelay_config_with_data(void);
void test_dump_mailrelay_config_null_pointer(void);
void test_dump_mailrelay_config_basic(void);
void test_dump_mailrelay_config_redacts_password(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_mailrelay_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_mailrelay_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.mail_relay.Enabled);
    TEST_ASSERT_FALSE(config.mail_relay.OutboundEnabled);
    TEST_ASSERT_FALSE(config.mail_relay.InboundEnabled);
    TEST_ASSERT_EQUAL(25, config.mail_relay.ListenPort);
    TEST_ASSERT_EQUAL(2, config.mail_relay.Workers);
    TEST_ASSERT_EQUAL(0, config.mail_relay.OutboundServerCount);
    TEST_ASSERT_EQUAL(0, config.mail_relay.AdminRecipientCount);
    TEST_ASSERT_FALSE(config.mail_relay.Events.Enabled);
    TEST_ASSERT_EQUAL(0, config.mail_relay.Events.RuleCount);
    TEST_ASSERT_EQUAL(300, config.mail_relay.Queue.StaleTimeoutSeconds);

    cleanup_mailrelay_config(&config.mail_relay);
}

void test_load_mailrelay_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_mailrelay_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.mail_relay.Enabled);
    TEST_ASSERT_FALSE(config.mail_relay.OutboundEnabled);
    TEST_ASSERT_FALSE(config.mail_relay.InboundEnabled);
    TEST_ASSERT_EQUAL(25, config.mail_relay.ListenPort);
    TEST_ASSERT_EQUAL(2, config.mail_relay.Workers);
    TEST_ASSERT_EQUAL(0, config.mail_relay.OutboundServerCount);
    TEST_ASSERT_EQUAL(0, config.mail_relay.AdminRecipientCount);
    TEST_ASSERT_EQUAL(0, config.mail_relay.Events.RuleCount);
    TEST_ASSERT_EQUAL(300, config.mail_relay.Queue.StaleTimeoutSeconds);

    json_decref(root);
    cleanup_mailrelay_config(&config.mail_relay);
}

// ===== BASIC FIELD TESTS =====

void test_load_mailrelay_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* mail_relay_section = json_object();

    json_object_set(mail_relay_section, "Enabled", json_false());
    json_object_set(mail_relay_section, "OutboundEnabled", json_true());
    json_object_set(mail_relay_section, "InboundEnabled", json_false());
    json_object_set(mail_relay_section, "ListenPort", json_integer(2525));
    json_object_set(mail_relay_section, "Workers", json_integer(4));

    json_object_set(root, "MailRelay", mail_relay_section);

    bool result = load_mailrelay_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.mail_relay.Enabled);
    TEST_ASSERT_TRUE(config.mail_relay.OutboundEnabled);
    TEST_ASSERT_FALSE(config.mail_relay.InboundEnabled);
    TEST_ASSERT_EQUAL(2525, config.mail_relay.ListenPort);
    TEST_ASSERT_EQUAL(4, config.mail_relay.Workers);
    TEST_ASSERT_EQUAL(0, config.mail_relay.OutboundServerCount);

    json_decref(root);
    cleanup_mailrelay_config(&config.mail_relay);
}

void test_load_mailrelay_config_queue_settings(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* mail_relay_section = json_object();
    json_t* queue_section = json_object();

    // Set up queue configuration
    json_object_set(queue_section, "MaxInMemory", json_integer(500));
    json_object_set(queue_section, "Persist", json_true());
    json_object_set(queue_section, "RetryAttempts", json_integer(5));
    json_object_set(queue_section, "RetryDelaySeconds", json_integer(600));
    json_object_set(queue_section, "InitialDelaySeconds", json_integer(15));
    json_object_set(queue_section, "MaxDelaySeconds", json_integer(1800));
    json_object_set(queue_section, "DebounceSeconds", json_integer(10));
    json_object_set(queue_section, "StaleTimeoutSeconds", json_integer(600));

    json_object_set(mail_relay_section, "Queue", queue_section);
    json_object_set(root, "MailRelay", mail_relay_section);

    bool result = load_mailrelay_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(500, config.mail_relay.Queue.MaxInMemory);
    TEST_ASSERT_TRUE(config.mail_relay.Queue.Persist);
    TEST_ASSERT_EQUAL(5, config.mail_relay.Queue.RetryAttempts);
    TEST_ASSERT_EQUAL(600, config.mail_relay.Queue.RetryDelaySeconds);
    TEST_ASSERT_EQUAL(15, config.mail_relay.Queue.InitialDelaySeconds);
    TEST_ASSERT_EQUAL(1800, config.mail_relay.Queue.MaxDelaySeconds);
    TEST_ASSERT_EQUAL(10, config.mail_relay.Queue.DebounceSeconds);
    TEST_ASSERT_EQUAL(600, config.mail_relay.Queue.StaleTimeoutSeconds);

    json_decref(root);
    cleanup_mailrelay_config(&config.mail_relay);
}

void test_load_mailrelay_config_events_rules(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* mail_relay_section = json_object();
    json_t* events_section = json_object();
    json_t* rules = json_object();

    json_object_set(events_section, "Enabled", json_true());
    json_object_set(rules, "server_started", json_string("mail_events.handle_start"));
    json_object_set(rules, "auth.failed", json_string("mail_events.handle_auth_fail"));

    json_object_set(events_section, "Rules", rules);
    json_object_set(mail_relay_section, "Events", events_section);
    json_object_set(root, "MailRelay", mail_relay_section);

    bool result = load_mailrelay_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.mail_relay.Events.Enabled);
    TEST_ASSERT_EQUAL(2, config.mail_relay.Events.RuleCount);
    TEST_ASSERT_EQUAL_STRING("server_started", config.mail_relay.Events.Rules[0].event_key);
    TEST_ASSERT_EQUAL_STRING("mail_events.handle_start", config.mail_relay.Events.Rules[0].script_name);
    TEST_ASSERT_EQUAL_STRING("auth.failed", config.mail_relay.Events.Rules[1].event_key);
    TEST_ASSERT_EQUAL_STRING("mail_events.handle_auth_fail", config.mail_relay.Events.Rules[1].script_name);

    json_decref(root);
    cleanup_mailrelay_config(&config.mail_relay);
}

void test_load_mailrelay_config_admin_recipients(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* mail_relay_section = json_object();
    json_t* recipients = json_array();

    json_array_append_new(recipients, json_string("admin@example.com"));
    json_array_append_new(recipients, json_string("ops@example.com"));

    json_object_set(mail_relay_section, "AdminRecipients", recipients);
    json_object_set(root, "MailRelay", mail_relay_section);

    bool result = load_mailrelay_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, config.mail_relay.AdminRecipientCount);
    TEST_ASSERT_EQUAL_STRING("admin@example.com", config.mail_relay.AdminRecipients[0]);
    TEST_ASSERT_EQUAL_STRING("ops@example.com", config.mail_relay.AdminRecipients[1]);

    json_decref(root);
    cleanup_mailrelay_config(&config.mail_relay);
}

void test_load_mailrelay_config_database_default(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Simulate a single database configuration
    config.databases.connection_count = 1;
    config.databases.connections[0].connection_name = strdup("acuranzo");
    config.databases.connections[0].enabled = true;

    json_t* root = json_object();
    // No Database field set in config

    bool result = load_mailrelay_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("acuranzo", config.mail_relay.Database);

    json_decref(root);
    free(config.databases.connections[0].connection_name);
    cleanup_mailrelay_config(&config.mail_relay);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_mailrelay_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_mailrelay_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_mailrelay_config_empty_config(void) {
    MailRelayConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_mailrelay_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_EQUAL(0, config.OutboundServerCount);
    TEST_ASSERT_EQUAL(0, config.AdminRecipientCount);
    TEST_ASSERT_EQUAL(0, config.Events.RuleCount);
}

void test_cleanup_mailrelay_config_with_data(void) {
    MailRelayConfig config = {0};

    // Initialize with some test data
    config.Enabled = true;
    config.OutboundEnabled = true;
    config.InboundEnabled = false;
    config.ListenPort = 587;
    config.Workers = 2;
    config.OutboundServerCount = 1;
    config.Database = strdup("acuranzo");
    config.DefaultFrom = strdup("noreply@example.com");
    config.DefaultReplyTo = strdup("support@example.com");
    config.AdminRecipientCount = 2;
    config.AdminRecipients[0] = strdup("admin@example.com");
    config.AdminRecipients[1] = strdup("ops@example.com");

    config.Servers[0].Host = strdup("smtp.example.com");
    config.Servers[0].Port = strdup("587");
    config.Servers[0].Username = strdup("test@example.com");
    config.Servers[0].Password = strdup("test-password");
    config.Servers[0].UseTLS = true;
    config.Servers[0].TLSMode = MAIL_TLS_MODE_STARTTLS;
    config.Servers[0].CAPath = strdup("/etc/ssl/certs/ca.pem");
    config.Servers[0].AuthMode = MAIL_AUTH_MODE_PLAIN;
    config.Servers[0].TimeoutSeconds = 30;

    config.Events.Enabled = true;
    config.Events.RuleCount = 1;
    config.Events.Rules[0].event_key = strdup("test_event");
    config.Events.Rules[0].script_name = strdup("test_handler");

    // Cleanup should free all allocated memory
    cleanup_mailrelay_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_EQUAL(0, config.OutboundServerCount);
    TEST_ASSERT_NULL(config.Servers[0].Host);
    TEST_ASSERT_NULL(config.Servers[0].Port);
    TEST_ASSERT_NULL(config.Servers[0].Username);
    TEST_ASSERT_NULL(config.Servers[0].Password);
    TEST_ASSERT_NULL(config.Servers[0].CAPath);
    TEST_ASSERT_EQUAL(0, config.AdminRecipientCount);
    TEST_ASSERT_NULL(config.AdminRecipients[0]);
    TEST_ASSERT_NULL(config.AdminRecipients[1]);
    TEST_ASSERT_NULL(config.Database);
    TEST_ASSERT_NULL(config.DefaultFrom);
    TEST_ASSERT_NULL(config.DefaultReplyTo);
    TEST_ASSERT_EQUAL(0, config.Events.RuleCount);
    TEST_ASSERT_NULL(config.Events.Rules[0].event_key);
    TEST_ASSERT_NULL(config.Events.Rules[0].script_name);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_mailrelay_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_mailrelay_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_mailrelay_config_basic(void) {
    MailRelayConfig config = {0};

    // Initialize with test data
    config.Enabled = true;
    config.ListenPort = 587;
    config.Workers = 2;
    config.Queue.MaxQueueSize = 1000;
    config.Queue.MaxInMemory = 1000;
    config.Queue.Persist = false;
    config.Queue.RetryAttempts = 3;
    config.Queue.RetryDelaySeconds = 300;
    config.Queue.InitialDelaySeconds = 10;
    config.Queue.MaxDelaySeconds = 3600;
    config.Queue.DebounceSeconds = 5;
    config.Queue.StaleTimeoutSeconds = 300;
    config.Templates.ReloadIntervalSeconds = 60;
    config.OutboundServerCount = 1;
    config.Servers[0].Host = strdup("smtp.example.com");
    config.Servers[0].Port = strdup("587");
    config.Servers[0].Username = strdup("test@example.com");
    config.Servers[0].Password = strdup("test-password");
    config.Servers[0].UseTLS = true;

    // Dump should not crash and handle the data properly
    dump_mailrelay_config(&config);

    // Clean up
    cleanup_mailrelay_config(&config);
}

void test_dump_mailrelay_config_redacts_password(void) {
    MailRelayConfig config = {0};

    config.Enabled = true;
    config.DefaultFrom = strdup("noreply@example.com");
    config.DefaultReplyTo = strdup("support@example.com");
    config.AdminRecipientCount = 1;
    config.AdminRecipients[0] = strdup("admin@example.com");
    config.OutboundServerCount = 1;
    config.Servers[0].Host = strdup("smtp.example.com");
    config.Servers[0].Port = strdup("587");
    config.Servers[0].Username = strdup("test@example.com");
    config.Servers[0].Password = strdup("SECRET_PASSWORD");
    config.Servers[0].CAPath = strdup("/etc/ssl/certs/ca.pem");
    config.Servers[0].TLSMode = MAIL_TLS_MODE_STARTTLS;
    config.Servers[0].AuthMode = MAIL_AUTH_MODE_PLAIN;
    config.Servers[0].TimeoutSeconds = 30;

    dump_mailrelay_config(&config);

    cleanup_mailrelay_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_mailrelay_config_null_root);
    RUN_TEST(test_load_mailrelay_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_mailrelay_config_basic_fields);
    RUN_TEST(test_load_mailrelay_config_queue_settings);
    RUN_TEST(test_load_mailrelay_config_events_rules);
    RUN_TEST(test_load_mailrelay_config_admin_recipients);
    RUN_TEST(test_load_mailrelay_config_database_default);

    // Cleanup function tests
    RUN_TEST(test_cleanup_mailrelay_config_null_pointer);
    RUN_TEST(test_cleanup_mailrelay_config_empty_config);
    RUN_TEST(test_cleanup_mailrelay_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_mailrelay_config_null_pointer);
    RUN_TEST(test_dump_mailrelay_config_basic);
    RUN_TEST(test_dump_mailrelay_config_redacts_password);

    return UNITY_END();
}
