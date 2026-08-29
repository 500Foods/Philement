/*
 * Unity Test File: Web Server Request - URL Looks Like File Test
 * This file contains unit tests for web_server_url_looks_like_file() function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_request.h>

// Standard library includes
#include <string.h>

// Forward declaration for function being tested
bool web_server_url_looks_like_file(const char *url);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// NULL parameter
static void test_url_looks_like_file_null(void) {
    TEST_ASSERT_FALSE(web_server_url_looks_like_file(NULL));
}

// Empty string
static void test_url_looks_like_file_empty(void) {
    TEST_ASSERT_FALSE(web_server_url_looks_like_file(""));
}

// File with extension, no path (no slash)
static void test_url_looks_like_file_no_slash_with_ext(void) {
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("file.html"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("script.js"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("config.json"));
}

// File with extension, with full path
static void test_url_looks_like_file_path_with_ext(void) {
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/path/to/file.html"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/assets/js/app.min.js"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/data/config.json"));
}

// Path with no dot (directory-like or endpoint)
static void test_url_looks_like_file_no_dot(void) {
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("file"));
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("/path/to/resource"));
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("/api/v1/users"));
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("/index"));
}

// Root path
static void test_url_looks_like_file_root(void) {
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("/"));
}

// Trailing slash (directory)
static void test_url_looks_like_file_trailing_slash(void) {
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("/path/to/"));
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("/"));
}

// Dot only (filename starts with dot - hidden file)
static void test_url_looks_like_file_dotfile_no_slash(void) {
    TEST_ASSERT_TRUE(web_server_url_looks_like_file(".html"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file(".env"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file(".htaccess"));
}

// Dotfile with path (hidden file in a directory)
static void test_url_looks_like_file_dotfile_with_path(void) {
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/path/to/.env"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/app/.hidden"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/.bashrc"));
}

// Multiple extensions
static void test_url_looks_like_file_multiple_extensions(void) {
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("file.tar.gz"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/path/to/archive.tar.gz"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("app.min.js"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("file.js.map"));
}

// File with dot but no extension (trailing dot)
static void test_url_looks_like_file_trailing_dot(void) {
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/path/to/file."));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("file."));
}

// Dot in directory name, not in filename
static void test_url_looks_like_file_dot_in_dir_name(void) {
    // The dot is in the directory name, not the filename - no file extension
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("/path.dir/to/resource"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/path.dir/file.html"));
}

// Single character after dot
static void test_url_looks_like_file_single_char_ext(void) {
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("file.c"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("main.h"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/src/file.c"));
}

// Common web file extensions
static void test_url_looks_like_file_common_extensions(void) {
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/index.html"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/style.css"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/app.js"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/data.json"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/image.png"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/photo.jpg"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/icon.svg"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/font.woff"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/compiled.wasm"));
}

// URL with query string (dot is still after slash)
static void test_url_looks_like_file_with_query_string(void) {
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/path/to/file.html?v=1.0"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/api/script.js?cb=123"));
}

// URL with query string but no file extension
static void test_url_looks_like_file_no_ext_with_query(void) {
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("/api/v1/users?name=john"));
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("/path/to/resource?page=1"));
}

// Path that looks like a file but is actually a directory with dots
static void test_url_looks_like_file_directory_with_dot(void) {
    // A trailing slash means the last character is '/', and the dot
    // in "dir.name" is before that slash, so it is treated as a
    // directory, not a file.
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("/path/to/dir.name/"));
}

// Complex nested paths
static void test_url_looks_like_file_complex_paths(void) {
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/a/b/c/d/e/f/file.txt"));
    TEST_ASSERT_FALSE(web_server_url_looks_like_file("/a/b/c/d/e/f/resource"));
    TEST_ASSERT_TRUE(web_server_url_looks_like_file("/assets/vendor/jquery.min.js"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_url_looks_like_file_null);
    RUN_TEST(test_url_looks_like_file_empty);
    RUN_TEST(test_url_looks_like_file_no_slash_with_ext);
    RUN_TEST(test_url_looks_like_file_path_with_ext);
    RUN_TEST(test_url_looks_like_file_no_dot);
    RUN_TEST(test_url_looks_like_file_root);
    RUN_TEST(test_url_looks_like_file_trailing_slash);
    RUN_TEST(test_url_looks_like_file_dotfile_no_slash);
    RUN_TEST(test_url_looks_like_file_dotfile_with_path);
    RUN_TEST(test_url_looks_like_file_multiple_extensions);
    RUN_TEST(test_url_looks_like_file_trailing_dot);
    RUN_TEST(test_url_looks_like_file_dot_in_dir_name);
    RUN_TEST(test_url_looks_like_file_single_char_ext);
    RUN_TEST(test_url_looks_like_file_common_extensions);
    RUN_TEST(test_url_looks_like_file_with_query_string);
    RUN_TEST(test_url_looks_like_file_no_ext_with_query);
    RUN_TEST(test_url_looks_like_file_directory_with_dot);
    RUN_TEST(test_url_looks_like_file_complex_paths);

    return UNITY_END();
}
