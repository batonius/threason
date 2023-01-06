#include "parser.h"
#include "stdatomic.h"
#include "threads.h"

typedef struct {
    ThsnSlice inbuffer_slice;
    size_t value_offset;
} ThsnPreparsedValue;

typedef enum {
    THSN_PREPARSE_STARTS_NOT_IN_STRING = 0,
    THSN_PREPARSE_STARTS_IN_STRING = 1
} ThsnPreparseScenario;

typedef struct {
    /* Completion flag */
    volatile atomic_bool completed;
    /* Thread inputs */
    ThsnSlice subbuffer_slice;
    uint8_t chunk_no;
    /* Thread outputs */
    struct {
        ThsnOwningSlice preparsed_table;
        ThsnOwningSlice parse_result;
        bool failed;
    } parsing_results[2];
} ThsnThreadContext;

typedef struct {
    ThsnSlice buffer_slice;
    ThsnOwningSlice parse_result;
    ThsnSlice preparse_thread_contexts;
} ThsnMainThreadContext;

typedef struct {
    ThsnSlice thread_contexts;
    ThsnThreadContext current_thread_context;
    ThsnSlice current_pp_table;
    ThsnPreparsedValue current_pp_value;
} ThsnPreparseIterator;

static ThsnPreparsedValue thsn_pp_value_make_empty() {
    return (ThsnPreparsedValue){.inbuffer_slice = thsn_slice_make_empty(),
                                .value_offset = 0};
}

static bool thsn_pp_value_is_empty(const ThsnPreparsedValue* pp_value) {
    return thsn_slice_is_empty(pp_value->inbuffer_slice);
}

static ThsnResult thsn_pp_iter_init(ThsnPreparseIterator* pp_iter,
                                    ThsnSlice thread_contexts) {
    *pp_iter = (ThsnPreparseIterator){0};
    pp_iter->thread_contexts = thread_contexts;
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(pp_iter->thread_contexts,
                                      pp_iter->current_thread_context));
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_pp_iter_advance_to_char(ThsnPreparseIterator* pp_iter,
                                               const char* point,
                                               bool in_string) {
    if (point <
        thsn_slice_end(pp_iter->current_thread_context.subbuffer_slice)) {
        return THSN_RESULT_SUCCESS;
    }

    while (true) {
        if (thsn_slice_is_empty(pp_iter->thread_contexts)) {
            /* No more thread contexts */
            pp_iter->current_pp_table = thsn_slice_make_empty();
            pp_iter->current_pp_value = thsn_pp_value_make_empty();
            return THSN_RESULT_SUCCESS;
        }
        BAIL_ON_ERROR(THSN_SLICE_READ_VAR(pp_iter->thread_contexts,
                                          pp_iter->current_thread_context));
        if (point <
            thsn_slice_end(pp_iter->current_thread_context.subbuffer_slice)) {
            break;
        }
    }
    // We have new current_thread_conext here, wait for it
    while (true) {
        if (atomic_load_explicit(&pp_iter->current_thread_context.completed,
                                 memory_order_acquire)) {
            break;
        } else {
            thrd_yield();
        }
    }
    size_t results_offset = in_string ? THSN_PREPARSE_STARTS_IN_STRING
                                      : THSN_PREPARSE_STARTS_NOT_IN_STRING;
    if (pp_iter->current_thread_context.parsing_results[results_offset]
            .failed) {
        return THSN_RESULT_INPUT_ERROR;
    }
    pp_iter->current_pp_table =
        pp_iter->current_thread_context.parsing_results[results_offset]
            .preparsed_table;
    if (thsn_slice_is_empty(pp_iter->current_pp_table)) {
        pp_iter->current_pp_value = thsn_pp_value_make_empty();
    } else {
        BAIL_ON_ERROR(THSN_SLICE_READ_VAR(pp_iter->current_pp_table,
                                          pp_iter->current_pp_value));
    }
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_pp_iter_str_token(ThsnPreparseIterator* pp_iter,
                                         ThsnSlice str_token_slice) {
    return thsn_pp_iter_advance_to_char(
        pp_iter, thsn_slice_end(str_token_slice) + 1, true);
}

static ThsnResult thsn_pp_iter_find_value_at(ThsnPreparseIterator* pp_iter,
                                             const char* point,
                                             ThsnValueHandle* value_handle,
                                             size_t* inbuffer_value_size) {
    *value_handle = THSN_VALUE_HANDLE_NOT_FOUND;
    *inbuffer_value_size = 0;
    BAIL_ON_ERROR(thsn_pp_iter_advance_to_char(pp_iter, point, false));
    if (thsn_pp_value_is_empty(&pp_iter->current_pp_value)) {
        return THSN_RESULT_SUCCESS;
    }
    while (true) {
        if (point < pp_iter->current_pp_value.inbuffer_slice.data) {
            return THSN_RESULT_SUCCESS;
        }
        if (point == pp_iter->current_pp_value.inbuffer_slice.data) {
            *value_handle = (ThsnValueHandle){
                .chunk_no = pp_iter->current_thread_context.chunk_no,
                pp_iter->current_pp_value.value_offset};
            *inbuffer_value_size =
                pp_iter->current_pp_value.inbuffer_slice.size;
            return THSN_RESULT_SUCCESS;
        }
        if (thsn_slice_is_empty(pp_iter->current_pp_table)) {
            return THSN_RESULT_SUCCESS;
        }
        BAIL_ON_ERROR(THSN_SLICE_READ_VAR(pp_iter->current_pp_table,
                                          pp_iter->current_pp_value));
    }

    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_preparse_buffer(ThsnSlice buffer_slice,
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
        ThsnPreparsedValue preparsed_value = {.inbuffer_slice = buffer_slice};
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
        preparsed_value.inbuffer_slice.size =
            buffer_slice.data - preparsed_value.inbuffer_slice.data;
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

static int thsn_preparse_thread(void* user_data) {
    ThsnThreadContext* thread_context = (ThsnThreadContext*)user_data;
    if (thsn_preparse_buffer(
            thread_context->subbuffer_slice,
            &thread_context->parsing_results[0].parse_result,
            &thread_context->parsing_results[0].preparsed_table) !=
        THSN_RESULT_SUCCESS) {
        thread_context->parsing_results[0].failed = true;
    }

    ThsnSlice buffer_slice = thread_context->subbuffer_slice;
    thsn_advance_after_end_of_string(&buffer_slice);
    if (thsn_preparse_buffer(
            thread_context->subbuffer_slice,
            &thread_context->parsing_results[1].parse_result,
            &thread_context->parsing_results[1].preparsed_table) !=
        THSN_RESULT_SUCCESS) {
        thread_context->parsing_results[1].failed = true;
    }

    atomic_store_explicit(&thread_context->completed, true,
                          memory_order_release);
    return 0;
}

static int thsn_main_thread(void* user_data) {
    ThsnMainThreadContext* main_thread_context =
        (ThsnMainThreadContext*)user_data;
    ThsnPreparseIterator pp_iter;
    BAIL_ON_ERROR(thsn_pp_iter_init(
        &pp_iter, main_thread_context->preparse_thread_contexts));
    ThsnSlice buffer_slice = main_thread_context->buffer_slice;
    ThsnParserContext parser_context;
    BAIL_ON_ERROR(thsn_parser_context_init(&parser_context));
    ThsnToken token;
    ThsnSlice token_slice;
    bool finished = false;
    while (!finished) {
        ThsnValueHandle value_handle = THSN_VALUE_HANDLE_NOT_FOUND;
        size_t inbuffer_value_size = 0;

        GOTO_ON_ERROR(thsn_next_token(&buffer_slice, &token_slice, &token),
                      error_cleanup);
        switch (token) {
            case THSN_TOKEN_STRING:
                GOTO_ON_ERROR(thsn_pp_iter_str_token(&pp_iter, token_slice),
                              error_cleanup);
                break;
            case THSN_TOKEN_OPEN_BRACE:
            case THSN_TOKEN_OPEN_BRACKET:
                GOTO_ON_ERROR(thsn_pp_iter_find_value_at(
                                  &pp_iter, token_slice.data, &value_handle,
                                  &inbuffer_value_size),
                              error_cleanup);
                break;
            default:
                break;
        }

        if (THSN_VALUE_HANDLE_IS_NOT_FOUND(value_handle)) {
            GOTO_ON_ERROR(thsn_parser_parse_next_token(&parser_context, token,
                                                       token_slice, &finished),
                          error_cleanup);
        } else {
            GOTO_ON_ERROR(
                thsn_parser_add_value_handle(&parser_context, value_handle),
                error_cleanup);
            GOTO_ON_ERROR(
                thsn_slice_at_offset(buffer_slice,
                                     inbuffer_value_size - token_slice.size, 0,
                                     &buffer_slice),
                error_cleanup);
        }
    }
    BAIL_ON_ERROR(thsn_parser_context_finish(
        &parser_context, &main_thread_context->parse_result));
    return 0;
error_cleanup:
    thsn_parser_context_finish(&parser_context, NULL);
    return 0;
}

ThsnResult thsn_parse_thread_per_chunk(ThsnSlice* json_str_slice,
                                       ThsnParsedJson* /*out*/ parsed_json) {
    const size_t threads_count = parsed_json->chunks_count;
    ThsnVector threads = thsn_vector_make_empty();
    ThsnThreadContext* thread_contexts = aligned_alloc(
        _Alignof(ThsnThreadContext), sizeof(ThsnThreadContext) * threads_count);
    thread_contexts[0] = (ThsnThreadContext){0};
    const size_t subbuffer_size = json_str_slice->size / threads_count;
    size_t current_offset =
        json_str_slice->size - subbuffer_size * threads_count;
    for (size_t i = 1; i < threads_count; ++i) {
        thread_contexts[i] = (ThsnThreadContext){0};
        thread_contexts[i].completed = false;
        thread_contexts[i].chunk_no = i;
        BAIL_ON_ERROR(thsn_slice_at_offset(
            *json_str_slice, current_offset, subbuffer_size,
            &thread_contexts[i].subbuffer_slice));
        BAIL_ON_ERROR(thsn_slice_truncate(&thread_contexts[i].subbuffer_slice,
                                          subbuffer_size));
        thrd_t thread;
        thrd_create(&thread, thsn_preparse_thread, &thread_contexts[i]);
        BAIL_ON_ERROR(THSN_VECTOR_PUSH_VAR(threads, thread));
    }
    ThsnMainThreadContext main_thread_context = {
        .preparse_thread_contexts =
            thsn_slice_make((const char*)thread_contexts,
                            sizeof(ThsnThreadContext) * threads_count),
        .buffer_slice = *json_str_slice,
        .parse_result = thsn_slice_make_empty()};
    thrd_t main_thread;
    thrd_create(&main_thread, thsn_main_thread, &main_thread_context);
    BAIL_ON_ERROR(THSN_VECTOR_PUSH_VAR(threads, main_thread));
    return THSN_RESULT_SUCCESS;
}
