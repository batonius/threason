#ifndef THSN_TAGS_H
#define THSN_TAGS_H

#include <stdbool.h>

#include "result.h"
#include "slice.h"
#include "vector.h"

typedef enum {
    THSN_TAG_NULL,
    THSN_TAG_BOOL,
    THSN_TAG_SMALL_STRING,
    THSN_TAG_REF_STRING,
    THSN_TAG_INT,
    THSN_TAG_DOUBLE,
    THSN_TAG_ARRAY,
    THSN_TAG_OBJECT,
} ThsnTagType;

typedef unsigned char ThsnTagSize;

typedef unsigned char ThsnTag;

#define THSN_TAG_SIZE_FALSE 0
#define THSN_TAG_SIZE_TRUE 1
#define THSN_TAG_SIZE_EMPTY 0
#define THSN_TAG_SIZE_ZERO 0
#define THSN_TAG_SIZE_F64 0
#define THSN_TAG_SIZE_MAX 0xf
#define THSN_TAG_SIZE_INBOUND 1
#define THSN_TAG_SIZE_NEAR_OFFSET 1
#define THSN_TAG_SIZE_FAR_OFFSET 1

inline ThsnTag thsn_tag_make(ThsnTagType type, ThsnTagSize size) {
    return (ThsnTag)((type << 4) | (size & 0x0f));
}

inline ThsnTagType thsn_tag_type(ThsnTag tag) {
    return (ThsnTagType)(tag >> 4);
}

inline ThsnTagSize thsn_tag_size(ThsnTag tag) {
    return (ThsnTagSize)(tag & 0x0f);
}

inline ThsnResult thsn_vector_store_tagged_value(ThsnVector* /*in/out*/ vector,
                                                 ThsnTag tag,
                                                 ThsnSlice value_slice) {
    BAIL_ON_NULL_INPUT(vector);
    ThsnMutSlice allocated_data;
    BAIL_ON_ERROR(thsn_vector_grow(vector, sizeof(tag) + value_slice.size,
                                   &allocated_data));
    BAIL_ON_ERROR(THSN_MUT_SLICE_WRITE_VAR(allocated_data, tag));
    BAIL_ON_ERROR(thsn_mut_slice_write(&allocated_data, value_slice));
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_vector_store_null(ThsnVector* /*in/out*/ vector) {
    return thsn_vector_store_tagged_value(
        vector, thsn_tag_make(THSN_TAG_NULL, 0), thsn_slice_make_empty());
}

inline ThsnResult thsn_vector_store_bool(ThsnVector* /*in/out*/ vector,
                                         bool value) {
    return thsn_vector_store_tagged_value(
        vector,
        thsn_tag_make(THSN_TAG_BOOL,
                      value ? THSN_TAG_SIZE_TRUE : THSN_TAG_SIZE_FALSE),
        thsn_slice_make_empty());
}

inline ThsnResult thsn_vector_store_double(ThsnVector* /*in/out*/ vector,
                                           double value) {
    return thsn_vector_store_tagged_value(
        vector, thsn_tag_make(THSN_TAG_DOUBLE, THSN_TAG_SIZE_F64),
        THSN_SLICE_FROM_VAR(value));
}

inline ThsnResult thsn_vector_store_int(ThsnVector* /*in/out*/ vector,
                                        long long value) {
    if (value == 0) {
        return thsn_vector_store_tagged_value(
            vector, thsn_tag_make(THSN_TAG_INT, THSN_TAG_SIZE_ZERO),
            thsn_slice_make_empty());
    }
    if (value <= INT8_MAX && value >= INT8_MIN) {
        const int8_t int8_value = (int8_t)value;
        return thsn_vector_store_tagged_value(
            vector, thsn_tag_make(THSN_TAG_INT, sizeof(int8_t)),
            THSN_SLICE_FROM_VAR(int8_value));
    }
    if (value <= INT16_MAX && value >= INT16_MIN) {
        const int16_t int16_value = (int16_t)value;
        return thsn_vector_store_tagged_value(
            vector, thsn_tag_make(THSN_TAG_INT, sizeof(int16_t)),
            THSN_SLICE_FROM_VAR(int16_value));
    }
    if (value <= INT32_MAX && value >= INT32_MIN) {
        const int32_t int32_value = (int32_t)value;
        return thsn_vector_store_tagged_value(
            vector, thsn_tag_make(THSN_TAG_INT, sizeof(int32_t)),
            THSN_SLICE_FROM_VAR(int32_value));
    }
    return thsn_vector_store_tagged_value(
        vector, thsn_tag_make(THSN_TAG_INT, sizeof(long long)),
        THSN_SLICE_FROM_VAR(value));
}

inline ThsnResult thsn_vector_store_string(ThsnVector* /*in/out*/ vector,
                                           ThsnSlice string_slice) {
    if (string_slice.size <= THSN_TAG_SIZE_MAX) {
        return thsn_vector_store_tagged_value(
            vector, thsn_tag_make(THSN_TAG_SMALL_STRING, string_slice.size),
            string_slice);
    } else {
        return thsn_vector_store_tagged_value(
            vector, thsn_tag_make(THSN_TAG_REF_STRING, THSN_TAG_SIZE_INBOUND),
            THSN_SLICE_FROM_VAR(string_slice));
    }
}

inline ThsnResult thsn_slice_read_string(ThsnSlice stored_string_slice,
                                         ThsnSlice* /*out*/ string_slice,
                                         size_t* /*out*/ stored_length) {
    BAIL_ON_NULL_INPUT(string_slice);
    char key_str_tag;
    if (!thsn_slice_try_consume_char(&stored_string_slice, &key_str_tag)) {
        return THSN_RESULT_INPUT_ERROR;
    }
    *string_slice = thsn_slice_make_empty();
    switch (thsn_tag_type(key_str_tag)) {
        case THSN_TAG_REF_STRING:
            if (thsn_tag_size(key_str_tag) != THSN_TAG_SIZE_EMPTY) {
                BAIL_ON_ERROR(
                    THSN_SLICE_READ_VAR(stored_string_slice, *string_slice));
            }
            if (stored_length != NULL) {
                *stored_length = sizeof(ThsnTag) + sizeof(*string_slice);
            }
            break;
        case THSN_TAG_SMALL_STRING: {
            size_t key_str_size = thsn_tag_size(key_str_tag);
            if (stored_string_slice.size < key_str_size) {
                return THSN_RESULT_INPUT_ERROR;
            }
            *string_slice =
                thsn_slice_make(stored_string_slice.data, key_str_size);
            if (stored_length != NULL) {
                *stored_length = sizeof(ThsnTag) + key_str_size;
            }
            break;
        }
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

#endif
