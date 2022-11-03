#include "tags.h"

thsn_result_t thsn_vector_store_tagged_value(thsn_vector_t* /*in/out*/ vector,
                                             thsn_tag_t tag,
                                             thsn_slice_t value_slice) {
    BAIL_ON_NULL_INPUT(vector);
    if (value_slice.size > 0) {
        BAIL_ON_NULL_INPUT(value_slice.data);
    }
    size_t allocation_offset = THSN_VECTOR_OFFSET(*vector);
    BAIL_ON_ERROR(thsn_vector_grow(vector, sizeof(tag) + value_slice.size));
    char* allocated_data = THSN_VECTOR_AT_OFFSET(*vector, allocation_offset);
    memcpy(allocated_data, &tag, sizeof(tag));
    allocated_data += sizeof(tag);
    if (value_slice.size > 0) {
        memcpy(allocated_data, value_slice.data, value_slice.size);
    }
    return THSN_RESULT_SUCCESS;
}

thsn_result_t thsn_vector_store_null(thsn_vector_t* /*in/out*/ vector) {
    return thsn_vector_store_tagged_value(
        vector, THSN_TAG_MAKE(THSN_TAG_NULL, 0), THSN_SLICE_MAKE_EMPTY());
}

thsn_result_t thsn_vector_store_bool(thsn_vector_t* /*in/out*/ vector,
                                     bool value) {
    return thsn_vector_store_tagged_value(
        vector,
        THSN_TAG_MAKE(THSN_TAG_BOOL,
                      value ? THSN_TAG_SIZE_TRUE : THSN_TAG_SIZE_FALSE),
        THSN_SLICE_MAKE_EMPTY());
}

thsn_result_t thsn_vector_store_double(thsn_vector_t* /*in/out*/ vector,
                                       double value) {
    return thsn_vector_store_tagged_value(
        vector, THSN_TAG_MAKE(THSN_TAG_DOUBLE, THSN_TAG_SIZE_F64),
        THSN_SLICE_FROM_VAR(value));
}

thsn_result_t thsn_vector_store_int(thsn_vector_t* /*in/out*/ vector,
                                    long long value) {
    if (value == 0) {
        return thsn_vector_store_tagged_value(
            vector, THSN_TAG_MAKE(THSN_TAG_INT, THSN_TAG_SIZE_ZERO),
            THSN_SLICE_MAKE_EMPTY());
    }
    if (value <= INT8_MAX && value >= INT8_MIN) {
        const int8_t int8_value = (int8_t)value;
        return thsn_vector_store_tagged_value(
            vector, THSN_TAG_MAKE(THSN_TAG_INT, THSN_TAG_SIZE_INT8),
            THSN_SLICE_FROM_VAR(int8_value));
    }
    if (value <= INT16_MAX && value >= INT16_MIN) {
        const int16_t int16_value = (int16_t)value;
        return thsn_vector_store_tagged_value(
            vector, THSN_TAG_MAKE(THSN_TAG_INT, THSN_TAG_SIZE_INT16),
            THSN_SLICE_FROM_VAR(int16_value));
    }
    if (value <= INT32_MAX && value >= INT32_MIN) {
        const int32_t int32_value = (int32_t)value;
        return thsn_vector_store_tagged_value(
            vector, THSN_TAG_MAKE(THSN_TAG_INT, THSN_TAG_SIZE_INT32),
            THSN_SLICE_FROM_VAR(int32_value));
    }
    return thsn_vector_store_tagged_value(
        vector, THSN_TAG_MAKE(THSN_TAG_INT, THSN_TAG_SIZE_INT64),
        THSN_SLICE_FROM_VAR(value));
}

thsn_result_t thsn_vector_store_string(thsn_vector_t* /*in/out*/ vector,
                                       thsn_slice_t string_slice) {
    if (string_slice.size <= THSN_TAG_SIZE_MAX) {
        return thsn_vector_store_tagged_value(
            vector, THSN_TAG_MAKE(THSN_TAG_SMALL_STRING, string_slice.size),
            string_slice);
    } else {
        return thsn_vector_store_tagged_value(
            vector, THSN_TAG_MAKE(THSN_TAG_REF_STRING, THSN_TAG_SIZE_INBOUND),
            THSN_SLICE_FROM_VAR(string_slice));
    }
}
