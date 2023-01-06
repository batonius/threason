#include "parser.h"
#include "stdatomic.h"
#include "threads.h"

typedef struct {
    ThsnSlice object_slice;
    size_t value_offset;
} ThsnPreparsedValue;

typedef struct {
    /* Completion flag */
    volatile atomic_bool completed;
    /* Thread inputs */
    ThsnSlice buffer_slice;
    /* Thread outputs */
    struct {
        ThsnOwningSlice preparsed_table;
        ThsnOwningSlice parse_result;
        bool failed;
    } parsing_results[2];
} ThsnThreadContext;

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

static void thsn_advance_after_end_of_string(ThsnSlice* buffer_slice) {
    while (true) {
        const char* const next_quotes =
            memchr(buffer_slice->data, '"', buffer_slice->size);
        if (next_quotes == NULL) {
            thsn_slice_advance_unsafe(buffer_slice, buffer_slice->size);
            break;
        }

        if (next_quotes == buffer_slice->data) {
            thsn_slice_advance_unsafe(buffer_slice, 1);
            break;
        }

        bool escaped = false;
        for (const char* slash_iterator = next_quotes - 1;
             slash_iterator >= buffer_slice->data && *slash_iterator == '\\';
             --slash_iterator) {
            escaped = !escaped;
        }

        if (!escaped) {
            const size_t step_size = next_quotes - buffer_slice->data;
            thsn_slice_advance_unsafe(buffer_slice, step_size + 1);
            break;
        }
        // `+1` to step over the escaped quote
        const size_t step_size = (next_quotes - buffer_slice->data) + 1;
        thsn_slice_advance_unsafe(buffer_slice, step_size);
    }
}

int thsn_preparse_thread(void* user_data) {
    ThsnThreadContext* thread_context = (ThsnThreadContext*)user_data;
    if (thsn_preparse_buffer(
            thread_context->buffer_slice,
            &thread_context->parsing_results[0].parse_result,
            &thread_context->parsing_results[0].preparsed_table) !=
        THSN_RESULT_SUCCESS) {
        thread_context->parsing_results[0].failed = true;
    }

    ThsnSlice buffer_slice = thread_context->buffer_slice;
    thsn_advance_after_end_of_string(&buffer_slice);
    if (thsn_preparse_buffer(
            thread_context->buffer_slice,
            &thread_context->parsing_results[1].parse_result,
            &thread_context->parsing_results[1].preparsed_table) !=
        THSN_RESULT_SUCCESS) {
        thread_context->parsing_results[1].failed = true;
    }

    atomic_store_explicit(&thread_context->completed, true,
                          memory_order_release);
    return 0;
}
