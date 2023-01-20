#ifndef THREASON_H
#define THREASON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    THSN_RESULT_SUCCESS,
    THSN_RESULT_OUT_OF_MEMORY_ERROR,
    THSN_RESULT_INPUT_ERROR,
    THSN_RESULT_NULL_INPUT_ERROR,
} ThsnResult;

typedef enum {
    THSN_VISITOR_RESULT_ABORT_ERROR,
    THSN_VISITOR_RESULT_ABORT_SUCCESS,
    THSN_VISITOR_RESULT_SKIP,
    THSN_VISITOR_RESULT_CONTINUE,
} ThsnVisitorResult;

typedef enum {
    THSN_VALUE_NULL,
    THSN_VALUE_BOOL,
    THSN_VALUE_NUMBER,
    THSN_VALUE_STRING,
    THSN_VALUE_ARRAY,
    THSN_VALUE_OBJECT,
} ThsnValueType;

typedef struct {
    uint8_t segment_no;
    uint64_t offset : 56;
} ThsnValueHandle;

static inline ThsnValueHandle thsn_value_handle_first(void) {
    return (ThsnValueHandle){0};
}

static inline ThsnValueHandle thsn_value_handle_not_found(void) {
    return (ThsnValueHandle){.segment_no = UINT8_MAX, .offset = 0};
}

static inline bool thsn_value_handle_is_not_found(
    ThsnValueHandle value_handle) {
    return value_handle.segment_no == UINT8_MAX;
}

typedef struct {
    size_t size;
    const char* data;
} ThsnSlice;

typedef struct {
    size_t size;
    char* data;
} ThsnMutSlice;

static inline ThsnSlice thsn_slice_make(const char* data, size_t size) {
    return (ThsnSlice){.size = size, .data = data};
}

static inline ThsnSlice thsn_slice_from_c_str(const char* data) {
    return thsn_slice_make(data, data == NULL ? 0 : strlen(data));
}

typedef ThsnMutSlice ThsnOwningMutSlice;
typedef ThsnSlice ThsnOwningSlice;

typedef struct {
    ThsnSlice key;
    bool in_array;
    bool in_object;
    bool last;
} ThsnVisitorContext;

typedef struct {
    size_t segment_count;
    ThsnOwningMutSlice segments[];
} ThsnDocument;

typedef struct {
    uint8_t segment_no;
    ThsnSlice elements_table;
} ThsnValueCompositeTable;

typedef ThsnValueCompositeTable ThsnValueArrayTable;
typedef ThsnValueCompositeTable ThsnValueObjectTable;

typedef struct {
    ThsnVisitorResult (*visit_number)(const ThsnVisitorContext* context,
                                      void* user_data, double value);
    ThsnVisitorResult (*visit_null)(const ThsnVisitorContext* context,
                                    void* user_data);
    ThsnVisitorResult (*visit_bool)(const ThsnVisitorContext* context,
                                    void* user_data, bool value);
    ThsnVisitorResult (*visit_string)(const ThsnVisitorContext* context,
                                      void* user_data, ThsnSlice value);
    ThsnVisitorResult (*visit_array_start)(const ThsnVisitorContext* context,
                                           void* user_data);
    ThsnVisitorResult (*visit_array_end)(const ThsnVisitorContext* context,
                                         void* user_data);
    ThsnVisitorResult (*visit_object_start)(const ThsnVisitorContext* context,
                                            void* user_data);
    ThsnVisitorResult (*visit_object_end)(const ThsnVisitorContext* context,
                                          void* user_data);
} ThsnVisitorVTable;

extern ThsnResult thsn_document_free(ThsnDocument** /*in*/ document);

extern ThsnResult thsn_document_parse(ThsnSlice* /*mut*/ json_str_slice,
                                      ThsnDocument** /*out*/ document);

extern ThsnResult thsn_document_parse_multithreaded(
    ThsnSlice* /*mut*/ json_str_slice, ThsnDocument** /*out*/ document,
    size_t threads_count);

extern ThsnResult thsn_document_visit(ThsnDocument* /*mut*/ document,
                                      const ThsnVisitorVTable* /*in*/ vtable,
                                      void* /*in*/ user_data);

extern ThsnResult thsn_document_value_type(const ThsnDocument* /*in*/ document,
                                           ThsnValueHandle value_handle,
                                           ThsnValueType* /*out*/ value_type);

extern ThsnResult thsn_document_read_bool(const ThsnDocument* /*in*/ document,
                                          ThsnValueHandle value_handle,
                                          bool* /*out*/ value);

extern ThsnResult thsn_document_read_number(const ThsnDocument* /*in*/ document,
                                            ThsnValueHandle value_handle,
                                            double* /*out*/ value);

extern ThsnResult thsn_document_read_string(const ThsnDocument* /*in*/ document,
                                            ThsnValueHandle value_handle,
                                            ThsnSlice* /*out*/ string_slice);

extern ThsnResult thsn_document_read_array(
    ThsnDocument* /*in*/ document, ThsnValueHandle value_handle,
    ThsnValueArrayTable* /*out*/ array_table);

extern size_t thsn_document_array_length(ThsnValueArrayTable array_table);

extern ThsnResult thsn_document_index_array_element(
    const ThsnDocument* /*in*/ document, ThsnValueArrayTable array_table,
    size_t element_no, ThsnValueHandle* /*out*/ element_handle);

extern ThsnResult thsn_document_array_consume_element(
    const ThsnDocument* /*in*/ document,
    ThsnValueArrayTable* /*mut*/ array_table,
    ThsnValueHandle* /*out*/ element_handle);

extern ThsnResult thsn_document_read_object(
    ThsnDocument* /*mut*/ document, ThsnValueHandle value_handle,
    ThsnValueObjectTable* /*out*/ object_table);

extern ThsnResult thsn_document_read_object_sorted(
    ThsnDocument* /*mut*/ document, ThsnValueHandle value_handle,
    ThsnValueObjectTable* /*out*/ object_table);

extern size_t thsn_document_object_length(ThsnValueObjectTable object_table);

extern ThsnResult thsn_document_object_index_element(
    const ThsnDocument* /*in*/ document, ThsnValueObjectTable object_table,
    size_t element_no, ThsnSlice* /*out*/ key_str_slice,
    ThsnValueHandle* /*out*/ element_handle);

extern ThsnResult thsn_document_object_consume_element(
    const ThsnDocument* /*in*/ document,
    ThsnValueObjectTable* /*mut*/ object_table, ThsnSlice* /*out*/ key_slice,
    ThsnValueHandle* /*out*/ element_handle);

extern ThsnResult thsn_document_object_index(
    const ThsnDocument* /*in*/ document, ThsnValueObjectTable object_table,
    ThsnSlice key_slice, ThsnValueHandle* /*out*/ element_handle);

#ifdef __cplusplus
}
#endif

#endif
