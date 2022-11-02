#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
    const char* test_name;
    size_t total_tests;
    size_t failed_tests;
} test_result_t;

typedef struct {
    const char* bench_name;
    struct timespec elapsed;
    size_t iterations;
} bench_result_t;

#define TEST(name)                                                         \
    static void test_function_##name(test_result_t* test_result);          \
    static test_result_t name() {                                          \
        test_result_t test_result = {.total_tests = 0, .failed_tests = 0}; \
        test_result.test_name = #name;                                     \
        test_function_##name(&test_result);                                \
        return test_result;                                                \
    }                                                                      \
    static void test_function_##name(test_result_t* test_result)

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

#define TEST_SUITE(name)                                                     \
    {                                                                        \
        printf("Testing %s:\n", name);                                       \
        test_result_t suite_results = {.total_tests = 0, .failed_tests = 0}; \
        test_result_t (*__test_functions[])() = {
#define END_TEST_SUITE()                                                     \
    }                                                                        \
    ;                                                                        \
    for (size_t i = 0;                                                       \
         i < sizeof(__test_functions) / sizeof(*__test_functions); ++i) {    \
        const test_result_t test_result = __test_functions[i]();             \
        printf("    %s: %zd/%zd\n", test_result.test_name,                   \
               test_result.total_tests - test_result.failed_tests,           \
               test_result.total_tests);                                     \
        suite_results.total_tests += test_result.total_tests;                \
        suite_results.failed_tests += test_result.failed_tests;              \
    }                                                                        \
    printf("Suite assertions %zd, failed %zd.\n", suite_results.total_tests, \
           suite_results.failed_tests);                                      \
    final_results.total_tests += suite_results.total_tests;                  \
    final_results.failed_tests += suite_results.failed_tests;                \
    }                                                                        \
    ;

#define TEST_MAIN()                   \
    int main(int argc, char** argv) { \
        (void)argc;                   \
        (void)argv;                   \
        test_result_t final_results = {.total_tests = 0, .failed_tests = 0};

#define END_MAIN()                                                           \
    printf("Total assertions %zd, failed %zd.\n", final_results.total_tests, \
           final_results.failed_tests);                                      \
    return final_results.failed_tests > 0 ? 1 : 0;                           \
    }

#define BENCH(name)                                                        \
    static bench_result_t name() {                                         \
        bench_result_t __result = {.elapsed = {.tv_sec = 0, .tv_nsec = 0}, \
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
        bench_result_t (*__bench_functions[])() = {
#define END_BENCH_SUITE()                                                   \
    }                                                                       \
    ;                                                                       \
    for (size_t i = 0;                                                      \
         i < sizeof(__bench_functions) / sizeof(*__bench_functions); ++i) { \
        const bench_result_t bench_result = __bench_functions[i]();         \
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
