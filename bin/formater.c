#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "threason.h"

void print_offset(size_t offset) {
    for (size_t i = 0; i < offset; ++i) {
        printf("  ");
    }
}

void print_prefix(const ThsnVisitorContext* context, size_t offset) {
    print_offset(offset);
    if (context->in_object) {
        printf("\"%.*s\": ", (int)context->key.size, context->key.data);
    }
}

void print_postfix(const ThsnVisitorContext* context) {
    if ((context->in_array || context->in_object) && !context->last) {
        printf(",");
    }
    printf("\n");
}

ThsnVisitorResult visit_number(const ThsnVisitorContext* context,
                               void* user_data, double value) {
    print_prefix(context, *(size_t*)user_data);
    printf("%g", value);
    print_postfix(context);
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_null(const ThsnVisitorContext* context,
                             void* user_data) {
    print_prefix(context, *(size_t*)user_data);
    printf("null");
    print_postfix(context);
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_bool(const ThsnVisitorContext* context, void* user_data,
                             bool value) {
    print_prefix(context, *(size_t*)user_data);
    printf("%s", value ? "true" : "false");
    print_postfix(context);
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_string(const ThsnVisitorContext* context,
                               void* user_data, ThsnSlice value) {
    print_prefix(context, *(size_t*)user_data);
    printf("\"%.*s\"", (int)value.size, value.data);
    print_postfix(context);
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_array_start(const ThsnVisitorContext* context,
                                    void* user_data) {
    print_prefix(context, *(size_t*)user_data);
    printf("[\n");
    (*(size_t*)user_data)++;
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_array_end(const ThsnVisitorContext* context,
                                  void* user_data) {
    (*(size_t*)user_data)--;
    print_offset(*(size_t*)user_data);
    printf("]");
    print_postfix(context);
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_object_start(const ThsnVisitorContext* context,
                                     void* user_data) {
    print_prefix(context, *(size_t*)user_data);
    printf("{\n");
    (*(size_t*)user_data)++;
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_object_end(const ThsnVisitorContext* context,
                                   void* user_data) {
    (*(size_t*)user_data)--;
    print_offset(*(size_t*)user_data);
    printf("}");
    print_postfix(context);
    return THSN_VISITOR_RESULT_CONTINUE;
}

int main(int argc, char** argv) {
    ThsnVisitorVTable visitor_vtable = {
        .visit_null = visit_null,
        .visit_bool = visit_bool,
        .visit_number = visit_number,
        .visit_string = visit_string,
        .visit_array_start = visit_array_start,
        .visit_array_end = visit_array_end,
        .visit_object_start = visit_object_start,
        .visit_object_end = visit_object_end,
    };

    FILE* file = stdin;

    int thread_count = 1;

    if (argc > 1) {
        thread_count = atoi(argv[1]);
    }

    if (thread_count == 0) {
        thread_count = 1;
    } else if (thread_count > 16) {
        thread_count = 16;
    }

    if (argc > 2) {
        file = fopen(argv[2], "rb");
    }
    char* json_str = NULL;
    size_t json_str_len = 0;
    char buffer[1024 * 1024];
    do {
        size_t read_len = fread(buffer, 1, 1024 * 1024, file);
        if (read_len == 0) {
            break;
        }
        json_str = realloc(json_str, json_str_len + read_len);
        memcpy(json_str + json_str_len, buffer, read_len);
        json_str_len += read_len;
    } while (true);
    fprintf(stderr, "Read %zu bytes.\n", json_str_len);
    ThsnSlice input_slice = {.data = json_str, .size = json_str_len};
    ThsnParsedJson* parsed_json;

    if (thsn_parsed_json_allocate(&parsed_json, thread_count) !=
        THSN_RESULT_SUCCESS) {
        fprintf(stderr, "Can't allocate_result\n");
        return 1;
    }

    ThsnResult parsing_result;

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    if (thread_count == 1) {
        parsing_result = thsn_parse_buffer(&input_slice, parsed_json);
    } else {
        parsing_result = thsn_parse_thread_per_chunk(&input_slice, parsed_json);
    }
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    end_time.tv_sec -= start_time.tv_sec;
    if (end_time.tv_nsec < start_time.tv_nsec) {
        --end_time.tv_sec;
        end_time.tv_nsec += 1000000000LL - start_time.tv_nsec;
    } else {
        end_time.tv_nsec -= start_time.tv_nsec;
    }
    fprintf(stderr, "Elapsed %luus\n", end_time.tv_nsec / 1000);

    if (parsing_result != THSN_RESULT_SUCCESS) {
        fprintf(stderr, "Can't parse input string\n");
        return 1;
    }
    fprintf(stderr, "Parse result size: ");
    for (size_t i = 0; i < parsed_json->chunks_count; ++i) {
        fprintf(stderr, "%zu, ", parsed_json->chunks[i].size);
    }
    fprintf(stderr, "\n");
    size_t offset = 0;
    if (thsn_visit(parsed_json, &visitor_vtable, (void*)&offset) !=
        THSN_RESULT_SUCCESS) {
        fprintf(stderr, "\nCan't visit parse result\n");
        goto error_cleanup;
    }
    printf("\n");
    thsn_parsed_json_free(&parsed_json);
    free(json_str);
    return 0;

error_cleanup:
    thsn_parsed_json_free(&parsed_json);
    free(json_str);
    return 1;
}
