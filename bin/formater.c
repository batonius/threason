#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "threason.h"

ThsnVisitorResult visit_number(const ThsnVisitorContext* context,
                               void* user_data, double value) {
    (void)user_data;
    if (context->in_object) {
        printf("\"%.*s\": ", (int)context->key.size, context->key.data);
    }
    printf("%g", value);
    if ((context->in_array || context->in_object) && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_null(const ThsnVisitorContext* context,
                             void* user_data) {
    (void)user_data;
    if (context->in_object) {
        printf("\"%.*s\": ", (int)context->key.size, context->key.data);
    }
    printf("null");
    if ((context->in_array || context->in_object) && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_bool(const ThsnVisitorContext* context, void* user_data,
                             bool value) {
    (void)user_data;
    if (context->in_object) {
        printf("\"%.*s\": ", (int)context->key.size, context->key.data);
    }
    printf("%s", value ? "true" : "false");
    if ((context->in_array || context->in_object) && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_string(const ThsnVisitorContext* context,
                               void* user_data, ThsnSlice value) {
    (void)user_data;
    if (context->in_object) {
        printf("\"%.*s\": ", (int)context->key.size, context->key.data);
    }
    printf("\"%.*s\"", (int)value.size, value.data);
    if ((context->in_array || context->in_object) && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_array_start(const ThsnVisitorContext* context,
                                    void* user_data) {
    (void)user_data;
    if (context->in_object) {
        printf("\"%.*s\": ", (int)context->key.size, context->key.data);
    }
    printf("[");
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_array_end(const ThsnVisitorContext* context,
                                  void* user_data) {
    (void)user_data;
    printf("]");
    if ((context->in_array || context->in_object) && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_object_start(const ThsnVisitorContext* context,
                                     void* user_data) {
    (void)user_data;
    if (context->in_object) {
        printf("\"%.*s\": ", (int)context->key.size, context->key.data);
    }
    printf("{");
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_object_end(const ThsnVisitorContext* context,
                                   void* user_data) {
    (void)user_data;
    printf("}");
    if ((context->in_array || context->in_object) && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
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

    if (argc > 1) {
        file = fopen(argv[1], "rb");
    }
    char* json_str = NULL;
    size_t json_str_len = 0;
    char buffer[1024];
    do {
        size_t read_len = fread(buffer, 1, 1024, file);
        if (read_len == 0) {
            break;
        }
        json_str = realloc(json_str, json_str_len + read_len);
        memcpy(json_str + json_str_len, buffer, read_len);
        json_str_len += read_len;
    } while (true);
    fprintf(stderr, "Read %zu bytes.\n", json_str_len);
    ThsnSlice input_slice = {.data = json_str, .size = json_str_len};
    ThsnParsedJson parsed_json;

    if (thsn_parse_json(&input_slice, &parsed_json) != THSN_RESULT_SUCCESS) {
        fprintf(stderr, "Can't parse input string\n");
        return 1;
    }
    fprintf(stderr, "Parse result size: %zu\n", parsed_json.size);
    if (thsn_visit(parsed_json, &visitor_vtable, NULL) != THSN_RESULT_SUCCESS) {
        fprintf(stderr, "\nCan't visit parse result\n");
        goto error_cleanup;
    }
    printf("\n");
    thsn_free_parsed_json(&parsed_json);
    return 0;

error_cleanup:
    thsn_free_parsed_json(&parsed_json);
    return 1;
}
