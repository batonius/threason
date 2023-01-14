#ifndef THSN_PARSER_H
#define THSN_PARSER_H

#include "segment.h"
#include "threason.h"
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
    ThsnSegment segment;
} ThsnParserContext;

static inline ThsnResult thsn_parser_context_init(
    ThsnParserContext* /*out*/ parser_context) {
    BAIL_ON_NULL_INPUT(parser_context);
    parser_context->state = THSN_PARSER_STATE_VALUE;
    BAIL_ON_ERROR(thsn_vector_allocate(&parser_context->stack, 1024 * 1024));
    BAIL_ON_ERROR(thsn_vector_allocate(&parser_context->segment, 1024 * 1024));
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_context_finish(
    ThsnParserContext* /*in*/ parser_context,
    ThsnOwningMutSlice* /*maybe out*/ parsing_result) {
    BAIL_ON_NULL_INPUT(parser_context);
    thsn_vector_free(&parser_context->stack);
    if (parsing_result == NULL) {
        thsn_vector_free(&parser_context->segment);
    } else {
        *parsing_result = thsn_vector_as_mut_slice(parser_context->segment);
    }
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_add_value_handle(
    ThsnParserContext* /*mut*/ parser_context, ThsnValueHandle value_handle) {
    BAIL_ON_NULL_INPUT(parser_context);
    BAIL_WITH_INPUT_ERROR_UNLESS(parser_context->state ==
                                 THSN_PARSER_STATE_VALUE);
    BAIL_ON_ERROR(thsn_segment_store_value_handle(&parser_context->segment,
                                                  value_handle));
    BAIL_ON_ERROR(
        THSN_VECTOR_POP_VAR(parser_context->stack, parser_context->state));
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_reset_state(
    ThsnParserContext* /*mut*/ parser_context) {
    BAIL_ON_NULL_INPUT(parser_context);
    parser_context->state = THSN_PARSER_STATE_VALUE;
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_next_value_offset(
    const ThsnParserContext* /*in*/ parser_context,
    size_t* /*out*/ next_offset) {
    BAIL_ON_NULL_INPUT(parser_context);
    BAIL_ON_NULL_INPUT(next_offset);
    *next_offset = thsn_vector_current_offset(parser_context->segment);
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_document_allocate(ThsnDocument** /*mut*/ document,
                                                uint8_t chunks_count) {
    BAIL_ON_NULL_INPUT(document);
    *document =
        calloc(1, sizeof(ThsnDocument) + sizeof(ThsnSlice) * chunks_count);
    if (*document == NULL) {
        return THSN_RESULT_OUT_OF_MEMORY_ERROR;
    }
    (*document)->segment_count = chunks_count;
    return THSN_RESULT_SUCCESS;
}

static inline long long thsn_parser_atoll_checked(ThsnSlice slice) {
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

static inline ThsnResult thsn_parser_atod_checked(ThsnSlice slice,
                                                  double* /*out*/ result) {
    BAIL_ON_NULL_INPUT(result);
    const size_t MAX_DOUBLE_LEN = 128;
    BAIL_WITH_INPUT_ERROR_UNLESS(slice.size <= MAX_DOUBLE_LEN);
    char double_str[MAX_DOUBLE_LEN + 1];
    memcpy(double_str, slice.data, slice.size);
    double_str[slice.size] = '\0';
    char* double_str_end = NULL;
    *result = strtod(double_str, &double_str_end);
    return double_str_end == double_str ? THSN_RESULT_INPUT_ERROR
                                        : THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_parse_value(
    ThsnToken token, ThsnSlice token_slice,
    ThsnParserContext* /*mut*/ parser_context) {
    BAIL_ON_NULL_INPUT(parser_context);
    parser_context->state = THSN_PARSER_STATE_FINISH;
    switch (token) {
        case THSN_TOKEN_EOF:
            return THSN_RESULT_SUCCESS;
        case THSN_TOKEN_NULL:
            return thsn_segment_store_null(&parser_context->segment);
        case THSN_TOKEN_TRUE:
            return thsn_segment_store_bool(&parser_context->segment, true);
        case THSN_TOKEN_FALSE:
            return thsn_segment_store_bool(&parser_context->segment, false);
        case THSN_TOKEN_INT:
            return thsn_segment_store_int(
                &parser_context->segment,
                thsn_parser_atoll_checked(token_slice));
        case THSN_TOKEN_FLOAT: {
            double value;
            BAIL_ON_ERROR(thsn_parser_atod_checked(token_slice, &value));
            return thsn_segment_store_double(&parser_context->segment, value);
        }
        case THSN_TOKEN_STRING:
            return thsn_segment_store_string(&parser_context->segment,
                                             token_slice);
        case THSN_TOKEN_OPEN_BRACKET:
            parser_context->state = THSN_PARSER_STATE_FIRST_ARRAY_ELEMENT;
            return THSN_RESULT_SUCCESS;
        case THSN_TOKEN_OPEN_BRACE:
            parser_context->state = THSN_PARSER_STATE_FIRST_KV;
            return THSN_RESULT_SUCCESS;
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
}

static inline ThsnResult thsn_parser_store_composite_header(
    ThsnParserContext* /*mut*/ parser_context, ThsnTag tag,
    bool with_sorted_table_offset) {
    BAIL_ON_NULL_INPUT(parser_context);
    size_t composite_header_offset;
    BAIL_ON_ERROR(thsn_segment_store_composite_header(
        &parser_context->segment, tag, with_sorted_table_offset,
        &composite_header_offset));
    const size_t first_element_offset =
        thsn_vector_current_offset(parser_context->segment);
    const size_t composite_elements_count = 1;
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_3_VARS(
        parser_context->stack, composite_header_offset, first_element_offset,
        composite_elements_count));
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_add_composite_element(
    ThsnParserContext* /*mut*/ parser_context) {
    BAIL_ON_NULL_INPUT(parser_context);
    size_t array_elements_count;
    BAIL_ON_ERROR(
        THSN_VECTOR_POP_VAR(parser_context->stack, array_elements_count));
    const size_t result_offset =
        thsn_vector_current_offset(parser_context->segment);
    ++array_elements_count;
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_2_VARS(parser_context->stack, result_offset,
                                          array_elements_count));
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_store_composite_elements_table(
    ThsnParserContext* /*mut*/ parser_context, bool reserve_sorted_table) {
    BAIL_ON_NULL_INPUT(parser_context);
    size_t composite_elements_count;
    BAIL_ON_ERROR(
        THSN_VECTOR_POP_VAR(parser_context->stack, composite_elements_count));
    const size_t elements_offsets_table_size =
        composite_elements_count * sizeof(size_t);
    ThsnSlice elements_table_src;
    BAIL_ON_ERROR(thsn_vector_shrink(&parser_context->stack,
                                     elements_offsets_table_size,
                                     &elements_table_src));
    size_t composite_header_offset;
    BAIL_ON_ERROR(
        THSN_VECTOR_POP_VAR(parser_context->stack, composite_header_offset));
    return thsn_segment_store_composite_elements_table(
        &parser_context->segment, composite_header_offset, elements_table_src,
        reserve_sorted_table);
}

static inline ThsnResult thsn_parser_parse_first_array_element(
    ThsnToken token, ThsnSlice token_slice,
    ThsnParserContext* /*mut*/ parser_context) {
    BAIL_ON_NULL_INPUT(parser_context);
    if (token == THSN_TOKEN_CLOSED_BRACKET) {
        parser_context->state = THSN_PARSER_STATE_FINISH;
        return thsn_segment_store_tagged_value(
            &parser_context->segment,
            thsn_tag_make(THSN_TAG_ARRAY, THSN_TAG_SIZE_EMPTY),
            thsn_slice_make_empty());
    }
    BAIL_ON_ERROR(thsn_parser_store_composite_header(
        parser_context, thsn_tag_make(THSN_TAG_ARRAY, THSN_TAG_SIZE_INBOUND),
        false));
    ThsnParserState return_to_state = THSN_PARSER_STATE_NEXT_ARRAY_ELEMENT;
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_VAR(parser_context->stack, return_to_state));
    return thsn_parser_parse_value(token, token_slice, parser_context);
}

static inline ThsnResult thsn_parser_parse_next_array_element(
    ThsnToken token, ThsnSlice token_slice,
    ThsnParserContext* /*mut*/ parser_context) {
    BAIL_ON_NULL_INPUT(parser_context);
    (void)token_slice;
    if (token == THSN_TOKEN_CLOSED_BRACKET) {
        BAIL_ON_ERROR(thsn_parser_store_composite_elements_table(
            parser_context, /* reserve_sorted_table = */ false));
        parser_context->state = THSN_PARSER_STATE_FINISH;
        return THSN_RESULT_SUCCESS;
    }
    BAIL_WITH_INPUT_ERROR_UNLESS(token == THSN_TOKEN_COMMA);
    BAIL_ON_ERROR(thsn_parser_add_composite_element(parser_context));
    ThsnParserState return_to_state = THSN_PARSER_STATE_NEXT_ARRAY_ELEMENT;
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_VAR(parser_context->stack, return_to_state));
    parser_context->state = THSN_PARSER_STATE_VALUE;
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_parse_next_kv(
    ThsnToken token, ThsnSlice token_slice,
    ThsnParserContext* /*mut*/ parser_context) {
    BAIL_ON_NULL_INPUT(parser_context);
    BAIL_WITH_INPUT_ERROR_UNLESS(token == THSN_TOKEN_STRING);
    BAIL_ON_ERROR(thsn_parser_add_composite_element(parser_context));
    BAIL_ON_ERROR(
        thsn_segment_store_string(&parser_context->segment, token_slice));
    parser_context->state = THSN_PARSER_STATE_KV_COLON;
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_parse_first_kv(
    ThsnToken token, ThsnSlice token_slice,
    ThsnParserContext* /*mut*/ parser_context) {
    BAIL_ON_NULL_INPUT(parser_context);
    if (token == THSN_TOKEN_CLOSED_BRACE) {
        parser_context->state = THSN_PARSER_STATE_FINISH;
        return thsn_segment_store_tagged_value(
            &parser_context->segment,
            thsn_tag_make(THSN_TAG_OBJECT, THSN_TAG_SIZE_EMPTY),
            thsn_slice_make_empty());
    }
    BAIL_WITH_INPUT_ERROR_UNLESS(token == THSN_TOKEN_STRING);
    BAIL_ON_ERROR(thsn_parser_store_composite_header(
        parser_context, thsn_tag_make(THSN_TAG_OBJECT, THSN_TAG_SIZE_INBOUND),
        true));
    BAIL_ON_ERROR(
        thsn_segment_store_string(&parser_context->segment, token_slice));
    parser_context->state = THSN_PARSER_STATE_KV_COLON;
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_parse_kv_colon(
    ThsnToken token, ThsnSlice token_slice,
    ThsnParserContext* /*mut*/ parser_context) {
    BAIL_ON_NULL_INPUT(parser_context);
    (void)token_slice;
    BAIL_WITH_INPUT_ERROR_UNLESS(token == THSN_TOKEN_COLON);
    const ThsnParserState return_to_state = THSN_PARSER_STATE_KV_END;
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_VAR(parser_context->stack, return_to_state));
    parser_context->state = THSN_PARSER_STATE_VALUE;
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_parse_kv_end(
    ThsnToken token, ThsnSlice token_slice,
    ThsnParserContext* /*mut*/ parser_context) {
    BAIL_ON_NULL_INPUT(parser_context);
    (void)token_slice;
    if (token == THSN_TOKEN_CLOSED_BRACE) {
        BAIL_ON_ERROR(thsn_parser_store_composite_elements_table(
            parser_context, /*reserve_sorted_table=*/true));
        parser_context->state = THSN_PARSER_STATE_FINISH;
        return THSN_RESULT_SUCCESS;
    }
    BAIL_WITH_INPUT_ERROR_UNLESS(token == THSN_TOKEN_COMMA);
    parser_context->state = THSN_PARSER_STATE_NEXT_KV;
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_parser_parse_next_token(
    ThsnParserContext* /*mut*/ parser_context, ThsnToken token,
    ThsnSlice token_slice, bool* /*out*/ finished) {
    BAIL_ON_NULL_INPUT(parser_context);
    BAIL_ON_NULL_INPUT(finished);
    *finished = false;
    switch (parser_context->state) {
        case THSN_PARSER_STATE_VALUE:
            BAIL_ON_ERROR(
                thsn_parser_parse_value(token, token_slice, parser_context));
            break;
        case THSN_PARSER_STATE_FIRST_ARRAY_ELEMENT:
            BAIL_ON_ERROR(thsn_parser_parse_first_array_element(
                token, token_slice, parser_context));
            break;
        case THSN_PARSER_STATE_NEXT_ARRAY_ELEMENT:
            BAIL_ON_ERROR(thsn_parser_parse_next_array_element(
                token, token_slice, parser_context));
            break;
        case THSN_PARSER_STATE_FIRST_KV:
            BAIL_ON_ERROR(
                thsn_parser_parse_first_kv(token, token_slice, parser_context));
            break;
        case THSN_PARSER_STATE_KV_COLON:
            BAIL_ON_ERROR(
                thsn_parser_parse_kv_colon(token, token_slice, parser_context));
            break;
        case THSN_PARSER_STATE_KV_END:
            BAIL_ON_ERROR(
                thsn_parser_parse_kv_end(token, token_slice, parser_context));
            break;
        case THSN_PARSER_STATE_NEXT_KV:
            BAIL_ON_ERROR(
                thsn_parser_parse_next_kv(token, token_slice, parser_context));
            break;
        case THSN_PARSER_STATE_FINISH:
            return THSN_RESULT_INPUT_ERROR;
    }
    if (parser_context->state == THSN_PARSER_STATE_FINISH) {
        if (thsn_vector_is_empty(parser_context->stack)) {
            *finished = true;
        } else {
            BAIL_ON_ERROR(THSN_VECTOR_POP_VAR(parser_context->stack,
                                              parser_context->state));
        }
    }
    return THSN_RESULT_SUCCESS;
}

#endif
