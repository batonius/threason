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
    THSN_TOKEN_SYMBOL,
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
    
    if (THSN_SLICE_EMPTY(*buffer_slice)) {
        *token = THSN_TOKEN_EOF;
        return THSN_RESULT_SUCCESS;
    }
    
    char c = THSN_SLICE_NEXT_CHAR_UNSAFE(*buffer_slice);

    while (isspace(c) && !THSN_SLICE_EMPTY(*buffer_slice) ) {
        c = THSN_SLICE_NEXT_CHAR_UNSAFE(*buffer_slice);
    }

    if (THSN_SLICE_EMPTY(*buffer_slice)) {
        *token = THSN_TOKEN_EOF;
        return THSN_RESULT_SUCCESS;
    }

    *token = THSN_TOKEN_ERROR;
    switch (c) {
        case '{':
            *token = THSN_TOKEN_OPEN_BRACE;
            break;
        case '}':
            *token = THSN_TOKEN_CLOSED_BRACE;
            break;
        case '[':
            *token = THSN_TOKEN_OPEN_BRACKET;
            break;
        case ']':
            *token = THSN_TOKEN_CLOSED_BRACKET;
            break;
        case ',':
            *token = THSN_TOKEN_COMMA;
            break;
        case ':':
            *token = THSN_TOKEN_COLON;
            break;
    }

    if (*token != THSN_TOKEN_ERROR) {
        token_slice->data = buffer_slice->data;
        token_slice->size = 1;
        THSN_SLICE_ADVANCE_UNSAFE(*token_slice, 1);
        return THSN_RESULT_SUCCESS;
    }

    if (*buffer_slice->data == '"') {
        THSN_SLICE_ADVANCE_UNSAFE(*buffer_slice, 1);
        if (*buffer_slice->data == '"') {
            token_slice->data = buffer_slice->data;
            token_slice->size = 0;
            THSN_SLICE
        }
        char* string_start = buffer_slice->data;
        size_t string_size = buffer_slice->size;
        bool escaped = false;
        do {
            char* closing_quotes =
                memchr(buffer_slice->data, '"', buffer_slice->size);
            if (closing_quotes == NULL) {
                token_slice->data = string_start;
                token_slice->size = string_size;
                *token = escaped ? THSN_TOKEN_UNCLOSED_ESCAPED_STRING
                                 : THSN_TOKEN_UNCLOSED_STRING;
                return THSN_RESULT_SUCCESS;
            }
        } while (1);
    }

    return THSN_RESULT_INPUT_ERROR;
}
