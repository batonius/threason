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

inline ThsnResult thsn_value_type(ThsnSlice slice, ThsnValueHandle value_handle,
                                  ThsnValueType* value_type) {
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(slice, value_handle, sizeof(ThsnTag),
                                       &value_slice));
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

inline ThsnResult thsn_value_read_number(ThsnSlice slice,
                                         ThsnValueHandle value_handle,
                                         double* value) {
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(slice, value_handle, sizeof(ThsnTag),
                                       &value_slice));
    ThsnTag value_tag;
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, value_tag));
    switch (thsn_tag_type(value_tag)) {
        case THSN_TAG_INT: {
            switch (thsn_tag_size(value_tag)) {
                case THSN_TAG_SIZE_ZERO:
                    break;
                case sizeof(int8_t): {
                    int8_t int8_value;
                    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(slice, int8_value));
                    *value = (double)int8_value;
                    break;
                }
                case sizeof(int16_t): {
                    int16_t int16_value;
                    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(slice, int16_value));
                    *value = (double)int16_value;
                    break;
                }
                case sizeof(int32_t): {
                    int32_t int32_value;
                    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(slice, int32_value));
                    *value = (double)int32_value;
                    break;
                }
                case sizeof(int64_t): {
                    int64_t int64_value;
                    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(slice, int64_value));
                    *value = (double)int64_value;
                    break;
                }
                default:
                    return THSN_RESULT_INPUT_ERROR;
            }
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

inline ThsnResult thsn_value_read_string(ThsnSlice slice,
                                         ThsnValueHandle value_handle,
                                         ThsnSlice* value) {
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(slice, value_handle, sizeof(ThsnTag),
                                       &value_slice));
    ThsnTag value_tag;
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, value_tag));
    switch (thsn_tag_type(value_tag)) {
        case THSN_TAG_SMALL_STRING: {
            size_t string_length = thsn_tag_size(value_tag);
            BAIL_ON_ERROR(thsn_slice_at_offset(value_slice, sizeof(ThsnTag),
                                               string_length, value));
            break;
        }
        case THSN_TAG_REF_STRING: {
            if (thsn_tag_size(value_tag) != THSN_TAG_SIZE_INBOUND) {
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

#endif
