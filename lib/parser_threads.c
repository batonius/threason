#include <stdio.h>

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
    /* Thread inputs */
    ThsnSlice subbuffer_slice;
    uint8_t chunk_no;
    /* Thread outputs */
    struct {
        ThsnOwningSlice pp_table;
        ThsnOwningMutSlice parse_result;
        bool failed;
        /* Completion flag */
        volatile atomic_bool completed;
    } parsing_results[2];
    ThsnPreparseScenario pp_scenario;
} ThsnThreadContext;

typedef struct {
    ThsnSlice thread_contexts;
    ThsnThreadContext* current_thread_context;
    ThsnOwningSlice current_pp_table;
    ThsnPreparsedValue current_pp_value;
} ThsnPreparseIterator;

static ThsnPreparsedValue thsn_pp_value_make_empty() {
    return (ThsnPreparsedValue){.inbuffer_slice = thsn_slice_make_empty(),
                                .value_offset = 0};
}

static bool thsn_pp_value_is_empty(const ThsnPreparsedValue* /*in*/ pp_value) {
    BAIL_ON_NULL_INPUT(pp_value);
    return thsn_slice_is_empty(pp_value->inbuffer_slice);
}

static ThsnResult thsn_pp_iter_init(ThsnPreparseIterator* /*mut*/ pp_iter,
                                    ThsnSlice thread_contexts) {
    BAIL_ON_NULL_INPUT(pp_iter);
    *pp_iter = (ThsnPreparseIterator){0};
    pp_iter->thread_contexts = thread_contexts;
    if (!thsn_slice_is_empty(pp_iter->thread_contexts)) {
        pp_iter->current_thread_context =
            (ThsnThreadContext*)pp_iter->thread_contexts.data;
        BAIL_ON_ERROR(thsn_slice_at_offset(pp_iter->thread_contexts,
                                           sizeof(ThsnThreadContext), 0,
                                           &pp_iter->thread_contexts));
    } else {
        return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_pp_iter_advance_to_char(
    ThsnPreparseIterator* /*mut*/ pp_iter, const char* /*in*/ point,
    bool in_string) {
    BAIL_ON_NULL_INPUT(pp_iter);
    BAIL_ON_NULL_INPUT(point);
    if (point <
        thsn_slice_end(pp_iter->current_thread_context->subbuffer_slice)) {
        return THSN_RESULT_SUCCESS;
    }

    while (true) {
        if (thsn_slice_is_empty(pp_iter->thread_contexts)) {
            /* No more thread contexts */
            pp_iter->current_pp_table = thsn_slice_make_empty();
            pp_iter->current_pp_value = thsn_pp_value_make_empty();
            return THSN_RESULT_SUCCESS;
        }
        pp_iter->current_thread_context =
            (ThsnThreadContext*)pp_iter->thread_contexts.data;
        BAIL_ON_ERROR(thsn_slice_at_offset(pp_iter->thread_contexts,
                                           sizeof(ThsnThreadContext), 0,
                                           &pp_iter->thread_contexts));
        if (point <
            thsn_slice_end(pp_iter->current_thread_context->subbuffer_slice)) {
            break;
        }
    }
    ThsnPreparseScenario results_offset =
        in_string ? THSN_PREPARSE_STARTS_IN_STRING
                  : THSN_PREPARSE_STARTS_NOT_IN_STRING;
    // We have new current_thread_conext here, wait for it
#ifdef METRICS
    size_t yield_count = 0;
#endif
    while (true) {
        if (atomic_load_explicit(&pp_iter->current_thread_context
                                      ->parsing_results[results_offset]
                                      .completed,
                                 memory_order_acquire)) {
            break;
        } else {
            thrd_yield();
#ifdef METRICS
            ++yield_count;
#endif
        }
    }
#ifdef METRICS
    fprintf(stderr, "Yelded %zu times\n", yield_count);
#endif
    pp_iter->current_thread_context->pp_scenario = results_offset;
    if (pp_iter->current_thread_context->parsing_results[results_offset]
            .failed) {
        return THSN_RESULT_INPUT_ERROR;
    }
    pp_iter->current_pp_table =
        pp_iter->current_thread_context->parsing_results[results_offset]
            .pp_table;
    if (thsn_slice_is_empty(pp_iter->current_pp_table)) {
        pp_iter->current_pp_value = thsn_pp_value_make_empty();
    } else {
        BAIL_ON_ERROR(THSN_SLICE_READ_VAR(pp_iter->current_pp_table,
                                          pp_iter->current_pp_value));
    }
    return THSN_RESULT_SUCCESS;
}

static ThsnResult thsn_pp_iter_str_token(ThsnPreparseIterator* /*mut*/ pp_iter,
                                         ThsnSlice str_token_slice) {
    return thsn_pp_iter_advance_to_char(
        pp_iter, thsn_slice_end(str_token_slice) + 1, true);
}

static ThsnResult thsn_pp_iter_find_value_at(
    ThsnPreparseIterator* /*mut*/ pp_iter, const char* /*in*/ point,
    ThsnValueHandle* /*out*/ value_handle,
    size_t* /*out*/ inbuffer_value_size) {
    BAIL_ON_NULL_INPUT(pp_iter);
    BAIL_ON_NULL_INPUT(point);
    BAIL_ON_NULL_INPUT(value_handle);
    BAIL_ON_NULL_INPUT(inbuffer_value_size);

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
                .chunk_no = pp_iter->current_thread_context->chunk_no,
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

static void thsn_advance_after_end_of_string(ThsnSlice* /*mut*/ buffer_slice) {
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

static ThsnResult thsn_preparse_buffer(
    ThsnSlice buffer_slice, ThsnOwningMutSlice* /*out*/ parse_result,
    ThsnOwningSlice* /*out*/ preparsed_table) {
    BAIL_ON_NULL_INPUT(parse_result);
    BAIL_ON_NULL_INPUT(preparsed_table);

    ThsnVector preparsed_vector = thsn_vector_make_empty();
    BAIL_ON_ERROR(thsn_vector_allocate(&preparsed_vector, 1024));
    ThsnParserContext parser_context;
    BAIL_ON_ERROR(thsn_parser_context_init(&parser_context));
#ifdef METRICS
    size_t total_preparsed = 0;
#endif
    /* TODO: try switching single-pass walkthrough */
    while (true) {
        ThsnToken token;
        ThsnSlice token_slice;
        /* Tokenizing/parsing failures are ok since the buffer isn't
              expected to be well-formed */
        do {
            GOTO_ON_ERROR(thsn_next_token(&buffer_slice, &token_slice, &token),
                          success_cleanup);
            if (token == THSN_TOKEN_EOF) {
                goto success_cleanup;
            }
        } while (token != THSN_TOKEN_OPEN_BRACE &&
                 token != THSN_TOKEN_OPEN_BRACKET);
        ThsnPreparsedValue preparsed_value = {.inbuffer_slice.data =
                                                  token_slice.data};
        GOTO_ON_ERROR(thsn_parser_next_value_offset(
                          &parser_context, &preparsed_value.value_offset),
                      error_cleanup);
        bool finished = false;
        GOTO_ON_ERROR(thsn_parser_parse_next_token(&parser_context, token,
                                                   token_slice, &finished),
                      success_cleanup);
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
#ifdef METRICS
        total_preparsed += preparsed_value.inbuffer_slice.size;
#endif
        GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(preparsed_vector, preparsed_value),
                      error_cleanup);
        GOTO_ON_ERROR(thsn_parser_reset_state(&parser_context), error_cleanup);
    }
success_cleanup:
#ifdef METRICS
    fprintf(stderr, "Total preparsed %zu, skipped %zu\n", total_preparsed,
            buffer_slice.size);
#endif
    thsn_parser_context_finish(&parser_context, parse_result);
    *preparsed_table = thsn_vector_as_slice(preparsed_vector);
    return THSN_RESULT_SUCCESS;
error_cleanup:
    thsn_parser_context_finish(&parser_context, NULL);
    thsn_vector_free(&preparsed_vector);
    return THSN_RESULT_INPUT_ERROR;
}

static int thsn_preparse_thread(void* /*in*/ user_data) {
    if (user_data == NULL) {
        /* TODO: report failure */
        return 0;
    }

    ThsnThreadContext* thread_context = (ThsnThreadContext*)user_data;

    ThsnSlice subbuffer_slice = thread_context->subbuffer_slice;
    thsn_advance_after_end_of_string(&subbuffer_slice);
    if (thsn_preparse_buffer(
            subbuffer_slice,
            &thread_context->parsing_results[THSN_PREPARSE_STARTS_IN_STRING]
                 .parse_result,
            &thread_context->parsing_results[THSN_PREPARSE_STARTS_IN_STRING]
                 .pp_table) != THSN_RESULT_SUCCESS) {
        thread_context->parsing_results[THSN_PREPARSE_STARTS_IN_STRING].failed =
            true;
    }
    atomic_store_explicit(
        &thread_context->parsing_results[THSN_PREPARSE_STARTS_IN_STRING]
             .completed,
        true, memory_order_release);

    if (thsn_preparse_buffer(
            thread_context->subbuffer_slice,
            &thread_context->parsing_results[THSN_PREPARSE_STARTS_NOT_IN_STRING]
                 .parse_result,
            &thread_context->parsing_results[THSN_PREPARSE_STARTS_NOT_IN_STRING]
                 .pp_table) != THSN_RESULT_SUCCESS) {
        thread_context->parsing_results[THSN_PREPARSE_STARTS_NOT_IN_STRING]
            .failed = true;
    }
    atomic_store_explicit(
        &thread_context->parsing_results[THSN_PREPARSE_STARTS_NOT_IN_STRING]
             .completed,
        true, memory_order_release);

    return 0;
}

static ThsnResult thsn_main_thread(ThsnSlice* /*mut*/ buffer_slice,
                                   ThsnOwningMutSlice* /*out*/ parse_result,
                                   ThsnSlice preparse_thread_contexts) {
    BAIL_ON_NULL_INPUT(buffer_slice);
    BAIL_ON_NULL_INPUT(parse_result);
    ThsnPreparseIterator pp_iter;
    BAIL_ON_ERROR(thsn_pp_iter_init(&pp_iter, preparse_thread_contexts));
    ThsnParserContext parser_context;
    BAIL_ON_ERROR(thsn_parser_context_init(&parser_context));
    ThsnToken token;
    ThsnSlice token_slice;
    bool finished = false;
#ifdef METRICS
    size_t total_skipped = 0;
#endif
    while (!finished) {
        ThsnValueHandle value_handle = THSN_VALUE_HANDLE_NOT_FOUND;
        size_t inbuffer_value_size = 0;

        GOTO_ON_ERROR(thsn_next_token(buffer_slice, &token_slice, &token),
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
                thsn_slice_at_offset(*buffer_slice,
                                     inbuffer_value_size - token_slice.size, 0,
                                     buffer_slice),
                error_cleanup);
#ifdef METRICS
            total_skipped += inbuffer_value_size - token_slice.size;
#endif
        }
    }
    BAIL_ON_ERROR(thsn_parser_context_finish(&parser_context, parse_result));
#ifdef METRICS
    fprintf(stderr, "Total skipped %zu, total parsed %zu\n", total_skipped,
            main_thread_context->buffer_slice.size - total_skipped);
#endif
    return THSN_RESULT_SUCCESS;
error_cleanup:
    thsn_parser_context_finish(&parser_context, NULL);
    return THSN_RESULT_INPUT_ERROR;
}

ThsnResult thsn_parse_thread_per_chunk(ThsnSlice* /*mut*/ json_str_slice,
                                       ThsnParsedJson* /*out*/ parsed_json) {
    BAIL_ON_NULL_INPUT(json_str_slice);
    BAIL_ON_NULL_INPUT(parsed_json);
    BAIL_WITH_INPUT_ERROR_UNLESS(parsed_json->chunks_count >= 1);
    size_t threads_count = parsed_json->chunks_count;
    if (json_str_slice->size / 32 < threads_count) {
        threads_count = json_str_slice->size / 32;
    }
    ThsnThreadContext* thread_contexts = aligned_alloc(
        _Alignof(ThsnThreadContext), sizeof(ThsnThreadContext) * threads_count);
    thread_contexts[0] = (ThsnThreadContext){0};
    size_t subbuffer_size = json_str_slice->size / threads_count;
    size_t current_offset =
        json_str_slice->size - subbuffer_size * (threads_count - 1);
    GOTO_ON_ERROR(thsn_slice_at_offset(*json_str_slice, 0, current_offset,
                                       &thread_contexts[0].subbuffer_slice),
                  error_cleanup);
    GOTO_ON_ERROR(thsn_slice_truncate(&thread_contexts[0].subbuffer_slice,
                                      current_offset),
                  error_cleanup);
    fprintf(stderr, "Thread 0: buffer size %zu\n", current_offset);
    for (size_t i = 1; i < threads_count; ++i) {
        /* TODO detach threads */
        thread_contexts[i] = (ThsnThreadContext){0};
        thread_contexts[i].chunk_no = i;
        const size_t buffer_left = json_str_slice->size - current_offset;
        if (buffer_left == 0) {
            break;
        } else if (buffer_left <= subbuffer_size) {
            subbuffer_size = buffer_left;
        }
        fprintf(stderr, "Thread %zu: buffer size %zu\n", i, subbuffer_size);
        GOTO_ON_ERROR(thsn_slice_at_offset(*json_str_slice, current_offset,
                                           subbuffer_size,
                                           &thread_contexts[i].subbuffer_slice),
                      error_cleanup);
        GOTO_ON_ERROR(thsn_slice_truncate(&thread_contexts[i].subbuffer_slice,
                                          subbuffer_size),
                      error_cleanup);
        thrd_t thread;
        thrd_create(&thread, thsn_preparse_thread, &thread_contexts[i]);
        thrd_detach(thread);
        current_offset += subbuffer_size;
    }
    ThsnOwningMutSlice parse_result;
    GOTO_ON_ERROR(thsn_main_thread(json_str_slice, &parse_result,
                                   thsn_slice_make((const char*)thread_contexts,
                                                   sizeof(ThsnThreadContext) *
                                                       threads_count)),
                  error_cleanup);
    /* Fill in results */
    parsed_json->chunks[0] = parse_result;
    for (size_t i = 1; i < parsed_json->chunks_count; ++i) {
        if (thread_contexts[i].pp_scenario == THSN_PREPARSE_STARTS_IN_STRING) {
            parsed_json->chunks[i] =
                thread_contexts[i]
                    .parsing_results[THSN_PREPARSE_STARTS_IN_STRING]
                    .parse_result;
            free(thread_contexts[i]
                     .parsing_results[THSN_PREPARSE_STARTS_NOT_IN_STRING]
                     .parse_result.data);
        } else {
            parsed_json->chunks[i] =
                thread_contexts[i]
                    .parsing_results[THSN_PREPARSE_STARTS_NOT_IN_STRING]
                    .parse_result;
            free(thread_contexts[i]
                     .parsing_results[THSN_PREPARSE_STARTS_IN_STRING]
                     .parse_result.data);
        }
        free((void*)thread_contexts[i]
                 .parsing_results[THSN_PREPARSE_STARTS_IN_STRING]
                 .pp_table.data);
        free((void*)thread_contexts[i]
                 .parsing_results[THSN_PREPARSE_STARTS_NOT_IN_STRING]
                 .pp_table.data);
    }
    free(thread_contexts);
    return THSN_RESULT_SUCCESS;
error_cleanup:
    free(thread_contexts);
    return THSN_RESULT_INPUT_ERROR;
}
