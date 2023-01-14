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

ThsnResult thsn_parser_parse_next_token(
    ThsnParserContext* /*mut*/ parser_context, ThsnToken token,
    ThsnSlice token_slice, bool* /*out*/ finished);

static inline ThsnResult thsn_parser_context_init(
    ThsnParserContext* /*out*/ parser_context) {
    BAIL_ON_NULL_INPUT(parser_context);
    parser_context->state = THSN_PARSER_STATE_VALUE;
    BAIL_ON_ERROR(thsn_vector_allocate(&parser_context->stack, 1024));
    BAIL_ON_ERROR(thsn_vector_allocate(&parser_context->segment, 1024));
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
    if (parser_context->state != THSN_PARSER_STATE_VALUE) {
        return THSN_RESULT_INPUT_ERROR;
    }
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

#endif
