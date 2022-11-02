#pragma once
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "result.h"
#include "slice.h"

typedef enum {
    THSN_TOKEN_ERROR,
    THSN_TOKEN_EOF,
    THSN_TOKEN_INT,
    THSN_TOKEN_FLOAT,
    THSN_TOKEN_NULL,
    THSN_TOKEN_TRUE,
    THSN_TOKEN_FALSE,
    THSN_TOKEN_STRING,
    THSN_TOKEN_ESCAPED_STRING,
    THSN_TOKEN_UNCLOSED_STRING,
    THSN_TOKEN_UNCLOSED_ESCAPED_STRING,
    THSN_TOKEN_COLON,
    THSN_TOKEN_COMMA,
    THSN_TOKEN_OPEN_BRACKET,
    THSN_TOKEN_CLOSED_BRACKET,
    THSN_TOKEN_OPEN_BRACE,
    THSN_TOKEN_CLOSED_BRACE,
} thsn_token_t;

thsn_result_t thsn_next_token(thsn_slice_t* /*in/out*/ buffer_slice,
                              thsn_slice_t* /*out*/ token_slice,
                              thsn_token_t* /*out*/ token) {
    BAIL_ON_NULL_INPUT(buffer_slice);
    BAIL_ON_NULL_INPUT(buffer_slice->data);
    BAIL_ON_NULL_INPUT(token_slice);

    char c = 0;

    do {
        if (THSN_SLICE_EMPTY(*buffer_slice)) {
            *token = THSN_TOKEN_EOF;
            token_slice->data = buffer_slice->data;
            token_slice->size = 0;
            return THSN_RESULT_SUCCESS;
        }

        c = THSN_SLICE_NEXT_CHAR_UNSAFE(*buffer_slice);
    } while (isspace(c));

    *token = THSN_TOKEN_ERROR;
    token_slice->data = buffer_slice->data - 1;
    token_slice->size = 1;

    switch (c) {
        case '{':
            *token = THSN_TOKEN_OPEN_BRACE;
            return THSN_RESULT_SUCCESS;
        case '}':
            *token = THSN_TOKEN_CLOSED_BRACE;
            return THSN_RESULT_SUCCESS;
        case '[':
            *token = THSN_TOKEN_OPEN_BRACKET;
            return THSN_RESULT_SUCCESS;
        case ']':
            *token = THSN_TOKEN_CLOSED_BRACKET;
            return THSN_RESULT_SUCCESS;
        case ',':
            *token = THSN_TOKEN_COMMA;
            return THSN_RESULT_SUCCESS;
        case ':':
            *token = THSN_TOKEN_COLON;
            return THSN_RESULT_SUCCESS;
        case 'n':
            if (buffer_slice->size >= 3 &&
                strncmp(buffer_slice->data, "ull", 3) == 0) {
                *token = THSN_TOKEN_NULL;
                token_slice->size = 4;
                THSN_SLICE_ADVANCE_UNSAFE(*buffer_slice, 3);
                return THSN_RESULT_SUCCESS;
            } else {
                return THSN_RESULT_INPUT_ERROR;
            }
        case 't':
            if (buffer_slice->size >= 3 &&
                strncmp(buffer_slice->data, "rue", 3) == 0) {
                *token = THSN_TOKEN_TRUE;
                token_slice->size = 4;
                THSN_SLICE_ADVANCE_UNSAFE(*buffer_slice, 3);
                return THSN_RESULT_SUCCESS;
            } else {
                return THSN_RESULT_INPUT_ERROR;
            }
        case 'f':
            if (buffer_slice->size >= 4 &&
                strncmp(buffer_slice->data, "alse", 4) == 0) {
                *token = THSN_TOKEN_FALSE;
                token_slice->size = 5;
                THSN_SLICE_ADVANCE_UNSAFE(*buffer_slice, 4);
                return THSN_RESULT_SUCCESS;
            } else {
                return THSN_RESULT_INPUT_ERROR;
            }
        case '"': {
            token_slice->data = buffer_slice->data;
            token_slice->size = 0;
            bool escape_present = false;
            do {
                const char* closing_quotes =
                    memchr(buffer_slice->data, '"', buffer_slice->size);
                if (closing_quotes == NULL) {
                    token_slice->size += buffer_slice->size;
                    THSN_SLICE_ADVANCE_UNSAFE(*buffer_slice,
                                              buffer_slice->size);
                    *token = escape_present ? THSN_TOKEN_UNCLOSED_ESCAPED_STRING
                                            : THSN_TOKEN_UNCLOSED_STRING;
                    return THSN_RESULT_SUCCESS;
                }
                // `closing quotes` is between buffer_slice->data and
                // buffer_slice->data + bufer_slice.size - 1
                if (closing_quotes == buffer_slice->data) {
                    THSN_SLICE_ADVANCE_UNSAFE(*buffer_slice, 1);
                    *token = escape_present ? THSN_TOKEN_ESCAPED_STRING
                                            : THSN_TOKEN_STRING;
                    return THSN_RESULT_SUCCESS;
                }
                // `closing quotes` is between buffer_slice->data+1 and
                // buffer_slice->data + bufer_slice.size - 1
                size_t i = 1;
                while (closing_quotes - i >= buffer_slice->data &&
                       closing_quotes[-i] == '\\') {
                    ++i;
                }
                if (i % 2 == 1) {
                    const size_t step_size =
                        closing_quotes - buffer_slice->data;
                    token_slice->size += step_size;
                    THSN_SLICE_ADVANCE_UNSAFE(*buffer_slice, step_size + 1);
                    *token = escape_present ? THSN_TOKEN_ESCAPED_STRING
                                            : THSN_TOKEN_STRING;
                    return THSN_RESULT_SUCCESS;
                }
                // `+1` to step over the escaped quote
                const size_t step_size =
                    (closing_quotes - buffer_slice->data) + 1;
                token_slice->size += step_size;
                THSN_SLICE_ADVANCE_UNSAFE(*buffer_slice, step_size);
                escape_present = true;
            } while (1);
        }
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            bool dot_present = false;
            bool e_present = false;
            bool done = false;
            bool waiting_for_sign = false;
            while (!THSN_SLICE_EMPTY(*buffer_slice) && !done) {
                c = THSN_SLICE_NEXT_CHAR_UNSAFE(*buffer_slice);
                ++token_slice->size;
                switch (c) {
                    case '.':
                        if (dot_present || e_present) {
                            return THSN_RESULT_INPUT_ERROR;
                        } else {
                            dot_present = true;
                        }
                        break;
                    case 'e':
                    case 'E':
                        if (e_present) {
                            return THSN_RESULT_INPUT_ERROR;
                        } else {
                            e_present = true;
                            waiting_for_sign = true;
                        }
                        break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        waiting_for_sign = false;
                        break;
                    case '+':
                    case '-':
                        if (waiting_for_sign) {
                            waiting_for_sign = false;
                            break;
                        }
                        [[fallthrough]];
                    default:
                        THSN_SLICE_ADVANCE_UNSAFE(*buffer_slice, -1);
                        --token_slice->size;
                        done = true;
                        break;
                }
            }
            if (token_slice->data[token_slice->size - 1] == '-' ||
                token_slice->data[token_slice->size - 1] == '+') {
                return THSN_RESULT_INPUT_ERROR;
            }
            *token =
                dot_present || e_present ? THSN_TOKEN_FLOAT : THSN_TOKEN_INT;
            return THSN_RESULT_SUCCESS;
        }
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
}
