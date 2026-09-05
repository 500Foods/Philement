#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/proxy.h>
#include <string.h>

void test_write_callback_null_buffer(void);
void test_write_callback_null_contents(void);
void test_write_callback_normal_append(void);
void test_write_callback_resize_growth(void);
void test_write_callback_exceeds_max_size(void);
void test_write_callback_zero_size(void);

void setUp(void) {}
void tearDown(void) {}

void test_write_callback_null_buffer(void) {
    size_t result = chat_proxy_write_callback("data", 1, 4, NULL);
    TEST_ASSERT_EQUAL_size_t(0, result);
}

void test_write_callback_null_contents(void) {
    ChatResponseBuffer buffer;
    buffer.data = calloc(1, 64);
    buffer.size = 0;
    buffer.capacity = 64;

    size_t result = chat_proxy_write_callback(NULL, 1, 4, &buffer);
    TEST_ASSERT_EQUAL_size_t(0, result);

    free(buffer.data);
}

void test_write_callback_normal_append(void) {
    ChatResponseBuffer buffer;
    buffer.data = calloc(1, 64);
    buffer.size = 0;
    buffer.capacity = 64;

    const char *data = "hello world";
    size_t result = chat_proxy_write_callback(data, 1, strlen(data), &buffer);
    TEST_ASSERT_EQUAL_size_t(strlen(data), result);
    TEST_ASSERT_EQUAL_STRING("hello world", buffer.data);
    TEST_ASSERT_EQUAL_size_t(11, buffer.size);

    free(buffer.data);
}

void test_write_callback_resize_growth(void) {
    ChatResponseBuffer buffer;
    buffer.data = calloc(1, 8);
    buffer.size = 0;
    buffer.capacity = 8;

    const char *data = "this is a long string that exceeds the buffer capacity";
    size_t result = chat_proxy_write_callback(data, 1, strlen(data), &buffer);
    TEST_ASSERT_EQUAL_size_t(strlen(data), result);
    TEST_ASSERT_EQUAL_STRING(data, buffer.data);
    TEST_ASSERT_EQUAL_size_t(strlen(data), buffer.size);
    TEST_ASSERT(buffer.capacity >= strlen(data) + 1);

    free(buffer.data);
}

void test_write_callback_exceeds_max_size(void) {
    ChatResponseBuffer buffer;
    buffer.data = calloc(1, 1024 * 1024);
    buffer.size = 0;
    buffer.capacity = 1024 * 1024;

    size_t big_size = (size_t)8 * 1024 * 1024 + 1024;
    char *big_data = calloc(1, big_size);
    TEST_ASSERT_NOT_NULL(big_data);

    size_t result = chat_proxy_write_callback(big_data, 1, big_size, &buffer);
    TEST_ASSERT_EQUAL_size_t(0, result);

    free(big_data);
    free(buffer.data);
}

void test_write_callback_zero_size(void) {
    ChatResponseBuffer buffer;
    buffer.data = calloc(1, 64);
    buffer.size = 0;
    buffer.capacity = 64;

    size_t result = chat_proxy_write_callback("", 0, 0, &buffer);
    TEST_ASSERT_EQUAL_size_t(0, result);
    TEST_ASSERT_EQUAL_size_t(0, buffer.size);

    free(buffer.data);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_write_callback_null_buffer);
    RUN_TEST(test_write_callback_null_contents);
    RUN_TEST(test_write_callback_normal_append);
    RUN_TEST(test_write_callback_resize_growth);
    RUN_TEST(test_write_callback_exceeds_max_size);
    RUN_TEST(test_write_callback_zero_size);
    return UNITY_END();
}
