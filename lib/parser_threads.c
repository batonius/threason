#include "parser.h"
#include "threads.h"

typedef struct {
    ThsnSlice object_slice;
    size_t value_offset;
} ThsnPreparsedValue;

typedef struct {
    /* mutex */
    /* Thread inputs */
    ThsnSlice buffer_slice;
    /* Thread outputs */
    ThsnOwningSlice preparsed_table;
    ThsnOwningSlice parse_result;
    /* */
    char padding[128];
} ThsnThreadResut;

ThsnResult thsn_preparse_buffer(ThsnSlice buffer_slice,
                                ThsnOwningSlice* parse_result,
                                ThsnOwningSlice* preparsed_table) {
    ThsnParserContext parser_context;
    ThsnVector preparsed_vector = {0};
    BAIL_ON_ERROR(thsn_parser_context_init(&parser_context));
    char c;
    while (true) {
        /* Fast-forwarding to the next meaningful value */
        while (thsn_slice_try_consume_char(&buffer_slice, &c)) {
            if (c == '{' || c == '\"' || c == '[') {
                thsn_slice_rewind_unsafe(&buffer_slice, 1);
                break;
            }
        }
        if (thsn_slice_is_empty(buffer_slice)) {
            goto success_cleanup;
        }
        ThsnPreparsedValue preparsed_value = {.object_slice = buffer_slice};
        GOTO_ON_ERROR(thsn_parser_next_value_offset(
                          &parser_context, &preparsed_value.value_offset),
                      error_cleanup);
        ThsnToken token;
        ThsnSlice token_slice;
        bool finished = false;
        while (!finished) {
            /* Tokenizing/parsing failures are ok since the buffer isn't
              expected to be well-formed */
            GOTO_ON_ERROR(thsn_next_token(&buffer_slice, &token_slice, &token),
                          success_cleanup);
            GOTO_ON_ERROR(thsn_parser_parse_next_token(&parser_context, token,
                                                       token_slice, &finished),
                          success_cleanup);
        }
        preparsed_value.object_slice.size =
            buffer_slice.data - preparsed_value.object_slice.data;
        GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(preparsed_vector, preparsed_value),
                      error_cleanup);
        GOTO_ON_ERROR(thsn_parser_reset_state(&parser_context), error_cleanup);
    }
success_cleanup:
    thsn_parser_context_finish(&parser_context, parse_result);
    *preparsed_table = thsn_vector_as_slice(preparsed_vector);
    return THSN_RESULT_SUCCESS;
error_cleanup:
    thsn_parser_context_finish(&parser_context, NULL);
    thsn_vector_free(&preparsed_vector);
    return THSN_RESULT_INPUT_ERROR;
}
