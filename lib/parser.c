#include <stdlib.h>

#include "result.h"
#include "slice.h"
#include "tags.h"
#include "tokenizer.h"
#include "vector.h"

typedef enum {
    THSN_PARSER_STATE_VALUE,
    THSN_PARSER_STATE_FINISH,
    THSN_PARSER_STATE_FIRST_ARRAY_ELEMENT,
    THSN_PARSER_STATE_NEXT_ARRAY_ELEMENT,
    THSN_PARSER_STATE_FIRST_KV,
    THSN_PARSER_STATE_KV_COLON,
    THSN_PARSER_STATE_KV_END,
    THSN_PARSER_STATE_NEXT_KV,
} ThsnParserState;

typedef struct {
    ThsnParserState state;
    ThsnVector stack;
} ThsnParserStatus;

static long long thsn_atoll_checked(ThsnSlice slice) {
    long long result = 0;
    char c;
    if (thsn_slice_try_consume_char(&slice, &c)) {
        bool sign = true;
        if (c == '-') {
            if (thsn_slice_is_empty(slice)) {
                return 0;
            }
            sign = false;
        } else {
            result = c - '0';
        }

        while (thsn_slice_try_consume_char(&slice, &c)) {
            result = result * 10 + (c - '0');
        }

        if (!sign) {
            result *= -1;
        }
    }
    return result;
}

static ThsnResult thsn_atod_checked(ThsnSlice slice, double* result) {
    BAIL_ON_NULL_INPUT(result);
    const size_t MAX_DOUBLE_LEN = 128;
    if (slice.size > MAX_DOUBLE_LEN) {
        return THSN_RESULT_INPUT_ERROR;
    }
    char double_str[MAX_DOUBLE_LEN + 1];
    memcpy(double_str, slice.data, slice.size);
    double_str[slice.size] = '\0';
    char* double_str_end = NULL;
    *result = strtod(double_str, &double_str_end);
    return double_str_end == double_str ? THSN_RESULT_INPUT_ERROR
                                        : THSN_RESULT_SUCCESS;
}

static int thsn_parser_compare_kv_keys(const void* a, const void* b,
                                       void* data) {
    size_t a_offset;
    size_t b_offset;
    memcpy(&a_offset, a, sizeof(size_t));
    memcpy(&b_offset, b, sizeof(size_t));
    ThsnVector* result_vector = (ThsnVector*)data;
    ThsnSlice a_key_slice;
    ThsnSlice b_key_slice;
    if (thsn_slice_at_offset(thsn_vector_as_slice(*result_vector), a_offset, 1,
                             &a_key_slice) != THSN_RESULT_SUCCESS) {
        return 0;
    }
    if (thsn_slice_at_offset(thsn_vector_as_slice(*result_vector), b_offset, 1,
                             &b_key_slice) != THSN_RESULT_SUCCESS) {
        return 0;
    }
    ThsnSlice a_key_str_slice;
    ThsnSlice b_key_str_slice;
    if (thsn_slice_read_string(a_key_slice, &a_key_str_slice, NULL)) {
        return 0;
    }
    if (thsn_slice_read_string(b_key_slice, &b_key_str_slice, NULL)) {
        return 0;
    }
    const size_t min_len = a_key_str_slice.size < b_key_str_slice.size
                               ? a_key_str_slice.size
                               : b_key_str_slice.size;
    const int cmp_result =
        memcmp(a_key_str_slice.data, b_key_str_slice.data, min_len);
    if (cmp_result == 0) {
        if (a_key_str_slice.size == b_key_str_slice.size) {
            return 0;
        } else if (a_key_str_slice.size < b_key_str_slice.size) {
            return -1;
        } else {
            return 1;
        }
    } else {
        return cmp_result;
    }
}

static ThsnResult thsn_parser_sort_elements_table(ThsnMutSlice elements_table,
                                                  ThsnVector* result_vector) {
    qsort_r(elements_table.data, elements_table.size / sizeof(size_t),
            sizeof(size_t), thsn_parser_compare_kv_keys, result_vector);
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_parser_parse_value(ThsnToken token,
                                          ThsnSlice token_slice,
                                          ThsnParserStatus* parser_status,
                                          ThsnVector* result_vector) {
    parser_status->state = THSN_PARSER_STATE_FINISH;
    switch (token) {
        case THSN_TOKEN_EOF:
            return THSN_RESULT_SUCCESS;
        case THSN_TOKEN_NULL:
            return thsn_vector_store_null(result_vector);
        case THSN_TOKEN_TRUE:
            return thsn_vector_store_bool(result_vector, true);
        case THSN_TOKEN_FALSE:
            return thsn_vector_store_bool(result_vector, false);
        case THSN_TOKEN_INT:
            return thsn_vector_store_int(result_vector,
                                         thsn_atoll_checked(token_slice));
        case THSN_TOKEN_FLOAT: {
            double value;
            BAIL_ON_ERROR(thsn_atod_checked(token_slice, &value));
            return thsn_vector_store_double(result_vector, value);
        }
        case THSN_TOKEN_STRING:
            return thsn_vector_store_string(result_vector, token_slice);
        case THSN_TOKEN_OPEN_BRACKET:
            parser_status->state = THSN_PARSER_STATE_FIRST_ARRAY_ELEMENT;
            return THSN_RESULT_SUCCESS;
        case THSN_TOKEN_OPEN_BRACE:
            parser_status->state = THSN_PARSER_STATE_FIRST_KV;
            return THSN_RESULT_SUCCESS;
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
}

static ThsnResult thsn_parser_store_composite_header(
    ThsnParserStatus* parser_status, ThsnVector* result_vector, ThsnTag tag) {
    const size_t composite_header_size = sizeof(ThsnTag) + 2 * sizeof(size_t);
    size_t composite_header_offset = thsn_vector_current_offset(*result_vector);
    ThsnMutSlice composite_header_mut_slice;
    BAIL_ON_ERROR(thsn_vector_grow(result_vector, composite_header_size,
                                   &composite_header_mut_slice));
    BAIL_ON_ERROR(THSN_MUT_SLICE_WRITE_VAR(composite_header_mut_slice, tag));
    composite_header_offset += sizeof(tag);
    const size_t first_element_offset =
        thsn_vector_current_offset(*result_vector);
    const size_t composite_elements_count = 1;
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_3_VARS(
        parser_status->stack, composite_header_offset, first_element_offset,
        composite_elements_count));
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_parser_add_composite_element(
    ThsnParserStatus* parser_status, ThsnVector* result_vector) {
    size_t array_elements_count;
    BAIL_ON_ERROR(
        THSN_VECTOR_POP_VAR(parser_status->stack, array_elements_count));
    size_t result_offset = thsn_vector_current_offset(*result_vector);
    ++array_elements_count;
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_2_VARS(parser_status->stack, result_offset,
                                          array_elements_count));
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_parser_store_composite_elements_table(
    ThsnParserStatus* parser_status, ThsnVector* result_vector,
    bool sort_as_kv) {
    size_t composite_elements_count;
    BAIL_ON_ERROR(
        THSN_VECTOR_POP_VAR(parser_status->stack, composite_elements_count));
    size_t elements_offsets_table_size =
        composite_elements_count * sizeof(size_t);

    ThsnSlice elements_table_src;
    BAIL_ON_ERROR(thsn_vector_shrink(&parser_status->stack,
                                     elements_offsets_table_size,
                                     &elements_table_src));
    ThsnMutSlice elements_table_dst;
    size_t elements_offsets_table_offset =
        thsn_vector_current_offset(*result_vector);
    BAIL_ON_ERROR(thsn_vector_grow(result_vector, elements_offsets_table_size,
                                   &elements_table_dst));
    BAIL_ON_ERROR(thsn_slice_read(&elements_table_src, elements_table_dst));
    if (sort_as_kv) {
        BAIL_ON_ERROR(
            thsn_parser_sort_elements_table(elements_table_dst, result_vector));
    }
    size_t composite_header_offset;
    BAIL_ON_ERROR(
        THSN_VECTOR_POP_VAR(parser_status->stack, composite_header_offset));
    ThsnMutSlice composite_header;
    BAIL_ON_ERROR(thsn_vector_mut_slice_at_offset(
        *result_vector, composite_header_offset,
        sizeof(composite_elements_count) +
            sizeof(elements_offsets_table_offset),
        &composite_header));
    BAIL_ON_ERROR(
        THSN_MUT_SLICE_WRITE_VAR(composite_header, composite_elements_count));
    BAIL_ON_ERROR(THSN_MUT_SLICE_WRITE_VAR(composite_header,
                                           elements_offsets_table_offset));
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_parser_parse_first_array_element(
    ThsnToken token, ThsnSlice token_slice, ThsnParserStatus* parser_status,
    ThsnVector* result_vector) {
    if (token == THSN_TOKEN_CLOSED_BRACKET) {
        parser_status->state = THSN_PARSER_STATE_FINISH;
        return thsn_vector_store_tagged_value(
            result_vector, thsn_tag_make(THSN_TAG_ARRAY, THSN_TAG_SIZE_EMPTY),
            thsn_slice_make_empty());
    }
    BAIL_ON_ERROR(thsn_parser_store_composite_header(
        parser_status, result_vector,
        thsn_tag_make(THSN_TAG_ARRAY, THSN_TAG_SIZE_INBOUND)));
    ThsnParserState return_to_state = THSN_PARSER_STATE_NEXT_ARRAY_ELEMENT;
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_VAR(parser_status->stack, return_to_state));
    return thsn_parser_parse_value(token, token_slice, parser_status,
                                   result_vector);
}

static ThsnResult thsn_parser_parse_next_array_element(
    ThsnToken token, ThsnSlice token_slice, ThsnParserStatus* parser_status,
    ThsnVector* result_vector) {
    // From the top of the stack: number of element, (elements offsets)+,
    // header offset
    (void)token_slice;
    if (token == THSN_TOKEN_CLOSED_BRACKET) {
        BAIL_ON_ERROR(thsn_parser_store_composite_elements_table(
            parser_status, result_vector, /*sort_as_kv = */ false));
        parser_status->state = THSN_PARSER_STATE_FINISH;
        return THSN_RESULT_SUCCESS;
    }
    if (token != THSN_TOKEN_COMMA) {
        return THSN_RESULT_INPUT_ERROR;
    }
    BAIL_ON_ERROR(
        thsn_parser_add_composite_element(parser_status, result_vector));
    ThsnParserState return_to_state = THSN_PARSER_STATE_NEXT_ARRAY_ELEMENT;
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_VAR(parser_status->stack, return_to_state));
    parser_status->state = THSN_PARSER_STATE_VALUE;
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_parser_parse_next_kv(ThsnToken token,
                                            ThsnSlice token_slice,
                                            ThsnParserStatus* parser_status,
                                            ThsnVector* result_vector) {
    if (token != THSN_TOKEN_STRING) {
        return THSN_RESULT_INPUT_ERROR;
    }
    BAIL_ON_ERROR(
        thsn_parser_add_composite_element(parser_status, result_vector));
    BAIL_ON_ERROR(thsn_vector_store_string(result_vector, token_slice));
    parser_status->state = THSN_PARSER_STATE_KV_COLON;
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_parser_parse_first_kv(ThsnToken token,
                                             ThsnSlice token_slice,
                                             ThsnParserStatus* parser_status,
                                             ThsnVector* result_vector) {
    if (token == THSN_TOKEN_CLOSED_BRACE) {
        parser_status->state = THSN_PARSER_STATE_FINISH;
        return thsn_vector_store_tagged_value(
            result_vector, thsn_tag_make(THSN_TAG_OBJECT, THSN_TAG_SIZE_EMPTY),
            thsn_slice_make_empty());
    }
    if (token != THSN_TOKEN_STRING) {
        return THSN_RESULT_INPUT_ERROR;
    }
    BAIL_ON_ERROR(thsn_parser_store_composite_header(
        parser_status, result_vector,
        thsn_tag_make(THSN_TAG_OBJECT, THSN_TAG_SIZE_INBOUND)));
    BAIL_ON_ERROR(thsn_vector_store_string(result_vector, token_slice));
    parser_status->state = THSN_PARSER_STATE_KV_COLON;
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_parser_parse_kv_colon(ThsnToken token,
                                             ThsnSlice token_slice,
                                             ThsnParserStatus* parser_status,
                                             ThsnVector* result_vector) {
    (void)token_slice;
    (void)result_vector;
    if (token != THSN_TOKEN_COLON) {
        return THSN_RESULT_INPUT_ERROR;
    }
    ThsnParserState return_to_state = THSN_PARSER_STATE_KV_END;
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_VAR(parser_status->stack, return_to_state));
    parser_status->state = THSN_PARSER_STATE_VALUE;
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_parser_parse_kv_end(ThsnToken token,
                                           ThsnSlice token_slice,
                                           ThsnParserStatus* parser_status,
                                           ThsnVector* result_vector) {
    (void)token_slice;
    (void)result_vector;
    if (token == THSN_TOKEN_CLOSED_BRACE) {
        BAIL_ON_ERROR(thsn_parser_store_composite_elements_table(
            parser_status, result_vector, /*sort_as_kv=*/true));
        parser_status->state = THSN_PARSER_STATE_FINISH;
        return THSN_RESULT_SUCCESS;
    }
    if (token != THSN_TOKEN_COMMA) {
        return THSN_RESULT_INPUT_ERROR;
    }
    parser_status->state = THSN_PARSER_STATE_NEXT_KV;
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_parse_value(ThsnSlice* /*in/out*/ buffer_slice,
                                   ThsnVector* /*in/out*/ result_vector) {
    BAIL_ON_NULL_INPUT(buffer_slice);
    BAIL_ON_NULL_INPUT(result_vector);
    ThsnSlice token_slice;
    ThsnToken token;
    ThsnParserStatus parser_status = {.state = THSN_PARSER_STATE_VALUE};
    BAIL_ON_ERROR(thsn_vector_allocate(&parser_status.stack, 1024));

    while (true) {
        GOTO_ON_ERROR(thsn_next_token(buffer_slice, &token_slice, &token),
                      error_cleanup);
        switch (parser_status.state) {
            case THSN_PARSER_STATE_VALUE:
                GOTO_ON_ERROR(
                    thsn_parser_parse_value(token, token_slice, &parser_status,
                                            result_vector),
                    error_cleanup);
                break;
            case THSN_PARSER_STATE_FIRST_ARRAY_ELEMENT:
                GOTO_ON_ERROR(
                    thsn_parser_parse_first_array_element(
                        token, token_slice, &parser_status, result_vector),
                    error_cleanup);
                break;
            case THSN_PARSER_STATE_NEXT_ARRAY_ELEMENT:
                GOTO_ON_ERROR(
                    thsn_parser_parse_next_array_element(
                        token, token_slice, &parser_status, result_vector),
                    error_cleanup);
                break;
            case THSN_PARSER_STATE_FIRST_KV:
                GOTO_ON_ERROR(
                    thsn_parser_parse_first_kv(token, token_slice,
                                               &parser_status, result_vector),
                    error_cleanup);
                break;
            case THSN_PARSER_STATE_KV_COLON:
                GOTO_ON_ERROR(
                    thsn_parser_parse_kv_colon(token, token_slice,
                                               &parser_status, result_vector),
                    error_cleanup);
                break;
            case THSN_PARSER_STATE_KV_END:
                GOTO_ON_ERROR(
                    thsn_parser_parse_kv_end(token, token_slice, &parser_status,
                                             result_vector),
                    error_cleanup);
                break;
            case THSN_PARSER_STATE_NEXT_KV:
                GOTO_ON_ERROR(
                    thsn_parser_parse_next_kv(token, token_slice,
                                              &parser_status, result_vector),
                    error_cleanup);
                break;
                break;
            case THSN_PARSER_STATE_FINISH:
                goto error_cleanup;
        }
        if (parser_status.state == THSN_PARSER_STATE_FINISH) {
            if (thsn_vector_is_empty(parser_status.stack)) {
                break;
            }
            GOTO_ON_ERROR(
                THSN_VECTOR_POP_VAR(parser_status.stack, parser_status.state),
                error_cleanup);
        }
    }

    thsn_vector_free(&parser_status.stack);
    return THSN_RESULT_SUCCESS;
error_cleanup:
    thsn_vector_free(&parser_status.stack);
    return THSN_RESULT_INPUT_ERROR;
}

ThsnResult thsn_parse_json(ThsnSlice* /*in/out*/ buffer_slice,
                           ThsnSlice* /*out*/ parsed_json) {
    BAIL_ON_NULL_INPUT(buffer_slice);
    BAIL_ON_NULL_INPUT(parsed_json);
    ThsnVector result_vector = thsn_vector_make_empty();
    BAIL_ON_ERROR(thsn_parse_value(buffer_slice, &result_vector));
    *parsed_json = thsn_vector_as_slice(result_vector);
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_free_parsed_json(ThsnSlice* /*in/out*/ parsed_json) {
    BAIL_ON_NULL_INPUT(parsed_json);
    free((char*)parsed_json->data);
    *parsed_json = thsn_slice_make_empty();
    return THSN_RESULT_SUCCESS;
}
