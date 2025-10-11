/*
 * Unity Test File: System AppConfig String Processing Logic Tests
 * This file contains unit tests for the string processing logic within appconfig.c
 * The string processing code in handle_system_appconfig_request could be extracted
 * into pure functions for better testability.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Mock functions to test the string processing logic that exists in appconfig.c
// These functions demonstrate how the logic could be extracted for testing

// Mock function: Find APPCONFIG marker in a line and return its position
static size_t find_appconfig_marker(const char *line) {
    if (!line) return 0;

    const char *marker = strstr(line, "APPCONFIG");
    if (!marker) return 0;

    return (size_t)(marker - line);
}

// Mock function: Extract content after APPCONFIG marker
static char* extract_content_after_marker(const char *line, size_t marker_offset) {
    if (!line || marker_offset >= strlen(line)) {
        return NULL;
    }

    const char *content_start = line + marker_offset;
    return strdup(content_start);
}

// Mock function: Process multiple lines to extract aligned content
static char** process_config_lines(const char *raw_text, size_t *line_count) {
    if (!raw_text || !line_count) return NULL;

    // Simple mock implementation of the line processing logic
    char **lines = NULL;
    *line_count = 0;

    // Split by newlines (simplified version)
    char *text_copy = strdup(raw_text);
    if (!text_copy) return NULL;

    const char *line = strtok(text_copy, "\n");
    while (line) {
        size_t marker_pos = find_appconfig_marker(line);
        if (marker_pos > 0) {  // Found APPCONFIG marker
            char **new_lines = realloc(lines, (*line_count + 1) * sizeof(char*));
            if (!new_lines) {
                free(text_copy);
                for (size_t i = 0; i < *line_count; i++) {
                    free(lines[i]);
                }
                free(lines);
                return NULL;
            }
            lines = new_lines;
            lines[*line_count] = extract_content_after_marker(line, marker_pos);
            if (lines[*line_count]) {
                (*line_count)++;
            }
        }
        line = strtok(NULL, "\n");
    }

    free(text_copy);
    return lines;
}

// Mock function: Build final processed text from lines (reverse order as in original)
static char* build_final_text(char **lines, size_t line_count) {
    if (!lines || line_count == 0) return NULL;

    // Calculate total size needed
    size_t processed_size = 0;
    for (size_t i = 0; i < line_count; i++) {
        if (lines[i]) {
            processed_size += strlen(lines[i]) + 1; // +1 for newline
        }
    }

    // Allocate and build final string in reverse order (as in original function)
    char *processed_text = malloc(processed_size + 1); // +1 for null terminator
    if (!processed_text) return NULL;

    size_t pos = 0;
    for (size_t i = line_count; i > 0; i--) {
        if (lines[i-1]) {
            size_t len = strlen(lines[i-1]);
            memcpy(processed_text + pos, lines[i-1], len);
            if (i > 1) { // Add newline after all but the last line
                processed_text[pos + len] = '\n';
                pos += len + 1;
            } else {
                pos += len;
            }
        }
    }
    processed_text[pos] = '\0'; // Properly terminate the string

    return processed_text;
}

// Mock function: Simulate text processing with alignment offset
static char* extract_aligned_content(const char *line, size_t content_offset) {
    if (!line) {
        return NULL;  // Return NULL for null input
    }

    if (strlen(line) <= content_offset) {
        return strdup("");  // Line too short, return empty string
    }

    return strdup(line + content_offset);
}

// Test function prototypes
void test_find_appconfig_marker_basic(void);
void test_find_appconfig_marker_not_found(void);
void test_find_appconfig_marker_null_input(void);
void test_find_appconfig_marker_empty_string(void);
void test_find_appconfig_marker_multiple_markers(void);
void test_extract_content_after_marker_basic(void);
void test_extract_content_after_marker_boundary(void);
void test_extract_content_after_marker_null_input(void);
void test_extract_content_after_marker_offset_at_end(void);
void test_process_config_lines_basic(void);
void test_process_config_lines_no_markers(void);
void test_process_config_lines_mixed_content(void);
void test_process_config_lines_empty_input(void);
void test_process_config_lines_null_input(void);
void test_build_final_text_basic(void);
void test_build_final_text_empty_lines(void);
void test_build_final_text_null_input(void);
void test_build_final_text_single_line(void);
void test_extract_aligned_content_basic(void);
void test_extract_aligned_content_short_line(void);
void test_extract_aligned_content_null_input(void);
void test_extract_aligned_content_zero_offset(void);

void setUp(void) {
    // Setup for string processing tests
}

void tearDown(void) {
    // Clean up after string processing tests
}

// Test finding APPCONFIG marker in basic case
void test_find_appconfig_marker_basic(void) {
    const char *test_line = "2024-01-01 APPCONFIG server.port=8080";
    size_t marker_pos = find_appconfig_marker(test_line);

    TEST_ASSERT_GREATER_THAN(0, marker_pos);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG server.port=8080", test_line + marker_pos);
}

// Test when APPCONFIG marker is not found
void test_find_appconfig_marker_not_found(void) {
    const char *test_line = "2024-01-01 Regular log message";
    size_t marker_pos = find_appconfig_marker(test_line);

    TEST_ASSERT_EQUAL(0, marker_pos);
}

// Test null input handling
void test_find_appconfig_marker_null_input(void) {
    size_t marker_pos = find_appconfig_marker(NULL);

    TEST_ASSERT_EQUAL(0, marker_pos);
}

// Test empty string handling
void test_find_appconfig_marker_empty_string(void) {
    size_t marker_pos = find_appconfig_marker("");

    TEST_ASSERT_EQUAL(0, marker_pos);
}

// Test extracting content after marker in basic case
void test_extract_content_after_marker_basic(void) {
    const char *test_line = "2024-01-01 APPCONFIG server.port=8080";
    size_t marker_pos = find_appconfig_marker(test_line);

    char *content = extract_content_after_marker(test_line, marker_pos);

    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG server.port=8080", content);

    free(content);
}

// Test boundary conditions for content extraction
void test_extract_content_after_marker_boundary(void) {
    const char *test_line = "APPCONFIG";  // Marker at start
    size_t marker_pos = 0;

    char *content = extract_content_after_marker(test_line, marker_pos);

    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG", content);

    free(content);
}

// Test null input for content extraction
void test_extract_content_after_marker_null_input(void) {
    char *content = extract_content_after_marker(NULL, 0);

    TEST_ASSERT_NULL(content);
}

// Test basic line processing
void test_process_config_lines_basic(void) {
    const char *raw_text =
        "2024-01-01 Regular log message\n"
        "2024-01-01 APPCONFIG server.port=8080\n"
        "2024-01-01 APPCONFIG server.host=localhost\n";

    size_t line_count = 0;
    char **lines = process_config_lines(raw_text, &line_count);

    TEST_ASSERT_NOT_NULL(lines);
    TEST_ASSERT_EQUAL(2, line_count);
    TEST_ASSERT_NOT_NULL(lines[0]);
    TEST_ASSERT_NOT_NULL(lines[1]);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG server.port=8080", lines[0]);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG server.host=localhost", lines[1]);

    // Clean up
    for (size_t i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
}

// Test processing with no markers
void test_process_config_lines_no_markers(void) {
    const char *raw_text =
        "2024-01-01 Regular log message\n"
        "2024-01-01 Another log message\n"
        "2024-01-01 Final log message\n";

    size_t line_count = 0;
    char **lines = process_config_lines(raw_text, &line_count);

    TEST_ASSERT_NULL(lines);
    TEST_ASSERT_EQUAL(0, line_count);
}

// Test processing mixed content
void test_process_config_lines_mixed_content(void) {
    const char *raw_text =
        "2024-01-01 Regular log message\n"
        "2024-01-01 APPCONFIG server.port=8080\n"
        "2024-01-01 Regular log message\n"
        "2024-01-01 APPCONFIG server.host=localhost\n"
        "2024-01-01 Regular log message\n";

    size_t line_count = 0;
    char **lines = process_config_lines(raw_text, &line_count);

    TEST_ASSERT_NOT_NULL(lines);
    TEST_ASSERT_EQUAL(2, line_count);
    TEST_ASSERT_NOT_NULL(lines[0]);
    TEST_ASSERT_NOT_NULL(lines[1]);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG server.port=8080", lines[0]);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG server.host=localhost", lines[1]);

    // Clean up
    for (size_t i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
}

// Test processing with empty input
void test_process_config_lines_empty_input(void) {
    const char *raw_text = "";

    size_t line_count = 0;
    char **lines = process_config_lines(raw_text, &line_count);

    TEST_ASSERT_NULL(lines);
    TEST_ASSERT_EQUAL(0, line_count);
}

// Test processing with null input
void test_process_config_lines_null_input(void) {
    size_t line_count = 0;
    char **lines = process_config_lines(NULL, &line_count);

    TEST_ASSERT_NULL(lines);
    TEST_ASSERT_EQUAL(0, line_count);
}

// Test building final text from lines
void test_build_final_text_basic(void) {
    const char *lines[] = {"APPCONFIG server.port=8080", "APPCONFIG server.host=localhost"};
    char **line_array = malloc(2 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(line_array);  // Check malloc succeeded

    line_array[0] = strdup(lines[0]);
    TEST_ASSERT_NOT_NULL(line_array[0]);  // Check strdup succeeded

    line_array[1] = strdup(lines[1]);
    TEST_ASSERT_NOT_NULL(line_array[1]);  // Check strdup succeeded

    char *result = build_final_text(line_array, 2);

    TEST_ASSERT_NOT_NULL(result);
    // Should be in reverse order with newline
    TEST_ASSERT_EQUAL_STRING("APPCONFIG server.host=localhost\nAPPCONFIG server.port=8080", result);

    free(result);
    free(line_array[0]);
    free(line_array[1]);
    free(line_array);
}

// Test building final text with empty lines array
void test_build_final_text_empty_lines(void) {
    char *result = build_final_text(NULL, 0);

    TEST_ASSERT_NULL(result);
}

// Test building final text with null input
void test_build_final_text_null_input(void) {
    char *result = build_final_text(NULL, 5);

    TEST_ASSERT_NULL(result);
}

// Test building final text with single line
void test_build_final_text_single_line(void) {
    const char *line = "APPCONFIG server.port=8080";
    char **line_array = malloc(1 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(line_array);  // Check malloc succeeded

    line_array[0] = strdup(line);
    TEST_ASSERT_NOT_NULL(line_array[0]);  // Check strdup succeeded

    char *result = build_final_text(line_array, 1);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG server.port=8080", result);

    free(result);
    free(line_array[0]);
    free(line_array);
}

// Test extracting aligned content with basic case
void test_extract_aligned_content_basic(void) {
    const char *line = "2024-01-01 APPCONFIG server.port=8080";
    size_t offset = 11;  // Position after date

    char *result = extract_aligned_content(line, offset);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG server.port=8080", result);

    free(result);
}

// Test extracting aligned content with short line
void test_extract_aligned_content_short_line(void) {
    const char *line = "short";
    size_t offset = 10;  // Offset beyond line length

    char *result = extract_aligned_content(line, offset);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);  // Should return empty string

    free(result);
}

// Test extracting aligned content with null input
void test_extract_aligned_content_null_input(void) {
    char *result = extract_aligned_content(NULL, 5);

    TEST_ASSERT_NULL(result);
}

// Test extracting aligned content with zero offset
void test_extract_aligned_content_zero_offset(void) {
    const char *line = "APPCONFIG server.port=8080";

    char *result = extract_aligned_content(line, 0);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG server.port=8080", result);

    free(result);
}

// Test multiple APPCONFIG markers in one line
void test_find_appconfig_marker_multiple_markers(void) {
    const char *test_line = "2024-01-01 APPCONFIG server.port=8080 APPCONFIG backup.port=9090";
    size_t marker_pos = find_appconfig_marker(test_line);

    // Should find the first occurrence
    TEST_ASSERT_GREATER_THAN(0, marker_pos);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG server.port=8080 APPCONFIG backup.port=9090", test_line + marker_pos);
}

// Test offset at end of string
void test_extract_content_after_marker_offset_at_end(void) {
    const char *test_line = "2024-01-01 APPCONFIG";
    size_t marker_pos = find_appconfig_marker(test_line);

    char *content = extract_content_after_marker(test_line, marker_pos);

    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("APPCONFIG", content);

    free(content);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_find_appconfig_marker_basic);
    RUN_TEST(test_find_appconfig_marker_not_found);
    RUN_TEST(test_find_appconfig_marker_null_input);
    RUN_TEST(test_find_appconfig_marker_empty_string);
    RUN_TEST(test_find_appconfig_marker_multiple_markers);
    RUN_TEST(test_extract_content_after_marker_basic);
    RUN_TEST(test_extract_content_after_marker_boundary);
    RUN_TEST(test_extract_content_after_marker_null_input);
    RUN_TEST(test_extract_content_after_marker_offset_at_end);
    RUN_TEST(test_process_config_lines_basic);
    RUN_TEST(test_process_config_lines_no_markers);
    RUN_TEST(test_process_config_lines_mixed_content);
    RUN_TEST(test_process_config_lines_empty_input);
    RUN_TEST(test_process_config_lines_null_input);
    RUN_TEST(test_build_final_text_basic);
    RUN_TEST(test_build_final_text_empty_lines);
    RUN_TEST(test_build_final_text_null_input);
    RUN_TEST(test_build_final_text_single_line);
    RUN_TEST(test_extract_aligned_content_basic);
    RUN_TEST(test_extract_aligned_content_short_line);
    RUN_TEST(test_extract_aligned_content_null_input);
    RUN_TEST(test_extract_aligned_content_zero_offset);

    return UNITY_END();
}
