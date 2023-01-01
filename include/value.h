#ifndef THSN_VALUE_H
#define THSN_VALUE_H

#include <stddef.h>

#include "result.h"
#include "slice.h"
#include "tags.h"

typedef enum {
    THSN_VALUE_NULL,
    THSN_VALUE_BOOL,
    THSN_VALUE_NUMBER,
    THSN_VALUE_STRING,
    THSN_VALUE_ARRAY,
    THSN_VALUE_OBJECT,
} ThsnValueType;

typedef size_t ThsnValueHandle;
#define THSN_VALUE_HANDLE_FIRST 0
#define THSN_VALUE_HANDLE_NOT_FOUND SIZE_MAX

typedef ThsnSlice ThsnValueArrayTable;
typedef ThsnSlice ThsnValueObjectTable;

inline ThsnResult thsn_value_type(ThsnSlice data_slice,
                                  ThsnValueHandle value_handle,
                                  ThsnValueType* value_type) {
    BAIL_ON_NULL_INPUT(value_type);
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(data_slice, value_handle,
                                       sizeof(ThsnTag), &value_slice));
    ThsnTag value_tag;
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, value_tag));
    switch (thsn_tag_type(value_tag)) {
        case THSN_TAG_NULL:
            *value_type = THSN_VALUE_NULL;
            break;
        case THSN_TAG_BOOL:
            *value_type = THSN_VALUE_BOOL;
            break;
        case THSN_TAG_SMALL_STRING:
        case THSN_TAG_REF_STRING:
            *value_type = THSN_VALUE_STRING;
            break;
        case THSN_TAG_INT:
        case THSN_TAG_DOUBLE:
            *value_type = THSN_VALUE_NUMBER;
            break;
        case THSN_TAG_ARRAY:
            *value_type = THSN_VALUE_ARRAY;
            break;
        case THSN_TAG_OBJECT:
            *value_type = THSN_VALUE_OBJECT;
            break;
    }
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_value_read_bool(ThsnSlice slice,
                                       ThsnValueHandle value_handle,
                                       bool* value) {
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(slice, value_handle, sizeof(ThsnTag),
                                       &value_slice));
    ThsnTag value_tag;
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, value_tag));
    switch (thsn_tag_type(value_tag)) {
        case THSN_TAG_BOOL:
            switch (thsn_tag_size(value_tag)) {
                case THSN_TAG_SIZE_FALSE:
                    *value = false;
                    break;
                case THSN_TAG_SIZE_TRUE:
                    *value = true;
                    break;
                default:
                    return THSN_RESULT_INPUT_ERROR;
            }
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_value_read_number(ThsnSlice data_slice,
                                         ThsnValueHandle value_handle,
                                         double* value) {
    BAIL_ON_NULL_INPUT(value);
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(data_slice, value_handle,
                                       sizeof(ThsnTag), &value_slice));
    ThsnTag value_tag;
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, value_tag));
    switch (thsn_tag_type(value_tag)) {
        case THSN_TAG_INT: {
            switch (thsn_tag_size(value_tag)) {
                case THSN_TAG_SIZE_ZERO:
                    break;
                case sizeof(int8_t): {
                    int8_t int8_value;
                    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, int8_value));
                    *value = (double)int8_value;
                    break;
                }
                case sizeof(int16_t): {
                    int16_t int16_value;
                    BAIL_ON_ERROR(
                        THSN_SLICE_READ_VAR(value_slice, int16_value));
                    *value = (double)int16_value;
                    break;
                }
                case sizeof(int32_t): {
                    int32_t int32_value;
                    BAIL_ON_ERROR(
                        THSN_SLICE_READ_VAR(value_slice, int32_value));
                    *value = (double)int32_value;
                    break;
                }
                case sizeof(int64_t): {
                    int64_t int64_value;
                    BAIL_ON_ERROR(
                        THSN_SLICE_READ_VAR(value_slice, int64_value));
                    *value = (double)int64_value;
                    break;
                }
                default:
                    return THSN_RESULT_INPUT_ERROR;
            }
            break;
        }
        case THSN_TAG_DOUBLE: {
            if (thsn_tag_size(value_tag) != THSN_TAG_SIZE_F64) {
                return THSN_RESULT_INPUT_ERROR;
            }
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, *value));
            break;
        }
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_value_read_string_ex(ThsnSlice data_slice,
                                            ThsnValueHandle value_handle,
                                            ThsnSlice* string_slice,
                                            size_t* consumed_size) {
    BAIL_ON_NULL_INPUT(string_slice);
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(data_slice, value_handle,
                                       sizeof(ThsnTag), &value_slice));
    ThsnTag value_tag;
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, value_tag));
    switch (thsn_tag_type(value_tag)) {
        case THSN_TAG_SMALL_STRING: {
            const size_t string_length = thsn_tag_size(value_tag);
            *string_slice = value_slice;
            BAIL_ON_ERROR(thsn_slice_truncate(string_slice, string_length));
            if (consumed_size != NULL) {
                *consumed_size = sizeof(ThsnTag) + string_length;
            }
            break;
        }
        case THSN_TAG_REF_STRING: {
            if (thsn_tag_size(value_tag) != THSN_TAG_SIZE_INBOUND) {
                return THSN_RESULT_INPUT_ERROR;
            }
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, *string_slice));
            if (consumed_size != NULL) {
                *consumed_size = sizeof(ThsnTag) + sizeof(ThsnSlice);
            }
            break;
        }
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_value_read_string(ThsnSlice data_slice,
                                         ThsnValueHandle value_handle,
                                         ThsnSlice* string_slice) {
    return thsn_value_read_string_ex(data_slice, value_handle, string_slice,
                                     NULL);
}

inline ThsnResult thsn_value_read_composite(ThsnSlice data_slice,
                                            ThsnValueHandle value_handle,
                                            ThsnTagType expected_type,
                                            ThsnSlice* elements_table) {
    BAIL_ON_NULL_INPUT(elements_table);
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(data_slice, value_handle,
                                       sizeof(ThsnTag), &value_slice));
    ThsnTag value_tag;
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, value_tag));
    if (thsn_tag_type(value_tag) != expected_type) {
        return THSN_RESULT_INPUT_ERROR;
    }
    switch (thsn_tag_size(value_tag)) {
        case THSN_TAG_SIZE_ZERO:
            *elements_table = thsn_slice_make_empty();
            break;
        case THSN_TAG_SIZE_INBOUND: {
            size_t table_offset;
            size_t table_len;
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, table_len));
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, table_offset));
            const size_t table_size = table_len * sizeof(size_t);
            BAIL_ON_ERROR(thsn_slice_at_offset(data_slice, table_offset,
                                               table_size, elements_table));
            BAIL_ON_ERROR(thsn_slice_truncate(elements_table, table_size));
            break;
        }
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

inline size_t thsn_value_read_array(ThsnSlice data_slice,
                                    ThsnValueHandle value_handle,
                                    ThsnValueArrayTable* array_table) {
    return thsn_value_read_composite(data_slice, value_handle, THSN_TAG_ARRAY,
                                     array_table);
}

inline size_t thsn_value_array_length(ThsnValueArrayTable array_table) {
    return array_table.size / sizeof(size_t);
}

inline ThsnResult thsn_value_array_element_handle(
    ThsnValueArrayTable array_table, size_t element_no,
    ThsnValueHandle* handle) {
    BAIL_ON_NULL_INPUT(handle);
    ThsnSlice element_handle_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(array_table, element_no * sizeof(size_t),
                                       sizeof(size_t), &element_handle_slice));
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(element_handle_slice, *handle));
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_value_array_consume_element(
    ThsnValueArrayTable* array_table, ThsnValueHandle* element_handle) {
    BAIL_ON_NULL_INPUT(array_table);
    BAIL_ON_NULL_INPUT(element_handle);
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(*array_table, *element_handle));
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_value_read_object(ThsnSlice data_slice,
                                         ThsnValueHandle value_handle,
                                         ThsnValueObjectTable* object_table) {
    BAIL_ON_NULL_INPUT(object_table);
    return thsn_value_read_composite(data_slice, value_handle, THSN_TAG_OBJECT,
                                     object_table);
}

inline size_t thsn_value_object_length(ThsnValueObjectTable object_table) {
    return thsn_value_array_length(object_table);
}

inline ThsnResult thsn_value_object_read_kv(ThsnSlice data_slice,
                                            ThsnValueHandle kv_handle,
                                            ThsnSlice* key_slice,
                                            ThsnValueHandle* value_handle) {
    BAIL_ON_NULL_INPUT(key_slice);
    BAIL_ON_NULL_INPUT(value_handle);
    size_t key_size;
    BAIL_ON_ERROR(
        thsn_value_read_string_ex(data_slice, kv_handle, key_slice, &key_size));
    *value_handle = kv_handle + key_size;
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_value_object_element_handle(
    ThsnSlice data_slice, ThsnValueObjectTable object_table, size_t element_no,
    ThsnSlice* key_slice, ThsnValueHandle* value_handle) {
    BAIL_ON_NULL_INPUT(key_slice);
    BAIL_ON_NULL_INPUT(value_handle);
    ThsnValueHandle kv_handle;
    BAIL_ON_ERROR(
        thsn_value_array_element_handle(object_table, element_no, &kv_handle));
    return thsn_value_object_read_kv(data_slice, kv_handle, key_slice,
                                     value_handle);
}

inline ThsnResult thsn_value_object_consume_element(
    ThsnSlice data_slice, ThsnValueObjectTable* object_table,
    ThsnSlice* key_slice, ThsnValueHandle* value_handle) {
    BAIL_ON_NULL_INPUT(object_table);
    BAIL_ON_NULL_INPUT(key_slice);
    BAIL_ON_NULL_INPUT(value_handle);
    ThsnValueHandle kv_handle;
    BAIL_ON_ERROR(thsn_value_array_consume_element(object_table, &kv_handle));
    return thsn_value_object_read_kv(data_slice, kv_handle, key_slice,
                                     value_handle);
}

inline ThsnResult thsn_value_object_index(ThsnSlice data_slice,
                                          ThsnValueObjectTable object_table,
                                          ThsnSlice key_slice,
                                          ThsnValueHandle* handle) {
    ThsnSlice element_key_slice;
    ThsnValueHandle element_value_handle;
    *handle = THSN_VALUE_HANDLE_NOT_FOUND;
    while (!thsn_slice_is_empty(object_table)) {
        const size_t elements_count = object_table.size / sizeof(size_t);
        const size_t midpoint = elements_count / 2;
        BAIL_ON_ERROR(thsn_value_object_element_handle(
            data_slice, object_table, midpoint, &element_key_slice,
            &element_value_handle));
        const size_t min_len = key_slice.size < element_key_slice.size
                                   ? key_slice.size
                                   : element_key_slice.size;
        int cmp_result =
            memcmp(key_slice.data, element_key_slice.data, min_len);
        if (cmp_result == 0 && key_slice.size != element_key_slice.size) {
            cmp_result = key_slice.size < element_key_slice.size ? -1 : 1;
        }

        if (cmp_result == 0) {
            *handle = element_value_handle;
            return THSN_RESULT_SUCCESS;
        } else if (cmp_result < 0) {
            BAIL_ON_ERROR(
                thsn_slice_truncate(&object_table, midpoint * sizeof(size_t)));
        } else {
            BAIL_ON_ERROR(thsn_slice_at_offset(
                object_table, (midpoint + 1) * sizeof(size_t),
                (elements_count - midpoint - 1) * sizeof(size_t),
                &object_table));
        }
    }
    return THSN_RESULT_SUCCESS;
}

#endif
