#include <stdio.h>

#include "parser.h"
#include "visitor.h"

ThsnVisitorResult visit_integer(ThsnVisitorContext* context, void* user_data,
                                long long value) {
    (void)user_data;
    if (context->in_object) {
        printf("\"%.*s\": ", (int)context->key.size, context->key.data);
    }
    printf("%lld", value);
    if ((context->in_array || context->in_object) && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_float(ThsnVisitorContext* context, void* user_data,
                              double value) {
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

ThsnVisitorResult visit_null(ThsnVisitorContext* context, void* user_data) {
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

ThsnVisitorResult visit_bool(ThsnVisitorContext* context, void* user_data,
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

ThsnVisitorResult visit_string(ThsnVisitorContext* context, void* user_data,
                               ThsnSlice value) {
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

ThsnVisitorResult visit_array_start(ThsnVisitorContext* context,
                                    void* user_data) {
    (void)user_data;
    if (context->in_object) {
        printf("\"%.*s\": ", (int)context->key.size, context->key.data);
    }
    printf("[");
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_array_end(ThsnVisitorContext* context,
                                  void* user_data) {
    (void)user_data;
    printf("]");
    if ((context->in_array || context->in_object) && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_object_start(ThsnVisitorContext* context,
                                     void* user_data) {
    (void)user_data;
    if (context->in_object) {
        printf("\"%.*s\": ", (int)context->key.size, context->key.data);
    }
    printf("{");
    return THSN_VISITOR_RESULT_CONTINUE;
}

ThsnVisitorResult visit_object_end(ThsnVisitorContext* context,
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
        .visit_integer = visit_integer,
        .visit_float = visit_float,
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

    ThsnVector input_data = THSN_VECTOR_INIT();
    if (thsn_vector_make(&input_data, 1024) != THSN_RESULT_SUCCESS) {
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
                             THSN_SLICE_MAKE(buffer, read_result)) !=
            THSN_RESULT_SUCCESS) {
            fprintf(stderr, "Can't append to input storage\n");
            goto error_cleanup;
        }
    }

    ThsnVector parse_result = THSN_VECTOR_INIT();
    if (thsn_vector_make(&parse_result, 1024) != THSN_RESULT_SUCCESS) {
        fprintf(stderr, "Can't allocate parse result storage\n");
        goto error_cleanup;
    }
    ThsnSlice input_slice = THSN_VECTOR_AS_SLICE(input_data);
    if (thsn_parse_value(&input_slice, &parse_result) != THSN_RESULT_SUCCESS) {
        fprintf(stderr, "Can't parse input string\n");
        goto error_cleanup;
    }
    fprintf(stderr, "Parse result size: %zu\n",
            THSN_VECTOR_OFFSET(parse_result));
    if (thsn_visit(THSN_VECTOR_AS_SLICE(parse_result), &visitor_vtable, NULL) !=
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
