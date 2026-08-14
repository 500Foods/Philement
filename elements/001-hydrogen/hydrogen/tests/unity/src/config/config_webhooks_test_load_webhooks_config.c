/*
 * Unity Test File: load_webhooks_config
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_webhooks.h>

void test_missing_section_keeps_disabled(void);
void test_loads_hooks_array(void);
void test_find_hook_case_insensitive(void);
void test_skips_hook_without_name_or_script(void);

static AppConfig cfg;

void setUp(void) {
    memset(&cfg, 0, sizeof(cfg));
}

void tearDown(void) {
    cleanup_webhooks_config(&cfg.webhooks);
}

void test_missing_section_keeps_disabled(void) {
    json_t *root = json_loads("{\"Server\":{}}", 0, NULL);
    TEST_ASSERT_TRUE(load_webhooks_config(root, &cfg));
    TEST_ASSERT_FALSE(cfg.webhooks.Enabled);
    TEST_ASSERT_EQUAL_INT(0, cfg.webhooks.HookCount);
    json_decref(root);
}

void test_loads_hooks_array(void) {
    json_t *root = json_loads(
        "{\"Webhooks\":{\"Enabled\":true,\"Hooks\":[{"
        "\"Name\":\"stripe\","
        "\"SecretEnv\":\"STRIPE_WEBHOOK_SECRET\","
        "\"SignatureHeader\":\"Stripe-Signature\","
        "\"Hmac\":\"sha256-timestamp\","
        "\"Script\":\"Stripe.Webhook\""
        "}]}}",
        0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(load_webhooks_config(root, &cfg));
    TEST_ASSERT_TRUE(cfg.webhooks.Enabled);
    TEST_ASSERT_EQUAL_INT(1, cfg.webhooks.HookCount);
    TEST_ASSERT_EQUAL_STRING("stripe", cfg.webhooks.Hooks[0].Name);
    TEST_ASSERT_EQUAL_STRING("STRIPE_WEBHOOK_SECRET",
                             cfg.webhooks.Hooks[0].SecretEnv);
    TEST_ASSERT_EQUAL_STRING("Stripe-Signature",
                             cfg.webhooks.Hooks[0].SignatureHeader);
    TEST_ASSERT_EQUAL_STRING("sha256-timestamp", cfg.webhooks.Hooks[0].Hmac);
    TEST_ASSERT_EQUAL_STRING("Stripe.Webhook", cfg.webhooks.Hooks[0].Script);
    json_decref(root);
}

void test_find_hook_case_insensitive(void) {
    json_t *root = json_loads(
        "{\"Webhooks\":{\"Enabled\":true,\"Hooks\":[{"
        "\"Name\":\"stripe\",\"Script\":\"Stripe.Webhook\"}]}}",
        0, NULL);
    TEST_ASSERT_TRUE(load_webhooks_config(root, &cfg));
    TEST_ASSERT_NOT_NULL(webhooks_find_hook(&cfg.webhooks, "STRIPE"));
    TEST_ASSERT_NULL(webhooks_find_hook(&cfg.webhooks, "github"));
    json_decref(root);
}

void test_skips_hook_without_name_or_script(void) {
    json_t *root = json_loads(
        "{\"Webhooks\":{\"Enabled\":true,\"Hooks\":["
        "{\"Name\":\"x\"},"
        "{\"Script\":\"Y.Z\"},"
        "{\"Name\":\"ok\",\"Script\":\"Ok.Script\"}"
        "]}}",
        0, NULL);
    TEST_ASSERT_TRUE(load_webhooks_config(root, &cfg));
    TEST_ASSERT_EQUAL_INT(1, cfg.webhooks.HookCount);
    TEST_ASSERT_EQUAL_STRING("ok", cfg.webhooks.Hooks[0].Name);
    json_decref(root);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_missing_section_keeps_disabled);
    RUN_TEST(test_loads_hooks_array);
    RUN_TEST(test_find_hook_case_insensitive);
    RUN_TEST(test_skips_hook_without_name_or_script);
    return UNITY_END();
}
