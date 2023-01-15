#include <stddef.h>

#include "parser.h"
#include "result.h"
#include "segment.h"
#include "slice.h"
#include "threason.h"

/* Used for sorting */
_Thread_local ThsnSlice* CURRENT_SEGMENT = NULL;

ThsnResult thsn_document_free(ThsnDocument** /*in*/ document) {
    BAIL_ON_NULL_INPUT(document);
    for (size_t i = 0; i < (*document)->segment_count; ++i) {
        free((*document)->segments[i].data);
    }
    free(*document);
    *document = NULL;
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_document_parse(ThsnSlice* /*mut*/ buffer_slice,
                               ThsnDocument** /*out*/ document) {
    BAIL_ON_NULL_INPUT(buffer_slice);
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_ERROR(thsn_document_allocate(document, 1));
    ThsnToken token;
    ThsnSlice token_slice;
    ThsnParserContext parser_context;
    BAIL_ON_ERROR(thsn_parser_context_init(&parser_context));
    bool finished = false;
    while (!finished) {
        GOTO_ON_ERROR(thsn_next_token(buffer_slice, &token_slice, &token),
                      error_cleanup);
        GOTO_ON_ERROR(thsn_parser_parse_next_token(&parser_context, token,
                                                   token_slice, &finished),
                      error_cleanup);
    }
    GOTO_ON_ERROR(
        thsn_parser_context_finish(&parser_context, &(*document)->segments[0]),
        error_cleanup);
    return THSN_RESULT_SUCCESS;

error_cleanup:
    thsn_parser_context_finish(&parser_context, NULL);
    return THSN_RESULT_INPUT_ERROR;
}

static ThsnResult thsn_document_read_tagged_value(
    const ThsnDocument* /*in*/ document, ThsnValueHandle value_handle,
    ThsnTag* /*out*/ value_tag, ThsnSlice* /*out*/ value_slice) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(value_tag);
    BAIL_ON_NULL_INPUT(value_slice);
    BAIL_WITH_INPUT_ERROR_UNLESS(value_handle.segment_no <
                                 document->segment_count);
    return thsn_segment_read_tagged_value(
        thsn_slice_from_mut_slice(document->segments[value_handle.segment_no]),
        value_handle.offset, value_tag, value_slice);
}

static ThsnResult thsn_document_follow_handle(
    const ThsnDocument* /*in*/ document,
    ThsnValueHandle* /*mut*/ value_handle) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(value_handle);
    ThsnTag value_tag;
    ThsnSlice value_slice;
    while (true) {
        BAIL_ON_ERROR(thsn_document_read_tagged_value(
            document, *value_handle, &value_tag, &value_slice));
        if (thsn_tag_type(value_tag) == THSN_TAG_VALUE_HANDLE) {
            BAIL_ON_ERROR(THSN_SLICE_READ_VAR(value_slice, *value_handle));
        } else {
            break;
        }
    }
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_document_value_type(const ThsnDocument* /*in*/ document,
                                    ThsnValueHandle value_handle,
                                    ThsnValueType* /*out*/ value_type) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(value_type);
    ThsnTag value_tag;
    ThsnSlice value_slice;
    BAIL_ON_ERROR(thsn_document_read_tagged_value(document, value_handle,
                                                  &value_tag, &value_slice));
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

ThsnResult thsn_document_read_bool(const ThsnDocument* /*in*/ document,
                                   ThsnValueHandle value_handle,
                                   bool* /*out*/ value) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(value);
    BAIL_WITH_INPUT_ERROR_UNLESS(value_handle.segment_no <
                                 document->segment_count);
    return thsn_segment_read_bool(
        thsn_slice_from_mut_slice(document->segments[value_handle.segment_no]),
        value_handle.offset, value);
}

ThsnResult thsn_document_read_number(const ThsnDocument* /*in*/ document,
                                     ThsnValueHandle value_handle,
                                     double* /*out*/ value) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(value);
    BAIL_WITH_INPUT_ERROR_UNLESS(value_handle.segment_no <
                                 document->segment_count);
    return thsn_segment_read_number(
        thsn_slice_from_mut_slice(document->segments[value_handle.segment_no]),
        value_handle.offset, value);
}

ThsnResult thsn_document_read_string(const ThsnDocument* /*in*/ document,
                                     ThsnValueHandle value_handle,
                                     ThsnSlice* /*out*/ string_slice) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(string_slice);
    BAIL_WITH_INPUT_ERROR_UNLESS(value_handle.segment_no <
                                 document->segment_count);
    return thsn_segment_read_string_ex(
        thsn_slice_from_mut_slice(document->segments[value_handle.segment_no]),
        value_handle.offset, string_slice, NULL);
}

static ThsnResult thsn_document_read_composite(
    ThsnDocument* /*mut*/ document, ThsnValueHandle value_handle,
    ThsnTagType expected_type, ThsnValueCompositeTable* /*out*/ composite_table,
    bool read_sorted_table) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(composite_table);
    BAIL_WITH_INPUT_ERROR_UNLESS(value_handle.segment_no <
                                 document->segment_count);
    composite_table->segment_no = value_handle.segment_no;
    return thsn_segment_read_composite(
        document->segments[value_handle.segment_no], value_handle.offset,
        expected_type, &composite_table->elements_table, read_sorted_table);
}

ThsnResult thsn_document_read_array(ThsnDocument* /*mut*/ document,
                                    ThsnValueHandle value_handle,
                                    ThsnValueArrayTable* /*out*/ array_table) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(array_table);
    return thsn_document_read_composite(document, value_handle, THSN_TAG_ARRAY,
                                        array_table, false);
}

size_t thsn_document_array_length(ThsnValueArrayTable array_table) {
    return array_table.elements_table.size / sizeof(size_t);
}

ThsnResult thsn_document_index_array_element(
    const ThsnDocument* /*in*/ document, ThsnValueArrayTable array_table,
    size_t element_no, ThsnValueHandle* /*out*/ element_handle) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(element_handle);
    size_t element_offset = 0;
    BAIL_ON_ERROR(thsn_segment_composite_index_element_offset(
        array_table.elements_table, element_no, &element_offset));
    *element_handle = (ThsnValueHandle){.segment_no = array_table.segment_no,
                                        .offset = element_offset};
    BAIL_ON_ERROR(thsn_document_follow_handle(document, element_handle));
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_document_array_consume_element(
    const ThsnDocument* /*in*/ document,
    ThsnValueArrayTable* /*mut*/ array_table,
    ThsnValueHandle* /*out*/ element_handle) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(array_table);
    BAIL_ON_NULL_INPUT(element_handle);
    size_t element_offset = 0;
    BAIL_ON_ERROR(thsn_segment_composite_consume_element_offset(
        &array_table->elements_table, &element_offset));
    *element_handle = (ThsnValueHandle){.segment_no = array_table->segment_no,
                                        .offset = element_offset};
    BAIL_ON_ERROR(thsn_document_follow_handle(document, element_handle));
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_document_read_object(
    ThsnDocument* /*mut*/ document, ThsnValueHandle value_handle,
    ThsnValueObjectTable* /*out*/ object_table) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(object_table);
    return thsn_document_read_composite(document, value_handle, THSN_TAG_OBJECT,
                                        object_table, false);
}

ThsnResult thsn_document_read_object_sorted(
    ThsnDocument* /*mut*/ document, ThsnValueHandle value_handle,
    ThsnValueObjectTable* /*out*/ sorted_object_table) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(sorted_object_table);
    return thsn_document_read_composite(document, value_handle, THSN_TAG_OBJECT,
                                        sorted_object_table, true);
}

size_t thsn_document_object_length(ThsnValueObjectTable object_table) {
    return thsn_document_array_length(object_table);
}

static ThsnResult thsn_document_object_read_kv(
    const ThsnDocument* /*in*/ document, ThsnValueHandle kv_handle,
    ThsnSlice* /*out*/ key_slice, ThsnValueHandle* /*out*/ element_handle) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(key_slice);
    BAIL_ON_NULL_INPUT(element_handle);
    size_t element_offset = 0;
    BAIL_WITH_INPUT_ERROR_UNLESS(kv_handle.segment_no <
                                 document->segment_count);
    BAIL_ON_ERROR(thsn_segment_object_read_kv(
        thsn_slice_from_mut_slice(document->segments[kv_handle.segment_no]),
        kv_handle.offset, key_slice, &element_offset));
    *element_handle = (ThsnValueHandle){.segment_no = kv_handle.segment_no,
                                        .offset = element_offset};
    BAIL_ON_ERROR(thsn_document_follow_handle(document, element_handle));
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_document_object_index_element(
    const ThsnDocument* /*in*/ document, ThsnValueObjectTable object_table,
    size_t element_no, ThsnSlice* /*out*/ key_slice,
    ThsnValueHandle* /*out*/ element_handle) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(key_slice);
    BAIL_ON_NULL_INPUT(element_handle);
    ThsnValueHandle kv_handle;
    BAIL_ON_ERROR(thsn_document_index_array_element(document, object_table,
                                                    element_no, &kv_handle));
    return thsn_document_object_read_kv(document, kv_handle, key_slice,
                                        element_handle);
}

ThsnResult thsn_document_object_consume_element(
    const ThsnDocument* /*in*/ document,
    ThsnValueObjectTable* /*mut*/ object_table, ThsnSlice* /*out*/ key_slice,
    ThsnValueHandle* /*out*/ value_handle) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(object_table);
    BAIL_ON_NULL_INPUT(key_slice);
    BAIL_ON_NULL_INPUT(value_handle);
    ThsnValueHandle kv_handle;
    BAIL_ON_ERROR(thsn_document_array_consume_element(document, object_table,
                                                      &kv_handle));
    return thsn_document_object_read_kv(document, kv_handle, key_slice,
                                        value_handle);
}

ThsnResult thsn_document_object_index(const ThsnDocument* /*in*/ document,
                                      ThsnValueObjectTable object_table,
                                      ThsnSlice key_slice,
                                      ThsnValueHandle* /*out*/ element_handle) {
    BAIL_ON_NULL_INPUT(document);
    BAIL_ON_NULL_INPUT(element_handle);
    BAIL_ON_NULL_INPUT(document);
    BAIL_WITH_INPUT_ERROR_UNLESS(object_table.segment_no <
                                 document->segment_count);
    size_t element_offset = 0;
    bool found = false;
    BAIL_ON_ERROR(thsn_segment_object_index(
        thsn_slice_from_mut_slice(document->segments[object_table.segment_no]),
        object_table.elements_table, key_slice, &element_offset, &found));

    if (!found) {
        *element_handle = thsn_value_handle_not_found();
    } else {
        *element_handle = (ThsnValueHandle){
            .segment_no = object_table.segment_no, .offset = element_offset};
    }
    return THSN_RESULT_SUCCESS;
}
