#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char* test_name;
    size_t total_tests;
    size_t failed_tests;
} test_result_t;

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
            fprintf(stderr, "        %s:%d: equality test failed: %s != %s\n", \
                    __FILE__, __LINE__, #A, #B);                               \
        }                                                                      \
    } while (0)

#define ASSERT_NEQ(A, B)                                                   \
    do {                                                                   \
        ++test_result->total_tests;                                        \
        if ((A) == (B)) {                                                  \
            ++test_result->failed_tests;                                   \
            fprintf(stderr,                                                \
                    "        %s:%d: non-equality test failed: %s == %s\n", \
                    __FILE__, __LINE__, #A, #B);                           \
        }                                                                  \
    } while (0)

#define ASSERT_STRN_EQ(A, B, n)                                                \
    do {                                                                       \
        ++test_result->total_tests;                                            \
        if (strncmp((A), (B), (n)) != 0) {                                     \
            ++test_result->failed_tests;                                       \
            fprintf(stderr, "        %s:%d: equality test failed: %s != %s\n", \
                    __FILE__, __LINE__, #A, #B);                               \
        }                                                                      \
    } while (0)

#define ASSERT_STRN_NEQ(A, B, n)                                           \
    do {                                                                   \
        ++test_result->total_tests;                                        \
        if (strncmp((A), (B), (n)) == 0) {                                 \
            ++test_result->failed_tests;                                   \
            fprintf(stderr,                                                \
                    "        %s:%d: non-equality test failed: %s == %s\n", \
                    __FILE__, __LINE__, #A, #B);                           \
        }                                                                  \
    } while (0)

#define TEST_SUITE(name)                                                     \
    {                                                                        \
        printf("%s:\n", name);                                               \
        test_result_t suite_results = {.total_tests = 0, .failed_tests = 0}; \
        test_result_t (*__test_functions[])() = {
#define END_SUITE()                                                       \
    }                                                                     \
    ;                                                                     \
    for (size_t i = 0;                                                    \
         i < sizeof(__test_functions) / sizeof(*__test_functions); ++i) { \
        const test_result_t test_result = __test_functions[i]();          \
        printf("    %s: %zd/%zd\n", test_result.test_name,                \
               test_result.total_tests - test_result.failed_tests,        \
               test_result.total_tests);                                  \
        suite_results.total_tests += test_result.total_tests;             \
        suite_results.failed_tests += test_result.failed_tests;           \
    }                                                                     \
    printf("Suite tests %zd, failed %zd.\n", suite_results.total_tests,   \
           suite_results.failed_tests);                                   \
    final_results.total_tests += suite_results.total_tests;               \
    final_results.failed_tests += suite_results.failed_tests;             \
    }                                                                     \
    ;

#define TEST_MAIN(name)               \
    int main(int argc, char** argv) { \
        (void)argc;                   \
        (void)argv;                   \
        printf("Testing %s\n", name); \
        test_result_t final_results = {.total_tests = 0, .failed_tests = 0};

#define END_MAIN()                                                      \
    printf("Total tests %zd, failed %zd.\n", final_results.total_tests, \
           final_results.failed_tests);                                 \
    return final_results.failed_tests > 0 ? 1 : 0;                      \
    }
