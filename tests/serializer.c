#include <stdio.h>

#include "parser.h"
#include "visitor.h"

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

    ThsnVector input_data = thsn_vector_make_empty();
    if (thsn_vector_allocate(&input_data, 1024) != THSN_RESULT_SUCCESS) {
        fprintf(stderr, "Can't allocate input storage\n");
        goto error_cleanup;
    }
    char buffer[1024];
    while (true) {
        size_t read_result = fread(buffer, 1, sizeof(buffer), file);
        if (read_result == 0) {
            break;
        }
        if (thsn_vector_push(&input_data,
                             thsn_slice_make(buffer, read_result)) !=
            THSN_RESULT_SUCCESS) {
            fprintf(stderr, "Can't append to input storage\n");
            goto error_cleanup;
        }
    }

    ThsnVector parse_result = thsn_vector_make_empty();
    if (thsn_vector_allocate(&parse_result, 1024) != THSN_RESULT_SUCCESS) {
        fprintf(stderr, "Can't allocate parse result storage\n");
        goto error_cleanup;
    }
    ThsnSlice input_slice = thsn_vector_as_slice(input_data);
    if (thsn_parse_value(&input_slice, &parse_result) != THSN_RESULT_SUCCESS) {
        fprintf(stderr, "Can't parse input string\n");
        goto error_cleanup;
    }
    fprintf(stderr, "Parse result size: %zu\n",
            thsn_vector_current_offset(parse_result));
    if (thsn_visit(thsn_vector_as_slice(parse_result), &visitor_vtable, NULL) !=
        THSN_RESULT_SUCCESS) {
        fprintf(stderr, "\nCan't visit parse result\n");
        goto error_cleanup;
    }
    printf("\n");
    thsn_vector_free(&parse_result);
    thsn_vector_free(&input_data);
    return 0;

error_cleanup:
    thsn_vector_free(&parse_result);
    thsn_vector_free(&input_data);
    return 1;
}
