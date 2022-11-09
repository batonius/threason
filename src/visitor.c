#include "tags.h"
#include "vector.h"
#include "visitor.h"

typedef enum {
    THSN_VISIT_TAG_ARRAY,
    THSN_VISIT_TAG_OBJECT_NODE,
    THSN_VISIT_TAG_OBJECT_KV,
    THSN_VISIT_TAG_OBJECT_END,
} ThsnVisitTag;

static const ThsnVisitTag VISIT_TAG_ARRAY = THSN_VISIT_TAG_ARRAY;
static const ThsnVisitTag VISIT_TAG_OBJECT_NODE = THSN_VISIT_TAG_OBJECT_NODE;
static const ThsnVisitTag VISIT_TAG_OBJECT_KV = THSN_VISIT_TAG_OBJECT_KV;
static const ThsnVisitTag VISIT_TAG_OBJECT_END = THSN_VISIT_TAG_OBJECT_END;

ThsnResult thsn_visit(ThsnSlice parse_result, ThsnVisitorVTable* vtable,
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

#define READ_SLICE_INTO_VAR(slice, var)                  \
    do {                                                 \
        if ((slice).size < sizeof(var)) {                \
            goto error_cleanup;                          \
        }                                                \
        memcpy(&(var), (slice).data, sizeof(var));       \
        THSN_SLICE_ADVANCE_UNSAFE((slice), sizeof(var)); \
    } while (0)

    if (THSN_SLICE_EMPTY(parse_result)) {
        return THSN_RESULT_SUCCESS;
    }

    ThsnVisitorContext context = {
        .key = THSN_SLICE_MAKE_EMPTY(),
        .in_array = false,
        .last = false,
    };

    ThsnVector stack = THSN_VECTOR_INIT();
    bool skip = false;
    size_t next_offset = 0;
    bool done_visiting = false;
    BAIL_ON_ERROR(thsn_vector_make(&stack, 1024));

    do {
        ThsnSlice data_slice;
        GOTO_ON_ERROR(thsn_slice_at_offset(parse_result, next_offset,
                                           sizeof(char), &data_slice),
                      error_cleanup);

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
                break;
            }
            case THSN_TAG_REF_STRING: {
                ThsnSlice string_slice = THSN_SLICE_MAKE_EMPTY();

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
            case THSN_TAG_ARRAY: {
                CALL_VISITOR2(vtable->visit_array_start, &context, user_data);
                if (skip) {
                    break;
                }
                size_t elements_count = 0;
                if (THSN_TAG_SIZE(tag) != THSN_TAG_SIZE_EMPTY) {
                    size_t elements_table_offset = 0;
                    READ_SLICE_INTO_VAR(data_slice, elements_count);
                    READ_SLICE_INTO_VAR(data_slice, elements_table_offset);
                    ThsnSlice elements_table_slice;
                    GOTO_ON_ERROR(thsn_slice_at_offset(
                                      parse_result, elements_table_offset,
                                      elements_count * sizeof(size_t),
                                      &elements_table_slice),
                                  error_cleanup);
                    for (size_t i = 0; i < elements_count; ++i) {
                        size_t element_offset;
                        memcpy(&element_offset,
                               elements_table_slice.data +
                                   (elements_count - i - 1) * sizeof(size_t),
                               sizeof(element_offset));
                        GOTO_ON_ERROR(
                            THSN_VECTOR_PUSH_VAR(stack, element_offset),
                            error_cleanup);
                    }
                }
                GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(stack, elements_count),
                              error_cleanup);
                GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(stack, context),
                              error_cleanup);
                GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(stack, VISIT_TAG_ARRAY),
                              error_cleanup);
                break;
            }
            case THSN_TAG_OBJECT: {
                CALL_VISITOR2(vtable->visit_object_start, &context, user_data);
                if (skip) {
                    break;
                }
                size_t node_offset = 0;
                if (THSN_TAG_SIZE(tag) != THSN_TAG_SIZE_EMPTY) {
                    READ_SLICE_INTO_VAR(data_slice, node_offset);
                }
                GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(stack, context),
                              error_cleanup);
                GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(stack, VISIT_TAG_OBJECT_END),
                              error_cleanup);
                if (node_offset != 0) {
                    GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(stack, node_offset),
                                  error_cleanup);
                    GOTO_ON_ERROR(
                        THSN_VECTOR_PUSH_VAR(stack, VISIT_TAG_OBJECT_NODE),
                        error_cleanup);
                }
                break;
            }
        }

        bool found_offset = false;
        do {
            if (THSN_VECTOR_EMPTY(stack)) {
                done_visiting = true;
                break;
            }
            ThsnVisitTag next_visit_tag;
            GOTO_ON_ERROR(THSN_VECTOR_POP_VAR(stack, next_visit_tag),
                          error_cleanup);
            switch (next_visit_tag) {
                case THSN_VISIT_TAG_ARRAY: {
                    GOTO_ON_ERROR(THSN_VECTOR_POP_VAR(stack, context),
                                  error_cleanup);
                    size_t elements_count;
                    GOTO_ON_ERROR(THSN_VECTOR_POP_VAR(stack, elements_count),
                                  error_cleanup);
                    if (elements_count == 0) {
                        CALL_VISITOR2(vtable->visit_array_end, &context,
                                      user_data);
                        break;
                    }
                    size_t element_offset;
                    GOTO_ON_ERROR(THSN_VECTOR_POP_VAR(stack, element_offset),
                                  error_cleanup);
                    --elements_count;
                    GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(stack, elements_count),
                                  error_cleanup);
                    GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(stack, context),
                                  error_cleanup);
                    GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(stack, VISIT_TAG_ARRAY),
                                  error_cleanup);
                    next_offset = element_offset;
                    context.in_object = false;
                    context.in_array = true;
                    context.last = elements_count == 0;
                    context.key = THSN_SLICE_MAKE_EMPTY();
                    found_offset = true;
                    break;
                }
                case THSN_VISIT_TAG_OBJECT_NODE: {
                    size_t object_offset;
                    GOTO_ON_ERROR(THSN_VECTOR_POP_VAR(stack, object_offset),
                                  error_cleanup);
                    ThsnSlice node_slice;
                    GOTO_ON_ERROR(
                        thsn_slice_at_offset(parse_result, object_offset,
                                             2 * sizeof(size_t), &node_slice),
                        error_cleanup);
                    size_t left_child_offset;
                    memcpy(&left_child_offset, node_slice.data,
                           sizeof(left_child_offset));
                    size_t right_child_offset;
                    memcpy(&right_child_offset,
                           node_slice.data + sizeof(size_t),
                           sizeof(right_child_offset));
                    if (right_child_offset != 0 &&
                        right_child_offset != left_child_offset) {
                        GOTO_ON_ERROR(
                            THSN_VECTOR_PUSH_VAR(stack, right_child_offset),
                            error_cleanup);
                        GOTO_ON_ERROR(
                            THSN_VECTOR_PUSH_VAR(stack, VISIT_TAG_OBJECT_NODE),
                            error_cleanup);
                    }
                    object_offset += 2 * sizeof(size_t);
                    GOTO_ON_ERROR(THSN_VECTOR_PUSH_VAR(stack, object_offset),
                                  error_cleanup);
                    GOTO_ON_ERROR(
                        THSN_VECTOR_PUSH_VAR(stack, VISIT_TAG_OBJECT_KV),
                        error_cleanup);
                    if (left_child_offset != 0) {
                        GOTO_ON_ERROR(
                            THSN_VECTOR_PUSH_VAR(stack, left_child_offset),
                            error_cleanup);
                        GOTO_ON_ERROR(
                            THSN_VECTOR_PUSH_VAR(stack, VISIT_TAG_OBJECT_NODE),
                            error_cleanup);
                    }
                    break;
                }
                case THSN_VISIT_TAG_OBJECT_KV: {
                    size_t kv_offset;
                    GOTO_ON_ERROR(THSN_VECTOR_POP_VAR(stack, kv_offset),
                                  error_cleanup);
                    ThsnSlice kv_slice;
                    GOTO_ON_ERROR(thsn_slice_at_offset(parse_result, kv_offset,
                                                       sizeof(char), &kv_slice),
                                  error_cleanup);
                    char key_str_tag = THSN_SLICE_NEXT_CHAR_UNSAFE(kv_slice);
                    ThsnSlice key_slice = THSN_SLICE_MAKE_EMPTY();
                    size_t value_offset = 0;
                    switch (THSN_TAG_TYPE(key_str_tag)) {
                        case THSN_TAG_REF_STRING:
                            if (THSN_TAG_SIZE(key_str_tag) !=
                                THSN_TAG_SIZE_EMPTY) {
                                READ_SLICE_INTO_VAR(kv_slice, key_slice);
                            }
                            value_offset =
                                kv_offset + sizeof(ThsnTag) + sizeof(key_slice);
                            break;
                        case THSN_TAG_SMALL_STRING: {
                            size_t key_str_size = THSN_TAG_SIZE(key_str_tag);
                            if (kv_slice.size < key_str_size) {
                                goto error_cleanup;
                            }
                            key_slice =
                                THSN_SLICE_MAKE(kv_slice.data, key_str_size);
                            value_offset =
                                kv_offset + sizeof(ThsnTag) + key_str_size;
                            break;
                        }
                        default:
                            goto error_cleanup;
                    }
                    next_offset = value_offset;
                    context.in_array = false;
                    context.last = false;
                    if (THSN_VECTOR_OFFSET(stack) >= sizeof(ThsnVisitTag)) {
                        ThsnVisitTag next_tag;
                        memcpy(&next_tag,
                               THSN_VECTOR_AT_OFFSET(stack,
                                                     THSN_VECTOR_OFFSET(stack) -
                                                         sizeof(ThsnVisitTag)),
                               sizeof(ThsnVisitTag));
                        if (next_tag == THSN_VISIT_TAG_OBJECT_END) {
                            context.last = true;
                        }
                    }
                    context.key = key_slice;
                    context.in_object = true;
                    found_offset = true;
                    break;
                }
                case THSN_VISIT_TAG_OBJECT_END: {
                    GOTO_ON_ERROR(THSN_VECTOR_POP_VAR(stack, context),
                                  error_cleanup);
                    CALL_VISITOR2(vtable->visit_object_end, &context,
                                  user_data);
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
#undef READ_SLICE_INTO_VAR
}
