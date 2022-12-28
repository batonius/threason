#ifndef THSN_TEST_TOKENIZER_H
#define THSN_TEST_TOKENIZER_H

#include "testing.h"
#include "tokenizer.h"

TEST(valid_tokens) {
    struct {
        char* input;
        char* token_str;
        char* rest;
        ThsnToken token;
    } test_values[] = {
        {"", "", "", THSN_TOKEN_EOF},
        {"      \t\n    ", "", "", THSN_TOKEN_EOF},
        {"\t{!", "{", "!", THSN_TOKEN_OPEN_BRACE},
        {"\t}!", "}", "!", THSN_TOKEN_CLOSED_BRACE},
        {"[123", "[", "123", THSN_TOKEN_OPEN_BRACKET},
        {" ]", "]", "", THSN_TOKEN_CLOSED_BRACKET},
        {",,,,,", ",", ",,,,", THSN_TOKEN_COMMA},
        {":", ":", "", THSN_TOKEN_COLON},
        {"nullllll", "null", "llll", THSN_TOKEN_NULL},
        {"   true", "true", "", THSN_TOKEN_TRUE},
        {"false1", "false", "1", THSN_TOKEN_FALSE},
        {"\"\"", "", "", THSN_TOKEN_STRING},
        {"\"\\\"", "\\\"", "", THSN_TOKEN_UNCLOSED_STRING},
        {"\"\\\\\"", "\\\\", "", THSN_TOKEN_STRING},
        {"\"\\\\\\\"", "\\\\\\\"", "", THSN_TOKEN_UNCLOSED_STRING},
        {"\"ABC\\\"\"\"", "ABC\\\"", "\"", THSN_TOKEN_STRING},
        {"\t \"abc \\\"", "abc \\\"", "", THSN_TOKEN_UNCLOSED_STRING},
        {"\"", "", "", THSN_TOKEN_UNCLOSED_STRING},
        {"\n1234!", "1234", "!", THSN_TOKEN_INT},
        {"-99.!!!", "-99.", "!!!", THSN_TOKEN_FLOAT},
        {" \t 12.34e10!", "12.34e10", "!", THSN_TOKEN_FLOAT},
        {" \t 12.34e+10!", "12.34e+10", "!", THSN_TOKEN_FLOAT},
        {" \t 12.34e-10!", "12.34e-10", "!", THSN_TOKEN_FLOAT},
    };

    for (size_t i = 0; i < sizeof(test_values) / sizeof(test_values[0]); ++i) {
        ThsnSlice input_slice;
        ASSERT_SUCCESS(
            thsn_slice_from_c_str(test_values[i].input, &input_slice));
        ThsnSlice token_slice;
        ThsnToken token;
        ASSERT_EQ(thsn_next_token(&input_slice, &token_slice, &token),
                  THSN_RESULT_SUCCESS);
        ASSERT_EQ(token, test_values[i].token);
        const size_t token_str_len = strlen(test_values[i].token_str);
        ASSERT_EQ(token_str_len, token_slice.size);
        const size_t rest_len = strlen(test_values[i].rest);
        ASSERT_EQ(rest_len, input_slice.size);
        ASSERT_STRN_EQ(input_slice.data, test_values[i].rest, rest_len);
    }
}

TEST(invalid_tokens) {
    char* invalid_tokens[] = {
        /* random symbols */
        "!",
        "%",
        ";",
        "\\",
        "/",
        "@",
        "#",
        "^",
        "&",
        "=",
        "+",
        "-",
        "_",
        "(",
        ")",
        "|",
        "~",
        "`",
        "'",
        ".",
        "*",
        "$",
        /* keywords */
        "nil",
        "True",
        "FALSE",
        "f",
        "n",
        "T",
        /* invalid numbers */
        "123.323.3",
        "123123e10.0",
        "-123e123e123",
        "0e-",
        "10e++8",
    };
    for (size_t i = 0; i < sizeof(invalid_tokens) / sizeof(invalid_tokens[0]);
         ++i) {
        ThsnSlice input_slice;
        ASSERT_SUCCESS(thsn_slice_from_c_str(invalid_tokens[i], &input_slice));
        ThsnSlice token_slice;
        ThsnToken token;
        ASSERT_EQ(thsn_next_token(&input_slice, &token_slice, &token),
                  THSN_RESULT_INPUT_ERROR);
    }
}

/* clang-format off */

TEST_SUITE(tokenizer)
    valid_tokens,
    invalid_tokens, 
END_TEST_SUITE()

#endif
