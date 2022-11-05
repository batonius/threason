#include "tags.h"
#include "vector.h"
#include "visitor.h"

thsn_result_t thsn_visit(thsn_slice_t parsed_data,
                         thsn_visitor_vtable_t* vtable, void* user_data) {
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

#define READ_SLICE_INTO_VAR(slice, var)                  \
    do {                                                 \
        if ((slice).size < sizeof(var)) {                \
            goto error_cleanup;                          \
        }                                                \
        memcpy(&(var), (slice).data, sizeof(var));       \
        THSN_SLICE_ADVANCE_UNSAFE((slice), sizeof(var)); \
    } while (0)

    if (THSN_SLICE_EMPTY(parsed_data)) {
        return THSN_RESULT_SUCCESS;
    }

    thsn_visitor_context_t context = {
        .depth = 0,
        .key = THSN_SLICE_MAKE_EMPTY(),
        .in_array = false,
        .last = false,
    };

    thsn_vector_t stack;
    bool skip = false;
    size_t next_offset = 0;
    BAIL_ON_ERROR(thsn_vector_make(&stack, 1024));

    do {
        thsn_slice_t data_slice = parsed_data;
        if (next_offset >= data_slice.size) {
            goto error_cleanup;
        }
        THSN_SLICE_ADVANCE_UNSAFE(data_slice, next_offset);
        char tag = THSN_SLICE_NEXT_CHAR_UNSAFE(data_slice);
        switch (THSN_TAG_TYPE(tag)) {
            case THSN_TAG_NULL:
                CALL_VISITOR2(vtable->visit_null, &context, user_data);
                break;
            case THSN_TAG_BOOL:
                CALL_VISITOR3(vtable->visit_bool, &context, user_data,
                              THSN_TAG_SIZE(tag) != THSN_TAG_SIZE_FALSE);
                break;
            case THSN_TAG_SMALL_STRING: {
                size_t string_size = THSN_TAG_SIZE(tag);
                if (data_slice.size < string_size) {
                    goto error_cleanup;
                }
                CALL_VISITOR3(vtable->visit_string, &context, user_data,
                              THSN_SLICE_MAKE(data_slice.data, string_size));
                THSN_SLICE_ADVANCE_UNSAFE(data_slice, string_size);
                break;
            }
            case THSN_TAG_REF_STRING: {
                thsn_slice_t string_slice = THSN_SLICE_MAKE_EMPTY();

                if (THSN_TAG_SIZE(tag) != THSN_TAG_SIZE_EMPTY) {
                    READ_SLICE_INTO_VAR(data_slice, string_slice);
                }
                CALL_VISITOR3(vtable->visit_string, &context, user_data,
                              string_slice);
                break;
            }
            case THSN_TAG_INT: {
                long long value = 0;
                switch (THSN_TAG_SIZE(tag)) {
                    case THSN_TAG_SIZE_ZERO:
                        break;
                    case sizeof(int8_t): {
                        int8_t int8_value;
                        READ_SLICE_INTO_VAR(data_slice, int8_value);
                        value = int8_value;
                        break;
                    }
                    case sizeof(int16_t): {
                        int16_t int16_value;
                        READ_SLICE_INTO_VAR(data_slice, int16_value);
                        value = int16_value;
                        break;
                    }
                    case sizeof(int32_t): {
                        int32_t int32_value;
                        READ_SLICE_INTO_VAR(data_slice, int32_value);
                        value = int32_value;
                        break;
                    }
                    case sizeof(long long): {
                        READ_SLICE_INTO_VAR(data_slice, value);
                        break;
                    }
                    default:
                        goto error_cleanup;
                }
                CALL_VISITOR3(vtable->visit_integer, &context, user_data,
                              value);
                break;
            }
            case THSN_TAG_DOUBLE:
                goto error_cleanup;
            case THSN_TAG_ARRAY:
            case THSN_TAG_OBJECT:
                break;
        }
        if (THSN_VECTOR_EMPTY(stack)) {
            break;
        }
    } while (true);

success_cleanup:
    thsn_vector_free(&stack);
    return THSN_RESULT_SUCCESS;

error_cleanup:
    thsn_vector_free(&stack);
    return THSN_RESULT_INPUT_ERROR;

#undef CALL_VISITOR2
#undef CALL_VISITOR3
#undef PROCESS_VISITOR_RESULT
#undef READ_SLICE_INTO_VAR
}
