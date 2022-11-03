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
    THSN_TOKEN_UNCLOSED_STRING,
    THSN_TOKEN_COLON,
    THSN_TOKEN_COMMA,
    THSN_TOKEN_OPEN_BRACKET,
    THSN_TOKEN_CLOSED_BRACKET,
    THSN_TOKEN_OPEN_BRACE,
    THSN_TOKEN_CLOSED_BRACE,
} thsn_token_t;

thsn_result_t thsn_next_token(thsn_slice_t* /*in/out*/ buffer_slice,
                              thsn_slice_t* /*out*/ token_slice,
                              thsn_token_t* /*out*/ token);
