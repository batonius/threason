#include <ctype.h>
#include <stdbool.h>

#include "slice.h"
#include "tokenizer.h"

ThsnResult thsn_next_token(ThsnSlice* buffer_slice, ThsnSlice* token_slice,
                           ThsnToken* token) {
    BAIL_ON_NULL_INPUT(buffer_slice);
    BAIL_ON_NULL_INPUT(token_slice);
    BAIL_ON_NULL_INPUT(token);

    char c = 0;

    do {
        if (!thsn_slice_try_consume_char(buffer_slice, &c)) {
            *token = THSN_TOKEN_EOF;
            *token_slice = thsn_slice_make_empty();
            return THSN_RESULT_SUCCESS;
        }
    } while (isspace(c));

    *token = THSN_TOKEN_ERROR;
    *token_slice = thsn_slice_make(buffer_slice->data - 1, 1);

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
            if (buffer_slice->size >= 3 && buffer_slice->data[0] == 'u' &&
                buffer_slice->data[1] == 'l' && buffer_slice->data[2] == 'l') {
                *token = THSN_TOKEN_NULL;
                token_slice->size = 4;
                thsn_slice_advance_unsafe(buffer_slice, 3);
                return THSN_RESULT_SUCCESS;
            } else {
                return THSN_RESULT_INPUT_ERROR;
            }
        case 't':
            if (buffer_slice->size >= 3 && buffer_slice->data[0] == 'r' &&
                buffer_slice->data[1] == 'u' && buffer_slice->data[2] == 'e') {
                *token = THSN_TOKEN_TRUE;
                token_slice->size = 4;
                thsn_slice_advance_unsafe(buffer_slice, 3);
                return THSN_RESULT_SUCCESS;
            } else {
                return THSN_RESULT_INPUT_ERROR;
            }
        case 'f':
            if (buffer_slice->size >= 4 && buffer_slice->data[0] == 'a' &&
                buffer_slice->data[1] == 'l' && buffer_slice->data[2] == 's' &&
                buffer_slice->data[3] == 'e') {
                *token = THSN_TOKEN_FALSE;
                token_slice->size = 5;
                thsn_slice_advance_unsafe(buffer_slice, 4);
                return THSN_RESULT_SUCCESS;
            } else {
                return THSN_RESULT_INPUT_ERROR;
            }
        case '"': {
            *token_slice = thsn_slice_make(buffer_slice->data, 0);
            do {
                const char* const next_quotes =
                    memchr(buffer_slice->data, '"', buffer_slice->size);
                if (next_quotes == NULL) {
                    token_slice->size += buffer_slice->size;
                    thsn_slice_advance_unsafe(buffer_slice, buffer_slice->size);
                    *token = THSN_TOKEN_UNCLOSED_STRING;
                    return THSN_RESULT_SUCCESS;
                }

                if (next_quotes == buffer_slice->data) {
                    thsn_slice_advance_unsafe(buffer_slice, 1);
                    *token = THSN_TOKEN_STRING;
                    return THSN_RESULT_SUCCESS;
                }

                bool escaped = false;
                for (const char* slash_iterator = next_quotes - 1;
                     slash_iterator >= buffer_slice->data &&
                     *slash_iterator == '\\';
                     --slash_iterator) {
                    escaped = !escaped;
                }

                if (!escaped) {
                    const size_t step_size = next_quotes - buffer_slice->data;
                    token_slice->size += step_size;
                    thsn_slice_advance_unsafe(buffer_slice, step_size + 1);
                    *token = THSN_TOKEN_STRING;
                    return THSN_RESULT_SUCCESS;
                }
                // `+1` to step over the escaped quote
                const size_t step_size = (next_quotes - buffer_slice->data) + 1;
                token_slice->size += step_size;
                thsn_slice_advance_unsafe(buffer_slice, step_size);
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
            while (!done && thsn_slice_try_consume_char(buffer_slice, &c)) {
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
                        thsn_slice_rewind_unsafe(buffer_slice, 1);
                        --token_slice->size;
                        done = true;
                        break;
                }
            }
            /* token_slice->size is at least 1 here */
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
