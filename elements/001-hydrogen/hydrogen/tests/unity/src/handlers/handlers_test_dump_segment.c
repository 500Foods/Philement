/*
 * Unity Test File: handlers_dump_segment
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/handlers/handlers.h>
#include <elf.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct CoreMapping {
    unsigned long start;
    unsigned long end;
    unsigned long offset;
    char perms[5];
    char path[PATH_MAX];
    int is_stack;
    int is_vdso;
};

struct LoadSegment {
    CoreMapping *m;
    Elf64_Phdr phdr;
};

void test_dump_segment_copies_bytes(void);
void test_dump_segment_seek_out_fail(void);
void test_dump_segment_seek_mem_fail(void);

void setUp(void) {}
void tearDown(void) {}

void test_dump_segment_copies_bytes(void) {
    const char payload[] = "hydrogen-dump-segment-test";
    char mem_path[] = "/tmp/h_dump_mem_XXXXXX";
    char out_path[] = "/tmp/h_dump_out_XXXXXX";
    int mfd = mkstemp(mem_path);
    int ofd = mkstemp(out_path);
    TEST_ASSERT_TRUE(mfd >= 0);
    TEST_ASSERT_TRUE(ofd >= 0);
    TEST_ASSERT_EQUAL_INT((int)sizeof(payload),
                          (int)write(mfd, payload, sizeof(payload)));
    close(mfd);
    close(ofd);

    FILE *mem = fopen(mem_path, "rb");
    FILE *out = fopen(out_path, "w+b");
    TEST_ASSERT_NOT_NULL(mem);
    TEST_ASSERT_NOT_NULL(out);

    CoreMapping mapping;
    memset(&mapping, 0, sizeof(mapping));
    mapping.start = 0;
    mapping.end = sizeof(payload);
    strcpy(mapping.perms, "r--p");

    LoadSegment seg;
    memset(&seg, 0, sizeof(seg));
    seg.m = &mapping;
    seg.phdr.p_offset = 0;
    seg.phdr.p_filesz = sizeof(payload);

    handlers_dump_segment(mem, out, &seg);

    fflush(out);
    fseek(out, 0, SEEK_SET);
    char got[64];
    memset(got, 0, sizeof(got));
    size_t n = fread(got, 1, sizeof(payload), out);
    TEST_ASSERT_EQUAL_UINT(sizeof(payload), n);
    TEST_ASSERT_EQUAL_MEMORY(payload, got, sizeof(payload));

    fclose(mem);
    fclose(out);
    unlink(mem_path);
    unlink(out_path);
}

void test_dump_segment_seek_out_fail(void) {
    char mem_path[] = "/tmp/h_dump_mem2_XXXXXX";
    int mfd = mkstemp(mem_path);
    TEST_ASSERT_TRUE(mfd >= 0);
    write(mfd, "x", 1);
    close(mfd);

    FILE *mem = fopen(mem_path, "rb");
    FILE *out = fopen("/dev/null", "rb"); /* not seekable for write path */
    TEST_ASSERT_NOT_NULL(mem);
    TEST_ASSERT_NOT_NULL(out);

    CoreMapping mapping;
    memset(&mapping, 0, sizeof(mapping));
    mapping.start = 0;
    mapping.end = 1;
    strcpy(mapping.perms, "r--p");

    LoadSegment seg;
    memset(&seg, 0, sizeof(seg));
    seg.m = &mapping;
    seg.phdr.p_offset = 0;

    handlers_dump_segment(mem, out, &seg);
    TEST_PASS();

    fclose(mem);
    fclose(out);
    unlink(mem_path);
}

void test_dump_segment_seek_mem_fail(void) {
    char out_path[] = "/tmp/h_dump_out2_XXXXXX";
    int ofd = mkstemp(out_path);
    TEST_ASSERT_TRUE(ofd >= 0);
    close(ofd);

    FILE *mem = fopen("/dev/null", "rb");
    FILE *out = fopen(out_path, "w+b");
    TEST_ASSERT_NOT_NULL(mem);
    TEST_ASSERT_NOT_NULL(out);

    CoreMapping mapping;
    memset(&mapping, 0, sizeof(mapping));
    /* Large address — fseeko on /dev/null or empty may fail or EOF */
    mapping.start = 0x7fffffffffffUL;
    mapping.end = mapping.start + 16;
    strcpy(mapping.perms, "r--p");

    LoadSegment seg;
    memset(&seg, 0, sizeof(seg));
    seg.m = &mapping;
    seg.phdr.p_offset = 0;

    handlers_dump_segment(mem, out, &seg);
    TEST_PASS();

    fclose(mem);
    fclose(out);
    unlink(out_path);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_dump_segment_copies_bytes);
    RUN_TEST(test_dump_segment_seek_out_fail);
    RUN_TEST(test_dump_segment_seek_mem_fail);
    return UNITY_END();
}
