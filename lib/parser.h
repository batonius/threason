#ifndef THSN_PARSER_H
#define THSN_PARSER_H

#include "threason.h"
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
} ThsnParserContext;

ThsnResult thsn_parser_context_init(ThsnParserContext* parser_context);

ThsnResult thsn_parser_context_free(ThsnParserContext* parser_context);

ThsnResult thsn_parser_parse_next_token(ThsnParserContext* parser_context,
                                        ThsnToken token, ThsnSlice token_slice,
                                        ThsnVector* result_vector,
                                        bool* finished);

#endif
