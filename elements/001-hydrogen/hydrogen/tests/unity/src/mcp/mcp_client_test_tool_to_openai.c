#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_client.h>

void test_tool_to_openai(void);
void test_tool_to_responses(void);
void test_tool_to_anthropic(void);
void test_tool_allowed(void);
void test_tools_filter(void);
json_t *sample_tool(void);

void setUp(void) {}
void tearDown(void) {}

json_t *sample_tool(void) {
    json_t *tool = json_object();
    json_t *schema = json_object();
    json_object_set_new(tool, "name", json_string("System.Info"));
    json_object_set_new(tool, "description", json_string("info"));
    json_object_set_new(schema, "type", json_string("object"));
    json_object_set_new(tool, "inputSchema", schema);
    return tool;
}

void test_tool_to_openai(void) {
    json_t *tool = sample_tool();
    json_t *out = mcp_client_tool_to_openai(tool);
    TEST_ASSERT_EQUAL_STRING("function", json_string_value(json_object_get(out, "type")));
    json_t *fn = json_object_get(out, "function");
    TEST_ASSERT_EQUAL_STRING("System.Info", json_string_value(json_object_get(fn, "name")));
    TEST_ASSERT_NOT_NULL(json_object_get(fn, "parameters"));
    json_decref(out);
    json_decref(tool);
}

void test_tool_to_responses(void) {
    json_t *tool = sample_tool();
    json_t *out = mcp_client_tool_to_responses(tool);
    TEST_ASSERT_EQUAL_STRING("function", json_string_value(json_object_get(out, "type")));
    TEST_ASSERT_EQUAL_STRING("System.Info", json_string_value(json_object_get(out, "name")));
    TEST_ASSERT_NOT_NULL(json_object_get(out, "parameters"));
    json_decref(out);
    json_decref(tool);
}

void test_tool_to_anthropic(void) {
    json_t *tool = sample_tool();
    json_t *out = mcp_client_tool_to_anthropic(tool);
    TEST_ASSERT_EQUAL_STRING("System.Info", json_string_value(json_object_get(out, "name")));
    TEST_ASSERT_NOT_NULL(json_object_get(out, "input_schema"));
    json_decref(out);
    json_decref(tool);
}

void test_tool_allowed(void) {
    char *allowed[] = { (char *)"System.Info", (char *)"Mcp.Echo" };
    TEST_ASSERT_TRUE(mcp_client_tool_allowed("System.Info", allowed, 2));
    TEST_ASSERT_FALSE(mcp_client_tool_allowed("H.query", allowed, 2));
    TEST_ASSERT_FALSE(mcp_client_tool_allowed("System.Info", allowed, 0));
    TEST_ASSERT_FALSE(mcp_client_tool_allowed(NULL, allowed, 2));
}

void test_tools_filter(void) {
    json_t *tools = json_array();
    json_array_append_new(tools, sample_tool());
    json_t *other = json_object();
    json_object_set_new(other, "name", json_string("Secret.Tool"));
    json_array_append_new(tools, other);
    char *allowed[] = { (char *)"System.Info" };
    json_t *filtered = mcp_client_tools_filter(tools, allowed, 1);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(filtered));
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(json_array_get(filtered, 0), "name")));
    json_decref(filtered);
    json_decref(tools);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tool_to_openai);
    RUN_TEST(test_tool_to_responses);
    RUN_TEST(test_tool_to_anthropic);
    RUN_TEST(test_tool_allowed);
    RUN_TEST(test_tools_filter);
    return UNITY_END();
}
