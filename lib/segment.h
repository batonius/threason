#ifndef THSN_SEGMENT_H
#define THSN_SEGMENT_H

#include <stdbool.h>

#include "result.h"
#include "slice.h"
#include "threason.h"
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
    THSN_TAG_VALUE_HANDLE,
} ThsnTagType;

typedef unsigned char ThsnTagSize;
typedef unsigned char ThsnTag;
typedef ThsnVector ThsnSegment;
typedef ThsnSlice ThsnSegmentSlice;
typedef ThsnMutSlice ThsnSegmentMutSlice;

#define THSN_TAG_SIZE_FALSE 0
#define THSN_TAG_SIZE_TRUE 1
#define THSN_TAG_SIZE_EMPTY 0
#define THSN_TAG_SIZE_ZERO 0
#define THSN_TAG_SIZE_F64 0
#define THSN_TAG_SIZE_MAX 0xf
#define THSN_TAG_SIZE_INBOUND 1
#define THSN_TAG_SIZE_INBOUND_SORTED 2

extern _Thread_local ThsnSlice* CURRENT_SEGMENT;

static inline ThsnTag thsn_tag_make(ThsnTagType type, ThsnTagSize size) {
    return (ThsnTag)((type << 4) | (size & 0x0f));
}

static inline ThsnTagType thsn_tag_type(ThsnTag tag) {
    return (ThsnTagType)(tag >> 4);
}

static inline ThsnTagSize thsn_tag_size(ThsnTag tag) {
    return (ThsnTagSize)(tag & 0x0f);
}

static inline ThsnResult thsn_segment_store_tagged_value(
    ThsnSegment* /*mut*/ segment, ThsnTag tag, ThsnSlice value_slice) {
    BAIL_ON_NULL_INPUT(segment);
    ThsnMutSlice allocated_data;
    BAIL_ON_ERROR(thsn_vector_grow(segment, sizeof(tag) + value_slice.size,
                                   &allocated_data));
    BAIL_ON_ERROR(THSN_MUT_SLICE_WRITE_VAR(allocated_data, tag));
    BAIL_ON_ERROR(thsn_mut_slice_write(&allocated_data, value_slice));
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_store_null(ThsnSegment* /*mut*/ segment) {
    return thsn_segment_store_tagged_value(
        segment, thsn_tag_make(THSN_TAG_NULL, 0), thsn_slice_make_empty());
}

static inline ThsnResult thsn_segment_store_bool(ThsnSegment* /*mut*/ segment,
                                                 bool value) {
    return thsn_segment_store_tagged_value(
        segment,
        thsn_tag_make(THSN_TAG_BOOL,
                      value ? THSN_TAG_SIZE_TRUE : THSN_TAG_SIZE_FALSE),
        thsn_slice_make_empty());
}

static inline ThsnResult thsn_segment_store_double(ThsnSegment* /*mut*/ segment,
                                                   double value) {
    return thsn_segment_store_tagged_value(
        segment, thsn_tag_make(THSN_TAG_DOUBLE, THSN_TAG_SIZE_F64),
        THSN_SLICE_FROM_VAR(value));
}

static inline ThsnResult thsn_segment_store_int(ThsnSegment* /*mut*/ segment,
                                                long long value) {
    if (value == 0) {
        return thsn_segment_store_tagged_value(
            segment, thsn_tag_make(THSN_TAG_INT, THSN_TAG_SIZE_ZERO),
            thsn_slice_make_empty());
    }
    if (value <= INT8_MAX && value >= INT8_MIN) {
        const int8_t int8_value = (int8_t)value;
        return thsn_segment_store_tagged_value(
            segment, thsn_tag_make(THSN_TAG_INT, sizeof(int8_t)),
            THSN_SLICE_FROM_VAR(int8_value));
    }
    if (value <= INT16_MAX && value >= INT16_MIN) {
        const int16_t int16_value = (int16_t)value;
        return thsn_segment_store_tagged_value(
            segment, thsn_tag_make(THSN_TAG_INT, sizeof(int16_t)),
            THSN_SLICE_FROM_VAR(int16_value));
    }
    if (value <= INT32_MAX && value >= INT32_MIN) {
        const int32_t int32_value = (int32_t)value;
        return thsn_segment_store_tagged_value(
            segment, thsn_tag_make(THSN_TAG_INT, sizeof(int32_t)),
            THSN_SLICE_FROM_VAR(int32_value));
    }
    return thsn_segment_store_tagged_value(
        segment, thsn_tag_make(THSN_TAG_INT, sizeof(long long)),
        THSN_SLICE_FROM_VAR(value));
}

static inline ThsnResult thsn_segment_store_value_handle(
    ThsnSegment* /*mut*/ segment, ThsnValueHandle value_handle) {
    return thsn_segment_store_tagged_value(
        segment, thsn_tag_make(THSN_TAG_VALUE_HANDLE, THSN_TAG_SIZE_ZERO),
        THSN_SLICE_FROM_VAR(value_handle));
}

static inline ThsnResult thsn_segment_store_string(ThsnSegment* /*mut*/ segment,
                                                   ThsnSlice string_slice) {
    if (string_slice.size <= THSN_TAG_SIZE_MAX) {
        return thsn_segment_store_tagged_value(
            segment, thsn_tag_make(THSN_TAG_SMALL_STRING, string_slice.size),
            string_slice);
    } else {
        return thsn_segment_store_tagged_value(
            segment, thsn_tag_make(THSN_TAG_REF_STRING, THSN_TAG_SIZE_INBOUND),
            THSN_SLICE_FROM_VAR(string_slice));
    }
}

static inline ThsnResult thsn_segment_store_composite_header(
    ThsnSegment* /*mut*/ segment, ThsnTag tag, bool reserve_sorted_table_offset,
    size_t* /*out*/ header_offset) {
    BAIL_ON_NULL_INPUT(segment);
    BAIL_ON_NULL_INPUT(header_offset);
    const size_t composite_header_size =
        sizeof(ThsnTag) +
        (reserve_sorted_table_offset ? 3 : 2) * sizeof(size_t);
    size_t composite_header_offset = thsn_vector_current_offset(*segment);
    ThsnMutSlice composite_header_mut_slice;
    BAIL_ON_ERROR(thsn_vector_grow(segment, composite_header_size,
                                   &composite_header_mut_slice));
    BAIL_ON_ERROR(THSN_MUT_SLICE_WRITE_VAR(composite_header_mut_slice, tag));
    *header_offset = composite_header_offset;
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_store_composite_elements_table(
    ThsnSegment* /*mut*/ segment, size_t composite_header_offset,
    ThsnSlice elements_table, bool reserve_sorted_table) {
    BAIL_ON_NULL_INPUT(segment);
    ThsnMutSlice elements_table_dst;
    const size_t composite_elements_count =
        elements_table.size / sizeof(size_t);
    const size_t elements_offsets_table_offset =
        thsn_vector_current_offset(*segment);
    BAIL_ON_ERROR(
        thsn_vector_grow(segment, elements_table.size, &elements_table_dst));
    BAIL_ON_ERROR(thsn_mut_slice_write(&elements_table_dst, elements_table));
    size_t sorted_elements_offsets_table_offset =
        thsn_vector_current_offset(*segment);
    if (reserve_sorted_table) {
        BAIL_ON_ERROR(thsn_vector_grow(segment, elements_table.size,
                                       &elements_table_dst));
        BAIL_ON_ERROR(
            thsn_mut_slice_write(&elements_table_dst, elements_table));
    }
    ThsnMutSlice composite_header;
    BAIL_ON_ERROR(thsn_vector_mut_slice_at_offset(
        *segment, composite_header_offset + sizeof(ThsnTag),
        sizeof(size_t) * (reserve_sorted_table ? 3 : 2), &composite_header));
    BAIL_ON_ERROR(
        THSN_MUT_SLICE_WRITE_VAR(composite_header, composite_elements_count));
    BAIL_ON_ERROR(THSN_MUT_SLICE_WRITE_VAR(composite_header,
                                           elements_offsets_table_offset));
    if (reserve_sorted_table) {
        BAIL_ON_ERROR(THSN_MUT_SLICE_WRITE_VAR(
            composite_header, sorted_elements_offsets_table_offset));
    }
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_read_string_from_slice(
    ThsnSlice slice, ThsnSlice* /*out*/ string_slice,
    size_t* /*maybe out*/ stored_length) {
    BAIL_ON_NULL_INPUT(string_slice);
    char key_str_tag;
    BAIL_WITH_INPUT_ERROR_UNLESS(
        thsn_slice_try_consume_char(&slice, &key_str_tag));
    *string_slice = thsn_slice_make_empty();
    switch (thsn_tag_type(key_str_tag)) {
        case THSN_TAG_REF_STRING:
            BAIL_WITH_INPUT_ERROR_UNLESS(thsn_tag_size(key_str_tag) ==
                                         THSN_TAG_SIZE_INBOUND);
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(slice, *string_slice));
            if (stored_length != NULL) {
                *stored_length = sizeof(ThsnTag) + sizeof(ThsnSlice);
            }
            break;
        case THSN_TAG_SMALL_STRING: {
            const size_t key_str_size = thsn_tag_size(key_str_tag);
            *string_slice = slice;
            BAIL_ON_ERROR(thsn_slice_truncate(string_slice, key_str_size));
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

static inline int thsn_compare_kv_keys(const void* a, const void* b) {
    if (CURRENT_SEGMENT == NULL) {
        return 0;
    }
    const ThsnSlice* result_slice = CURRENT_SEGMENT;
    size_t a_offset;
    size_t b_offset;
    memcpy(&a_offset, a, sizeof(size_t));
    memcpy(&b_offset, b, sizeof(size_t));
    ThsnSlice a_key_slice;
    ThsnSlice b_key_slice;
    if (thsn_slice_at_offset(*result_slice, a_offset, 1, &a_key_slice) !=
        THSN_RESULT_SUCCESS) {
        return 0;
    }
    if (thsn_slice_at_offset(*result_slice, b_offset, 1, &b_key_slice) !=
        THSN_RESULT_SUCCESS) {
        return 0;
    }
    ThsnSlice a_key_str_slice;
    ThsnSlice b_key_str_slice;
    if (thsn_segment_read_string_from_slice(a_key_slice, &a_key_str_slice,
                                            NULL)) {
        return 0;
    }
    if (thsn_segment_read_string_from_slice(b_key_slice, &b_key_str_slice,
                                            NULL)) {
        return 0;
    }
    const size_t min_len = a_key_str_slice.size < b_key_str_slice.size
                               ? a_key_str_slice.size
                               : b_key_str_slice.size;
    const int cmp_result =
        memcmp(a_key_str_slice.data, b_key_str_slice.data, min_len);
    if (cmp_result == 0) {
        if (a_key_str_slice.size == b_key_str_slice.size) {
            return 0;
        } else if (a_key_str_slice.size < b_key_str_slice.size) {
            return -1;
        } else {
            return 1;
        }
    } else {
        return cmp_result;
    }
}

static inline ThsnResult thsn_segment_sort_elements_table(
    ThsnMutSlice elements_table, ThsnSlice result_slice) {
    CURRENT_SEGMENT = &result_slice;
    qsort(elements_table.data, elements_table.size / sizeof(size_t),
          sizeof(size_t), thsn_compare_kv_keys);
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_read_tagged_value(
    ThsnSegmentSlice segment_slice, size_t offset, ThsnTag* /*out*/ value_tag,
    ThsnSlice* /*out*/ value_slice) {
    BAIL_ON_NULL_INPUT(value_tag);
    BAIL_ON_NULL_INPUT(value_slice);
    BAIL_ON_ERROR(thsn_slice_at_offset(segment_slice, offset, sizeof(ThsnTag),
                                       value_slice));
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(*value_slice, *value_tag));
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_update_tag(
    ThsnSegmentMutSlice segment_mut_slice, size_t offset, ThsnTag tag) {
    ThsnMutSlice tag_mut_slice;
    BAIL_ON_ERROR(thsn_mut_slice_at_offset(segment_mut_slice, offset,
                                           sizeof(ThsnTag), &tag_mut_slice));
    BAIL_ON_ERROR(THSN_MUT_SLICE_WRITE_VAR(tag_mut_slice, tag));
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_read_bool(ThsnSegmentSlice segment_slice,
                                                size_t offset,
                                                bool* /*out*/ value) {
    BAIL_ON_NULL_INPUT(value);
    ThsnTag value_tag;
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_segment_read_tagged_value(segment_slice, offset,
                                                 &value_tag, &value_slice));
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
            break;
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_read_number(
    ThsnSegmentSlice segment_slice, size_t offset, double* /*out*/ value) {
    BAIL_ON_NULL_INPUT(value);
    ThsnTag value_tag;
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_segment_read_tagged_value(segment_slice, offset,
                                                 &value_tag, &value_slice));
    switch (thsn_tag_type(value_tag)) {
        case THSN_TAG_INT: {
            switch (thsn_tag_size(value_tag)) {
                case THSN_TAG_SIZE_ZERO:
                    *value = 0;
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
            BAIL_WITH_INPUT_ERROR_UNLESS(thsn_tag_size(value_tag) ==
                                         THSN_TAG_SIZE_F64);
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, *value));
            break;
        }
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_read_string_ex(
    ThsnSegmentSlice segment_slice, size_t offset,
    ThsnSlice* /*out*/ string_slice, size_t* /*maybe out*/ consumed_size) {
    BAIL_ON_NULL_INPUT(string_slice);
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(segment_slice, offset, sizeof(ThsnTag),
                                       &value_slice));
    return thsn_segment_read_string_from_slice(value_slice, string_slice,
                                               consumed_size);
}

static inline ThsnResult thsn_segment_read_composite(
    ThsnSegmentMutSlice segment_slice, size_t offset, ThsnTagType expected_type,
    ThsnSlice* /*out*/ elements_table, bool read_sorted_table) {
    BAIL_ON_NULL_INPUT(elements_table);
    ThsnTag value_tag;
    ThsnSlice value_slice;
    BAIL_ON_ERROR(
        thsn_segment_read_tagged_value(thsn_slice_from_mut_slice(segment_slice),
                                       offset, &value_tag, &value_slice));
    BAIL_WITH_INPUT_ERROR_UNLESS(thsn_tag_type(value_tag) == expected_type);
    switch (thsn_tag_size(value_tag)) {
        case THSN_TAG_SIZE_ZERO:
            *elements_table = thsn_slice_make_empty();
            break;
        case THSN_TAG_SIZE_INBOUND:
        case THSN_TAG_SIZE_INBOUND_SORTED: {
            size_t table_offset;
            size_t table_len;
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, table_len));
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, table_offset));
            if (read_sorted_table) {
                BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, table_offset));
            }
            const size_t table_size = table_len * sizeof(size_t);
            BAIL_ON_ERROR(
                thsn_slice_at_offset(thsn_slice_from_mut_slice(segment_slice),
                                     table_offset, table_size, elements_table));
            BAIL_ON_ERROR(thsn_slice_truncate(elements_table, table_size));
            if (read_sorted_table &&
                thsn_tag_size(value_tag) != THSN_TAG_SIZE_INBOUND_SORTED) {
                BAIL_ON_ERROR(thsn_segment_sort_elements_table(
                    thsn_mut_slice_make((char*)elements_table->data,
                                        elements_table->size),
                    thsn_slice_from_mut_slice(segment_slice)));
                BAIL_ON_ERROR(thsn_segment_update_tag(
                    segment_slice, offset,
                    thsn_tag_make(expected_type,
                                  THSN_TAG_SIZE_INBOUND_SORTED)));
            }
            break;
        }
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_composite_index_element_offset(
    ThsnSlice elements_table, size_t element_no,
    size_t* /*out*/ element_offset) {
    BAIL_ON_NULL_INPUT(element_offset);
    ThsnSlice element_handle_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(elements_table,
                                       element_no * sizeof(size_t),
                                       sizeof(size_t), &element_handle_slice));
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(element_handle_slice, *element_offset));
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_composite_consume_element_offset(
    ThsnSlice* /*mut*/ elements_table, size_t* /*out*/ element_offset) {
    BAIL_ON_NULL_INPUT(elements_table);
    BAIL_ON_NULL_INPUT(element_offset);
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(*elements_table, *element_offset));
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_object_read_kv(
    ThsnSegmentSlice segment_slice, size_t offset, ThsnSlice* /*out*/ key_slice,
    size_t* /*out*/ element_offset) {
    BAIL_ON_NULL_INPUT(key_slice);
    BAIL_ON_NULL_INPUT(element_offset);
    size_t key_size;
    BAIL_ON_ERROR(thsn_segment_read_string_ex(segment_slice, offset, key_slice,
                                              &key_size));
    *element_offset = offset + key_size;
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_segment_object_index(
    ThsnSegmentSlice segment_slice, ThsnSlice sorted_elements_table,
    ThsnSlice key_slice, size_t* /*out*/ element_offset, bool* /*out*/ found) {
    BAIL_ON_NULL_INPUT(element_offset);
    BAIL_ON_NULL_INPUT(found);
    ThsnSlice element_key_slice = thsn_slice_make_empty();
    *found = false;
    while (!thsn_slice_is_empty(sorted_elements_table)) {
        const size_t elements_count =
            sorted_elements_table.size / sizeof(size_t);
        const size_t midpoint = elements_count / 2;
        size_t kv_offset;
        BAIL_ON_ERROR(thsn_segment_composite_index_element_offset(
            sorted_elements_table, midpoint, &kv_offset));
        BAIL_ON_ERROR(thsn_segment_object_read_kv(
            segment_slice, kv_offset, &element_key_slice, element_offset));
        const size_t min_len = key_slice.size < element_key_slice.size
                                   ? key_slice.size
                                   : element_key_slice.size;
        int cmp_result =
            memcmp(key_slice.data, element_key_slice.data, min_len);
        if (cmp_result == 0 && key_slice.size != element_key_slice.size) {
            cmp_result = key_slice.size < element_key_slice.size ? -1 : 1;
        }

        if (cmp_result == 0) {
            *found = true;
            return THSN_RESULT_SUCCESS;
        } else if (cmp_result < 0) {
            BAIL_ON_ERROR(thsn_slice_truncate(&sorted_elements_table,
                                              midpoint * sizeof(size_t)));
        } else {
            BAIL_ON_ERROR(thsn_slice_at_offset(
                sorted_elements_table, (midpoint + 1) * sizeof(size_t),
                (elements_count - midpoint - 1) * sizeof(size_t),
                &sorted_elements_table));
        }
    }
    return THSN_RESULT_SUCCESS;
}

#endif
