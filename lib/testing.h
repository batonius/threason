#ifndef THSN_TESTING_H
#define THSN_TESTING_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
    const char* test_name;
    size_t total_tests;
    size_t failed_tests;
} TestResult;

typedef struct {
    const char* bench_name;
    struct timespec elapsed;
    size_t iterations;
} BenchResult;

#define TEST(name)                                                      \
    void test_function_##name(TestResult* test_result);                 \
    TestResult name() {                                                 \
        TestResult test_result = {.total_tests = 0, .failed_tests = 0}; \
        test_result.test_name = #name;                                  \
        test_function_##name(&test_result);                             \
        return test_result;                                             \
    }                                                                   \
    void test_function_##name(TestResult* test_result)

#define ASSERT_EQ(A, B)                                                        \
    do {                                                                       \
        ++test_result->total_tests;                                            \
        if ((A) != (B)) {                                                      \
            ++test_result->failed_tests;                                       \
            fprintf(stderr,                                                    \
                    "        %s:%d: equality test failed: %s != %s. (%lld != " \
                    "%lld)\n",                                                 \
                    __FILE__, __LINE__, #A, #B, (long long)(A),                \
                    (long long)(B));                                           \
        }                                                                      \
    } while (0)

#define ASSERT_NEQ(A, B)                                                       \
    do {                                                                       \
        ++test_result->total_tests;                                            \
        if ((A) == (B)) {                                                      \
            ++test_result->failed_tests;                                       \
            fprintf(                                                           \
                stderr,                                                        \
                "        %s:%d: non-equality test failed: %s == %s. (%lld == " \
                "%lld)\n",                                                     \
                __FILE__, __LINE__, #A, #B, (long long)(A), (long long)(B));   \
        }                                                                      \
    } while (0)

#define ASSERT_STRN_EQ(A, B, n)                                      \
    do {                                                             \
        ++test_result->total_tests;                                  \
        if (strncmp((A), (B), (n)) != 0) {                           \
            ++test_result->failed_tests;                             \
            fprintf(stderr,                                          \
                    "        %s:%d: equality test failed: %s != %s " \
                    "(\"%.10s\" != \"%.10s\")\n",                    \
                    __FILE__, __LINE__, #A, #B, (A), (B));           \
        }                                                            \
    } while (0)

#define ASSERT_STRN_NEQ(A, B, n)                                         \
    do {                                                                 \
        ++test_result->total_tests;                                      \
        if (strncmp((A), (B), (n)) == 0) {                               \
            ++test_result->failed_tests;                                 \
            fprintf(stderr,                                              \
                    "        %s:%d: non-equality test failed: %s == %s " \
                    "(\"%.10s\" == \"%.10s\")\n",                        \
                    __FILE__, __LINE__, #A, #B, (A), (B));               \
        }                                                                \
    } while (0)

#define ASSERT_SUCCESS(v) ASSERT_EQ((v), THSN_RESULT_SUCCESS)

#define ASSERT_INPUT_ERROR(v) ASSERT_EQ((v), THSN_RESULT_INPUT_ERROR)

#define ASSERT_SLICE_EQ_BYTES(slice_)     \
    do {                                  \
        ++test_result->total_tests;       \
        ThsnSlice slice = slice_;         \
        const char* slice_name = #slice_; \
    char bytes[] = {
#define END_BYTES()                                                         \
    }                                                                       \
    ;                                                                       \
    if (slice.size != sizeof(bytes)) {                                      \
        ++test_result->failed_tests;                                        \
        fprintf(stderr,                                                     \
                "        %s:%d: slice %s has size %zu, expected %zu.\n",    \
                __FILE__, __LINE__, slice_name, slice.size, sizeof(bytes)); \
        fprintf(stderr, "        slice: ");                                 \
        for (size_t i = 0; i < 100 && i < slice.size; ++i) {                \
            fprintf(stderr, "%02X ", (unsigned char)slice.data[i]);         \
        }                                                                   \
        fprintf(stderr, "\n");                                              \
        fprintf(stderr, "        bytes: ");                                 \
        for (size_t i = 0; i < 100 && i < sizeof(bytes); ++i) {             \
            fprintf(stderr, "%02X ", (unsigned char)bytes[i]);              \
        }                                                                   \
        fprintf(stderr, "\n");                                              \
        break;                                                              \
    }                                                                       \
    if (memcmp(slice.data, bytes, slice.size) != 0) {                       \
        ++test_result->failed_tests;                                        \
        fprintf(stderr, "        %s:%d: slice %s differs from bytes.\n",    \
                __FILE__, __LINE__, slice_name);                            \
        fprintf(stderr, "        slice: ");                                 \
        for (size_t i = 0; i < 100 && i < slice.size; ++i) {                \
            fprintf(stderr, "%02X ", (unsigned char)slice.data[i]);         \
        }                                                                   \
        fprintf(stderr, "\n");                                              \
        fprintf(stderr, "        bytes: ");                                 \
        for (size_t i = 0; i < 100 && i < sizeof(bytes); ++i) {             \
            fprintf(stderr, "%02X ", (unsigned char)bytes[i]);              \
        }                                                                   \
        fprintf(stderr, "\n");                                              \
    }                                                                       \
    }                                                                       \
    while (0)

#define TEST_SUITE(name)                                                  \
    void test_suite_##name(TestResult* final_results) {                   \
        printf("Testing %s:\n", #name);                                   \
        TestResult suite_results = {.total_tests = 0, .failed_tests = 0}; \
        TestResult (*__test_functions[])() = {
#define END_TEST_SUITE()                                                       \
    }                                                                          \
    ;                                                                          \
    for (size_t i = 0;                                                         \
         i < sizeof(__test_functions) / sizeof(*__test_functions); ++i) {      \
        const TestResult test_result = __test_functions[i]();                  \
        printf("    %s: %zd/%zd\n", test_result.test_name,                     \
               test_result.total_tests - test_result.failed_tests,             \
               test_result.total_tests);                                       \
        suite_results.total_tests += test_result.total_tests;                  \
        suite_results.failed_tests += test_result.failed_tests;                \
    }                                                                          \
    printf("Suite assertions %zd, failed %zd.\n\n", suite_results.total_tests, \
           suite_results.failed_tests);                                        \
    final_results->total_tests += suite_results.total_tests;                   \
    final_results->failed_tests += suite_results.failed_tests;                 \
    }

#define RUN_SUITE(name) test_suite_##name(&final_results)

#define TEST_MAIN()                   \
    int main(int argc, char** argv) { \
        (void)argc;                   \
        (void)argv;                   \
        TestResult final_results = {.total_tests = 0, .failed_tests = 0};

#define END_MAIN()                                                           \
    printf("Total assertions %zd, failed %zd.\n", final_results.total_tests, \
           final_results.failed_tests);                                      \
    return final_results.failed_tests > 0 ? 1 : 0;                           \
    }

#define BENCH(name)                                                     \
    static BenchResult name() {                                         \
        BenchResult __result = {.elapsed = {.tv_sec = 0, .tv_nsec = 0}, \
                                .iterations = 0,                        \
                                .bench_name = #name};

#define BENCH_MEASURE(iters)                              \
    size_t __iters = (iters);                             \
    for (volatile size_t __i = 0; __i < __iters; ++__i) { \
        struct timespec __start_time;                     \
        clock_gettime(CLOCK_MONOTONIC, &__start_time);

#define END_MEASURE()                                                   \
    struct timespec __end_time;                                         \
    clock_gettime(CLOCK_MONOTONIC, &__end_time);                        \
    __end_time.tv_sec -= __start_time.tv_sec;                           \
    if (__end_time.tv_nsec < __start_time.tv_nsec) {                    \
        --__end_time.tv_sec;                                            \
        __end_time.tv_nsec += 1000000000LL - __start_time.tv_nsec;      \
    } else {                                                            \
        __end_time.tv_nsec -= __start_time.tv_nsec;                     \
    }                                                                   \
    __result.elapsed.tv_sec += __end_time.tv_sec;                       \
    __result.elapsed.tv_nsec += __end_time.tv_nsec;                     \
    }                                                                   \
    __result.iterations = __iters;                                      \
    __result.elapsed.tv_sec += __result.elapsed.tv_nsec / 1000000000LL; \
    __result.elapsed.tv_nsec %= 1000000000LL;

#define BENCH_MEASURE_LOOP(iters)                  \
    size_t __iters = (iters);                      \
    struct timespec __start_time;                  \
    clock_gettime(CLOCK_MONOTONIC, &__start_time); \
    for (volatile size_t __i = 0; __i < __iters; ++__i) {
#define END_MEASURE_LOOP()                                              \
    }                                                                   \
    struct timespec __end_time;                                         \
    clock_gettime(CLOCK_MONOTONIC, &__end_time);                        \
    __end_time.tv_sec -= __start_time.tv_sec;                           \
    if (__end_time.tv_nsec < __start_time.tv_nsec) {                    \
        --__end_time.tv_sec;                                            \
        __end_time.tv_nsec += 1000000000LL - __start_time.tv_nsec;      \
    } else {                                                            \
        __end_time.tv_nsec -= __start_time.tv_nsec;                     \
    }                                                                   \
    __result.elapsed.tv_sec += __end_time.tv_sec;                       \
    __result.elapsed.tv_nsec += __end_time.tv_nsec;                     \
    __result.elapsed.tv_sec += __result.elapsed.tv_nsec / 1000000000LL; \
    __result.elapsed.tv_nsec %= 1000000000LL;                           \
    __result.iterations = __iters;

#define END_BENCH()  \
    return __result; \
    }

#define BENCH_SUITE(name)                   \
    {                                       \
        printf("Benchmarking %s:\n", name); \
        BenchResult (*__bench_functions[])() = {
#define END_BENCH_SUITE()                                                   \
    }                                                                       \
    ;                                                                       \
    for (size_t i = 0;                                                      \
         i < sizeof(__bench_functions) / sizeof(*__bench_functions); ++i) { \
        const BenchResult bench_result = __bench_functions[i]();            \
        uint64_t elapsed_nsec = bench_result.elapsed.tv_sec * 1000000000 +  \
                                bench_result.elapsed.tv_nsec;               \
        uint64_t elapsed_nsec_per_iter =                                    \
            elapsed_nsec / bench_result.iterations;                         \
        printf(                                                             \
            "    %25s: %3lums %3luus %3luns per iter (total %3lums %3luus " \
            "%3luns for %zd "                                               \
            "iters)\n",                                                     \
            bench_result.bench_name, (elapsed_nsec_per_iter / 1000000),     \
            (elapsed_nsec_per_iter % 1000000) / 1000,                       \
            (elapsed_nsec_per_iter % 1000000) % 1000,                       \
            (elapsed_nsec / 1000000), (elapsed_nsec % 1000000) / 1000,      \
            (elapsed_nsec % 1000000) % 1000, bench_result.iterations);      \
    }                                                                       \
    }                                                                       \
    ;

#endif
