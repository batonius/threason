#include "parser.h"
#include "tags.h"
#include "tokenizer.h"

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
    if (!THSN_SLICE_EMPTY(slice)) {
        bool sign = true;
        char c = THSN_SLICE_NEXT_CHAR_UNSAFE(slice);
        if (c == '-') {
            if (THSN_SLICE_EMPTY(slice)) {
                return 0;
            }
            sign = false;
        } else {
            result = c - '0';
        }

        while (!THSN_SLICE_EMPTY(slice)) {
            c = THSN_SLICE_NEXT_CHAR_UNSAFE(slice);
            result = result * 10 + (c - '0');
        }

        if (!sign) {
            result *= -1;
        }
    }
    return result;
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
        case THSN_TOKEN_FLOAT:
            // TODO
            return THSN_RESULT_INPUT_ERROR;
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

static ThsnResult thsn_parser_parse_first_array_element(
    ThsnToken token, ThsnSlice token_slice, ThsnParserStatus* parser_status,
    ThsnVector* result_vector) {
    if (token == THSN_TOKEN_CLOSED_BRACKET) {
        parser_status->state = THSN_PARSER_STATE_FINISH;
        return thsn_vector_store_tagged_value(
            result_vector, THSN_TAG_MAKE(THSN_TAG_ARRAY, THSN_TAG_SIZE_EMPTY),
            THSN_SLICE_MAKE_EMPTY());
    }
    // Save the array tag and reserve two size_t for number of elements
    // and an offset to a elements offsets table
    size_t array_header_size = sizeof(ThsnTag) + 2 * sizeof(size_t);
    size_t array_header_offset = THSN_VECTOR_OFFSET(*result_vector);
    BAIL_ON_ERROR(thsn_vector_grow(result_vector, array_header_size));
    char* array_header =
        THSN_VECTOR_AT_OFFSET(*result_vector, array_header_offset);
    ThsnTag array_tag = THSN_TAG_MAKE(THSN_TAG_ARRAY, THSN_TAG_SIZE_INBOUND);
    memcpy(array_header, &array_tag, sizeof(array_tag));
    array_header_offset += sizeof(array_tag);
    // Push offset to array header, offset to the first element, number of
    // elements, the next parser state
    BAIL_ON_ERROR(
        THSN_VECTOR_PUSH_VAR(parser_status->stack, array_header_offset));
    size_t first_element_offset = THSN_VECTOR_OFFSET(*result_vector);
    BAIL_ON_ERROR(
        THSN_VECTOR_PUSH_VAR(parser_status->stack, first_element_offset));
    size_t array_elements_count = 1;
    BAIL_ON_ERROR(
        THSN_VECTOR_PUSH_VAR(parser_status->stack, array_elements_count));
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
    size_t array_elements_count;
    BAIL_ON_ERROR(
        THSN_VECTOR_POP_VAR(parser_status->stack, array_elements_count));
    if (token == THSN_TOKEN_CLOSED_BRACKET) {
        size_t elements_offsets_table_size =
            array_elements_count * sizeof(size_t);
        BAIL_ON_ERROR(thsn_vector_shrink(&parser_status->stack,
                                         elements_offsets_table_size));
        size_t elements_offsets_table_offset =
            THSN_VECTOR_OFFSET(*result_vector);
        BAIL_ON_ERROR(thsn_vector_push(
            result_vector,
            THSN_SLICE_MAKE(THSN_VECTOR_AT_CURRENT_OFFSET(parser_status->stack),
                            elements_offsets_table_size)));
        size_t array_header_offset;
        BAIL_ON_ERROR(
            THSN_VECTOR_POP_VAR(parser_status->stack, array_header_offset));
        char* array_header =
            THSN_VECTOR_AT_OFFSET(*result_vector, array_header_offset);
        memcpy(array_header, &array_elements_count,
               sizeof(array_elements_count));
        array_header += sizeof(array_elements_count);
        memcpy(array_header, &elements_offsets_table_offset,
               sizeof(elements_offsets_table_offset));
        parser_status->state = THSN_PARSER_STATE_FINISH;
        return THSN_RESULT_SUCCESS;
    }
    if (token != THSN_TOKEN_COMMA) {
        return THSN_RESULT_INPUT_ERROR;
    }
    size_t result_offset = THSN_VECTOR_OFFSET(*result_vector);
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_VAR(parser_status->stack, result_offset));
    ++array_elements_count;
    BAIL_ON_ERROR(
        THSN_VECTOR_PUSH_VAR(parser_status->stack, array_elements_count));
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
    size_t prev_node_offset;
    BAIL_ON_ERROR(THSN_VECTOR_POP_VAR(parser_status->stack, prev_node_offset));
    size_t cur_node_offset = THSN_VECTOR_OFFSET(*result_vector);
    char* prev_node = THSN_VECTOR_AT_OFFSET(*result_vector, prev_node_offset);
    memcpy(prev_node, &cur_node_offset, sizeof(cur_node_offset));
    prev_node += sizeof(cur_node_offset);
    memcpy(prev_node, &cur_node_offset, sizeof(cur_node_offset));
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_VAR(parser_status->stack, cur_node_offset));
    BAIL_ON_ERROR(thsn_vector_grow(result_vector, 2 * sizeof(size_t)));
    memset(THSN_VECTOR_AT_CURRENT_OFFSET(*result_vector) - 2 * sizeof(size_t),
           0, 2 * sizeof(size_t));
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
            result_vector, THSN_TAG_MAKE(THSN_TAG_OBJECT, THSN_TAG_SIZE_EMPTY),
            THSN_SLICE_MAKE_EMPTY());
    }
    size_t object_header_size = sizeof(ThsnTag) + 2 * sizeof(size_t);
    size_t object_header_offset = THSN_VECTOR_OFFSET(*result_vector);
    BAIL_ON_ERROR(thsn_vector_grow(result_vector, object_header_size));
    char* object_header =
        THSN_VECTOR_AT_OFFSET(*result_vector, object_header_offset);
    ThsnTag object_tag = THSN_TAG_MAKE(THSN_TAG_OBJECT, THSN_TAG_SIZE_INBOUND);
    memcpy(object_header, &object_tag, sizeof(object_tag));
    object_header_offset += sizeof(object_tag);
    BAIL_ON_ERROR(
        THSN_VECTOR_PUSH_VAR(parser_status->stack, object_header_offset));
    return thsn_parser_parse_next_kv(token, token_slice, parser_status,
                                     result_vector);
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
        size_t prev_node_offset;
        BAIL_ON_ERROR(
            THSN_VECTOR_POP_VAR(parser_status->stack, prev_node_offset));
        parser_status->state = THSN_PARSER_STATE_FINISH;
        return THSN_RESULT_SUCCESS;
    }
    if (token != THSN_TOKEN_COMMA) {
        return THSN_RESULT_INPUT_ERROR;
    }
    parser_status->state = THSN_PARSER_STATE_NEXT_KV;
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_parse_value(ThsnSlice* /*in/out*/ buffer_slice,
                            ThsnVector* /*in/out*/ result_vector) {
    BAIL_ON_NULL_INPUT(buffer_slice);
    BAIL_ON_NULL_INPUT(result_vector);
    ThsnSlice token_slice;
    ThsnToken token;
    ThsnParserStatus parser_status = {.state = THSN_PARSER_STATE_VALUE};
    BAIL_ON_ERROR(thsn_vector_make(&parser_status.stack, 1024));

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
            if (THSN_VECTOR_EMPTY(parser_status.stack)) {
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
