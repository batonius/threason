#include "slice.h"
#include "value.h"
#include "vector.h"
#include "visitor.h"

typedef enum {
    THSN_VISIT_TAG_ARRAY,
    THSN_VISIT_TAG_OBJECT_KV,
} ThsnVisitTag;

static const ThsnVisitTag VISIT_TAG_ARRAY = THSN_VISIT_TAG_ARRAY;
static const ThsnVisitTag VISIT_TAG_OBJECT_KV = THSN_VISIT_TAG_OBJECT_KV;

ThsnResult thsn_visit(ThsnSlice parse_result, const ThsnVisitorVTable* vtable,
                      void* user_data) {
#define PROCESS_VISITOR_RESULT(result)              \
    do {                                            \
        switch ((result)) {                         \
            case THSN_VISITOR_RESULT_ABORT_SUCCESS: \
                goto success_cleanup;               \
            case THSN_VISITOR_RESULT_SKIP:          \
                skip = true;                        \
                break;                              \
            case THSN_VISITOR_RESULT_CONTINUE:      \
                break;                              \
            default:                                \
                goto error_cleanup;                 \
        }                                           \
    } while (0)

#define CALL_VISITOR2(visitor_fn, arg1, arg2)                   \
    do {                                                        \
        skip = false;                                           \
        if (visitor_fn != NULL) {                               \
            PROCESS_VISITOR_RESULT(visitor_fn((arg1), (arg2))); \
        }                                                       \
    } while (0)

#define CALL_VISITOR3(visitor_fn, arg1, arg2, arg3)                     \
    do {                                                                \
        skip = false;                                                   \
        if (visitor_fn != NULL) {                                       \
            PROCESS_VISITOR_RESULT(visitor_fn((arg1), (arg2), (arg3))); \
        }                                                               \
    } while (0)

    if (thsn_slice_is_empty(parse_result)) {
        return THSN_RESULT_SUCCESS;
    }

    ThsnVisitorContext context = {
        .key = thsn_slice_make_empty(),
        .in_array = false,
        .last = false,
    };

    ThsnVector stack = thsn_vector_make_empty();
    BAIL_ON_ERROR(thsn_vector_allocate(&stack, 1024));
    bool skip = false;
    ThsnValueHandle current_value_handle = THSN_VALUE_HANDLE_FIRST;
    bool done_visiting = false;

    do {
        ThsnValueType value_type;
        GOTO_ON_ERROR(
            thsn_value_type(parse_result, current_value_handle, &value_type),
            error_cleanup);
        switch (value_type) {
            case THSN_VALUE_NULL:
                CALL_VISITOR2(vtable->visit_null, &context, user_data);
                break;
            case THSN_VALUE_BOOL: {
                bool value;
                GOTO_ON_ERROR(thsn_value_read_bool(
                                  parse_result, current_value_handle, &value),
                              error_cleanup);
                CALL_VISITOR3(vtable->visit_bool, &context, user_data, value);
                break;
            }
            case THSN_VALUE_NUMBER: {
                double value;
                GOTO_ON_ERROR(thsn_value_read_number(
                                  parse_result, current_value_handle, &value),
                              error_cleanup);
                CALL_VISITOR3(vtable->visit_number, &context, user_data, value);
                break;
            }
            case THSN_VALUE_STRING: {
                ThsnSlice string_slice;
                GOTO_ON_ERROR(
                    thsn_value_read_string(parse_result, current_value_handle,
                                           &string_slice),
                    error_cleanup);
                CALL_VISITOR3(vtable->visit_string, &context, user_data,
                              string_slice);
                break;
            }
            case THSN_VALUE_ARRAY: {
                CALL_VISITOR2(vtable->visit_array_start, &context, user_data);
                if (skip) {
                    break;
                }
                ThsnValueArrayTable array_table;
                GOTO_ON_ERROR(
                    thsn_value_read_array(parse_result, current_value_handle,
                                          &array_table),
                    error_cleanup);
                GOTO_ON_ERROR(THSN_VECTOR_PUSH_3_VARS(stack, array_table,
                                                      context, VISIT_TAG_ARRAY),
                              error_cleanup);
                break;
            }
            case THSN_VALUE_OBJECT: {
                CALL_VISITOR2(vtable->visit_object_start, &context, user_data);
                if (skip) {
                    break;
                }
                ThsnValueObjectTable object_table;
                GOTO_ON_ERROR(
                    thsn_value_read_object(parse_result, current_value_handle,
                                           &object_table),
                    error_cleanup);
                GOTO_ON_ERROR(
                    THSN_VECTOR_PUSH_3_VARS(stack, object_table, context,
                                            VISIT_TAG_OBJECT_KV),
                    error_cleanup);
                break;
            }
        }

        bool found_offset = false;
        do {
            if (thsn_vector_is_empty(stack)) {
                done_visiting = true;
                break;
            }
            ThsnVisitTag next_visit_tag;
            GOTO_ON_ERROR(THSN_VECTOR_POP_VAR(stack, next_visit_tag),
                          error_cleanup);
            switch (next_visit_tag) {
                case THSN_VISIT_TAG_ARRAY: {
                    ThsnValueArrayTable array_table;
                    GOTO_ON_ERROR(
                        THSN_VECTOR_POP_2_VARS(stack, context, array_table),
                        error_cleanup);
                    if (thsn_value_array_length(array_table) == 0) {
                        CALL_VISITOR2(vtable->visit_array_end, &context,
                                      user_data);
                        break;
                    }
                    GOTO_ON_ERROR(thsn_value_array_consume_element(
                                      &array_table, &current_value_handle),
                                  error_cleanup);
                    GOTO_ON_ERROR(
                        THSN_VECTOR_PUSH_3_VARS(stack, array_table, context,
                                                VISIT_TAG_ARRAY),
                        error_cleanup);
                    context.in_object = false;
                    context.in_array = true;
                    context.last = thsn_value_array_length(array_table) == 0;
                    context.key = thsn_slice_make_empty();
                    found_offset = true;
                    break;
                }
                case THSN_VISIT_TAG_OBJECT_KV: {
                    ThsnValueObjectTable object_table;
                    GOTO_ON_ERROR(
                        THSN_VECTOR_POP_2_VARS(stack, context, object_table),
                        error_cleanup);
                    if (thsn_value_object_length(object_table) == 0) {
                        CALL_VISITOR2(vtable->visit_object_end, &context,
                                      user_data);
                        break;
                    }
                    ThsnSlice key_slice;
                    GOTO_ON_ERROR(thsn_value_object_consume_element(
                                      parse_result, &object_table, &key_slice,
                                      &current_value_handle),
                                  error_cleanup);
                    GOTO_ON_ERROR(
                        THSN_VECTOR_PUSH_3_VARS(stack, object_table, context,
                                                VISIT_TAG_OBJECT_KV),
                        error_cleanup);

                    context.in_array = false;
                    context.last = thsn_value_object_length(object_table) == 0;
                    context.key = key_slice;
                    context.in_object = true;
                    found_offset = true;
                    break;
                }
            }
        } while (!done_visiting && !found_offset);
    } while (!done_visiting);

success_cleanup:
    thsn_vector_free(&stack);
    return THSN_RESULT_SUCCESS;

error_cleanup:
    thsn_vector_free(&stack);
    return THSN_RESULT_INPUT_ERROR;

#undef CALL_VISITOR2
#undef CALL_VISITOR3
#undef PROCESS_VISITOR_RESULT
}
