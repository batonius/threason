#include <stddef.h>

#include "result.h"
#include "slice.h"
#include "tags.h"
#include "threason.h"

ThsnResult thsn_value_follow_handle(const ThsnParsedJson* parsed_json,
                                    ThsnValueHandle* value_handle) {
    BAIL_ON_NULL_INPUT(parsed_json);
    BAIL_ON_NULL_INPUT(value_handle);
    while (true) {
        BAIL_WITH_INPUT_ERROR_UNLESS(value_handle->chunk_no <
                                     parsed_json->chunks_count);
        ThsnSlice value_slice;
        BAIL_ON_ERROR(thsn_slice_at_offset(
            parsed_json->chunks[value_handle->chunk_no], value_handle->offset,
            sizeof(ThsnTag), &value_slice));
        ThsnTag value_tag;
        BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, value_tag));
        if (thsn_tag_type(value_tag) == THSN_TAG_VALUE_HANDLE) {
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, *value_handle));
        } else {
            break;
        }
    }
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_value_type(const ThsnParsedJson* parsed_json,
                           ThsnValueHandle value_handle,
                           ThsnValueType* value_type) {
    BAIL_ON_NULL_INPUT(value_type);
    BAIL_WITH_INPUT_ERROR_UNLESS(value_handle.chunk_no <
                                 parsed_json->chunks_count);
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(
        parsed_json->chunks[value_handle.chunk_no], value_handle.offset,
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
        case THSN_TAG_VALUE_HANDLE:
            return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_value_read_bool(const ThsnParsedJson* parsed_json,
                                ThsnValueHandle value_handle, bool* value) {
    BAIL_ON_NULL_INPUT(value);
    BAIL_WITH_INPUT_ERROR_UNLESS(value_handle.chunk_no <
                                 parsed_json->chunks_count);
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(
        parsed_json->chunks[value_handle.chunk_no], value_handle.offset,
        sizeof(ThsnTag), &value_slice));
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
            break;
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_value_read_number(const ThsnParsedJson* parsed_json,
                                  ThsnValueHandle value_handle, double* value) {
    BAIL_ON_NULL_INPUT(value);
    BAIL_WITH_INPUT_ERROR_UNLESS(value_handle.chunk_no <
                                 parsed_json->chunks_count);
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(
        parsed_json->chunks[value_handle.chunk_no], value_handle.offset,
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

static ThsnResult thsn_value_read_string_ex(const ThsnParsedJson* parsed_json,
                                            ThsnValueHandle value_handle,
                                            ThsnSlice* string_slice,
                                            size_t* consumed_size) {
    BAIL_ON_NULL_INPUT(parsed_json);
    BAIL_ON_NULL_INPUT(string_slice);
    BAIL_WITH_INPUT_ERROR_UNLESS(value_handle.chunk_no <
                                 parsed_json->chunks_count);
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(
        parsed_json->chunks[value_handle.chunk_no], value_handle.offset,
        sizeof(ThsnTag), &value_slice));
    return thsn_slice_read_string(value_slice, string_slice, consumed_size);
}

ThsnResult thsn_value_read_string(const ThsnParsedJson* parsed_json,
                                  ThsnValueHandle value_handle,
                                  ThsnSlice* string_slice) {
    return thsn_value_read_string_ex(parsed_json, value_handle, string_slice,
                                     NULL);
}

static ThsnResult thsn_value_read_composite(
    const ThsnParsedJson* parsed_json, ThsnValueHandle value_handle,
    ThsnTagType expected_type, ThsnValueCompositeTable* composite_table) {
    BAIL_ON_NULL_INPUT(parsed_json);
    BAIL_ON_NULL_INPUT(composite_table);
    BAIL_WITH_INPUT_ERROR_UNLESS(value_handle.chunk_no <
                                 parsed_json->chunks_count);
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(
        parsed_json->chunks[value_handle.chunk_no], value_handle.offset,
        sizeof(ThsnTag), &value_slice));
    ThsnTag value_tag;
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, value_tag));
    if (thsn_tag_type(value_tag) != expected_type) {
        return THSN_RESULT_INPUT_ERROR;
    }
    composite_table->chunk_no = value_handle.chunk_no;
    switch (thsn_tag_size(value_tag)) {
        case THSN_TAG_SIZE_ZERO:
            composite_table->elements_table = thsn_slice_make_empty();
            break;
        case THSN_TAG_SIZE_INBOUND: {
            size_t table_offset;
            size_t table_len;
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, table_len));
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, table_offset));
            const size_t table_size = table_len * sizeof(size_t);
            BAIL_ON_ERROR(thsn_slice_at_offset(
                parsed_json->chunks[value_handle.chunk_no], table_offset,
                table_size, &composite_table->elements_table));
            BAIL_ON_ERROR(thsn_slice_truncate(&composite_table->elements_table,
                                              table_size));
            break;
        }
        default:
            return THSN_RESULT_INPUT_ERROR;
    }
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_value_read_array(const ThsnParsedJson* parsed_json,
                                 ThsnValueHandle value_handle,
                                 ThsnValueArrayTable* array_table) {
    return thsn_value_read_composite(parsed_json, value_handle, THSN_TAG_ARRAY,
                                     array_table);
}

size_t thsn_value_array_length(ThsnValueArrayTable array_table) {
    return array_table.elements_table.size / sizeof(size_t);
}

ThsnResult thsn_value_array_element_handle(const ThsnParsedJson* parsed_json,
                                           ThsnValueArrayTable array_table,
                                           size_t element_no,
                                           ThsnValueHandle* handle) {
    BAIL_ON_NULL_INPUT(handle);
    BAIL_ON_NULL_INPUT(parsed_json);
    ThsnSlice element_handle_slice;
    BAIL_ON_ERROR(thsn_slice_at_offset(array_table.elements_table,
                                       element_no * sizeof(size_t),
                                       sizeof(size_t), &element_handle_slice));
    size_t element_offset;
    BAIL_ON_ERROR(THSN_SLICE_READ_VAR(element_handle_slice, element_offset));
    *handle = (ThsnValueHandle){.chunk_no = array_table.chunk_no,
                                .offset = element_offset};
    BAIL_ON_ERROR(thsn_value_follow_handle(parsed_json, handle));
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_value_array_consume_element(const ThsnParsedJson* parsed_json,
                                            ThsnValueArrayTable* array_table,
                                            ThsnValueHandle* element_handle) {
    BAIL_ON_NULL_INPUT(array_table);
    BAIL_ON_NULL_INPUT(element_handle);
    size_t element_offset;
    BAIL_ON_ERROR(
        THSN_SLICE_READ_VAR(array_table->elements_table, element_offset));
    *element_handle = (ThsnValueHandle){.chunk_no = array_table->chunk_no,
                                        .offset = element_offset};
    BAIL_ON_ERROR(thsn_value_follow_handle(parsed_json, element_handle));
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_value_read_object(const ThsnParsedJson* parsed_json,
                                  ThsnValueHandle value_handle,
                                  ThsnValueObjectTable* object_table) {
    BAIL_ON_NULL_INPUT(object_table);
    return thsn_value_read_composite(parsed_json, value_handle, THSN_TAG_OBJECT,
                                     object_table);
}

size_t thsn_value_object_length(ThsnValueObjectTable object_table) {
    return thsn_value_array_length(object_table);
}

static ThsnResult thsn_value_object_read_kv(const ThsnParsedJson* parsed_json,
                                            ThsnValueHandle kv_handle,
                                            ThsnSlice* key_slice,
                                            ThsnValueHandle* value_handle) {
    BAIL_ON_NULL_INPUT(key_slice);
    BAIL_ON_NULL_INPUT(value_handle);
    size_t key_size;
    BAIL_ON_ERROR(thsn_value_read_string_ex(parsed_json, kv_handle, key_slice,
                                            &key_size));
    *value_handle = kv_handle;
    value_handle->offset += key_size;
    BAIL_ON_ERROR(thsn_value_follow_handle(parsed_json, value_handle));
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_value_object_element_handle(const ThsnParsedJson* parsed_json,
                                            ThsnValueObjectTable object_table,
                                            size_t element_no,
                                            ThsnSlice* key_slice,
                                            ThsnValueHandle* value_handle) {
    BAIL_ON_NULL_INPUT(key_slice);
    BAIL_ON_NULL_INPUT(value_handle);
    ThsnValueHandle kv_handle;
    BAIL_ON_ERROR(thsn_value_array_element_handle(parsed_json, object_table,
                                                  element_no, &kv_handle));
    return thsn_value_object_read_kv(parsed_json, kv_handle, key_slice,
                                     value_handle);
}

ThsnResult thsn_value_object_consume_element(const ThsnParsedJson* parsed_json,
                                             ThsnValueObjectTable* object_table,
                                             ThsnSlice* key_slice,
                                             ThsnValueHandle* value_handle) {
    BAIL_ON_NULL_INPUT(object_table);
    BAIL_ON_NULL_INPUT(key_slice);
    BAIL_ON_NULL_INPUT(value_handle);
    ThsnValueHandle kv_handle;
    BAIL_ON_ERROR(thsn_value_array_consume_element(parsed_json, object_table,
                                                   &kv_handle));
    return thsn_value_object_read_kv(parsed_json, kv_handle, key_slice,
                                     value_handle);
}

ThsnResult thsn_value_object_index(const ThsnParsedJson* parsed_json,
                                   ThsnValueObjectTable object_table,
                                   ThsnSlice key_slice,
                                   ThsnValueHandle* handle) {
    ThsnSlice element_key_slice;
    ThsnValueHandle element_value_handle;
    *handle = THSN_VALUE_HANDLE_NOT_FOUND;
    while (!thsn_slice_is_empty(object_table.elements_table)) {
        const size_t elements_count =
            object_table.elements_table.size / sizeof(size_t);
        const size_t midpoint = elements_count / 2;
        BAIL_ON_ERROR(thsn_value_object_element_handle(
            parsed_json, object_table, midpoint, &element_key_slice,
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
            BAIL_ON_ERROR(thsn_slice_truncate(&object_table.elements_table,
                                              midpoint * sizeof(size_t)));
        } else {
            BAIL_ON_ERROR(thsn_slice_at_offset(
                object_table.elements_table, (midpoint + 1) * sizeof(size_t),
                (elements_count - midpoint - 1) * sizeof(size_t),
                &object_table.elements_table));
        }
    }
    return THSN_RESULT_SUCCESS;
}
