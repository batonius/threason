#include <stdio.h>

#include "parser.h"
#include "visitor.h"

thsn_visitor_result_t visit_integer(thsn_visitor_context_t* context,
                                    void* user_data, long long value) {
    (void)user_data;
    printf("%lld", value);
    if (context->in_array && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

thsn_visitor_result_t visit_float(thsn_visitor_context_t* context,
                                  void* user_data, double value) {
    (void)user_data;
    printf("%g", value);
    if (context->in_array && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}
thsn_visitor_result_t visit_null(thsn_visitor_context_t* context,
                                 void* user_data) {
    (void)user_data;
    printf("null");
    if (context->in_array && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

thsn_visitor_result_t visit_bool(thsn_visitor_context_t* context,
                                 void* user_data, bool value) {
    (void)user_data;
    printf("%s", value ? "true" : "false");
    if (context->in_array && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

thsn_visitor_result_t visit_string(thsn_visitor_context_t* context,
                                   void* user_data, thsn_slice_t value) {
    (void)user_data;
    printf("\"%.*s\"", (int)value.size, value.data);
    if (context->in_array && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

thsn_visitor_result_t visit_array_start(thsn_visitor_context_t* context,
                                        void* user_data) {
    (void)user_data;
    (void)context;
    printf("[");
    return THSN_VISITOR_RESULT_CONTINUE;
}

thsn_visitor_result_t visit_array_end(thsn_visitor_context_t* context,
                                      void* user_data) {
    (void)user_data;
    (void)context;
    printf("]");
    if (context->in_array && !context->last) {
        printf(", ");
    }
    return THSN_VISITOR_RESULT_CONTINUE;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    thsn_visitor_vtable_t visitor_vtable = {
        .visit_null = visit_null,
        .visit_bool = visit_bool,
        .visit_integer = visit_integer,
        .visit_float = visit_float,
        .visit_string = visit_string,
        .visit_array_start = visit_array_start,
        .visit_array_end = visit_array_end,
    };

    thsn_vector_t parse_result = THSN_VECTOR_INIT();
    if (thsn_vector_make(&parse_result, 1024) != THSN_RESULT_SUCCESS) {
        printf("Can't allocate parse result storage\n");
        return 1;
    }
    thsn_slice_t input_slice = THSN_SLICE_FROM_C_STR(argv[1]);
    if (thsn_parse_value(&input_slice, &parse_result) != THSN_RESULT_SUCCESS) {
        printf("Can't parse input string\n");
        goto error_cleanup;
    }
    if (thsn_visit(THSN_VECTOR_AS_SLICE(parse_result), &visitor_vtable, NULL) !=
        THSN_RESULT_SUCCESS) {
        printf("Can't visit parse result\n");
        goto error_cleanup;
    }
    printf("\n");
    thsn_vector_free(&parse_result);
    return 0;

error_cleanup:
    thsn_vector_free(&parse_result);
    return 1;
}
