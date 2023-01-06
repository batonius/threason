#ifndef THREASON_H
#define THREASON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*!
    \file threason.h
    \brief A JSON-parsing library.
*/

/*! Function result, should not be ignored. */
typedef enum {
    THSN_RESULT_SUCCESS,
    THSN_RESULT_OUT_OF_MEMORY_ERROR,
    THSN_RESULT_INPUT_ERROR,
} ThsnResult;

/*! Controls enumeration from within visitor functions. Defaults to
 * `::THSN_VISITOR_RESULT_CONTINUE`. */
typedef enum {
    THSN_VISITOR_RESULT_ABORT_ERROR,
    THSN_VISITOR_RESULT_ABORT_SUCCESS,
    THSN_VISITOR_RESULT_SKIP,
    THSN_VISITOR_RESULT_CONTINUE,
} ThsnVisitorResult;

/*! Basic JSON types. */
typedef enum {
    THSN_VALUE_NULL,
    THSN_VALUE_BOOL,
    THSN_VALUE_NUMBER,
    THSN_VALUE_STRING,
    THSN_VALUE_ARRAY,
    THSN_VALUE_OBJECT,
} ThsnValueType;

/*! A unique way to refer to a JSON value in `::ThsnParsedJson`. Should never be
 * constructed manually. */
typedef struct {
    uint8_t chunk_no;
    uint64_t offset : 56;
} ThsnValueHandle;

/*! The first value in the parsed document, the top level of the JSON tree. */
#define THSN_VALUE_HANDLE_FIRST \
    (ThsnValueHandle) { 0 }

/*! Used in case the key wasn't found. */
#define THSN_VALUE_HANDLE_NOT_FOUND \
    (ThsnValueHandle) { .chunk_no = -1, .offset = -1 }

#define THSN_VALUE_HANDLE_IS_NOT_FOUND(v) ((v).chunk_no == (uint8_t)-1)

/*! Poor man's &[u8]. */
typedef struct {
    size_t size;
    const char* data;
} ThsnSlice;

/*! Value's context for `::thsn_visit` enumeration */
typedef struct {
    ThsnSlice key;
    bool in_array;
    bool in_object;
    bool last;
} ThsnVisitorContext;

typedef ThsnSlice ThsnOwningSlice;

/*! */
typedef struct {
    size_t chunks_count;
    ThsnOwningSlice chunks[];
} ThsnParsedJson;

/*! A slice with array elements, should only be used with `thsn_value_array_*`
 * functions. */
typedef struct {
    uint8_t chunk_no;
    ThsnSlice elements_table;
} ThsnValueCompositeTable;

typedef ThsnValueCompositeTable ThsnValueArrayTable;
typedef ThsnValueCompositeTable ThsnValueObjectTable;

/*! VTable to be used by `::thsn_visit`, `NULL` fields are ignored. */
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

/*! Allocate space for parsing result */
extern ThsnResult thsn_parsed_json_allocate(ThsnParsedJson** parsed_json,
                                            uint8_t chunks_count);

/*! Free parsing result */
extern ThsnResult thsn_parsed_json_free(ThsnParsedJson** /*in*/ parsed_json);

/*! Parse JSON string into JSON tree. */
extern ThsnResult thsn_parse_buffer(ThsnSlice* json_str_slice,
                                    ThsnParsedJson* /*out*/ parsed_json);

extern ThsnResult thsn_parse_thread_per_chunk(
    ThsnSlice* json_str_slice, ThsnParsedJson* /*out*/ parsed_json);

/*! Enumerate JSON tree using function from vtable */
extern ThsnResult thsn_visit(const ThsnParsedJson* parsed_json,
                             const ThsnVisitorVTable* vtable, void* user_data);

/*! Return JSON type of a value identified by `value_handle`. */
extern ThsnResult thsn_value_type(const ThsnParsedJson* parsed_json,
                                  ThsnValueHandle value_handle,
                                  ThsnValueType* /*out*/ value_type);

/*! Read JSON value at `value_handle` as a bool. */
extern ThsnResult thsn_value_read_bool(const ThsnParsedJson* parsed_json,
                                       ThsnValueHandle value_handle,
                                       bool* /*out*/ value);

/*! Read JSON value at `value_handle` as a number. */
extern ThsnResult thsn_value_read_number(const ThsnParsedJson* parsed_json,
                                         ThsnValueHandle value_handle,
                                         double* /*out*/ value);

/*! Read JSON value at `value_handle` as a string. */
extern ThsnResult thsn_value_read_string(const ThsnParsedJson* parsed_json,
                                         ThsnValueHandle value_handle,
                                         ThsnSlice* /*out*/ string_slice);

/*! Read JSON value at `value_handle` as an array. */
extern ThsnResult thsn_value_read_array(
    const ThsnParsedJson* parsed_json, ThsnValueHandle value_handle,
    ThsnValueArrayTable* /*out*/ array_table);

/*! Return number of elements in the array. */
extern size_t thsn_value_array_length(ThsnValueArrayTable array_table);

/*! Return handle for the n-th element of the array. */
extern ThsnResult thsn_value_array_element_handle(
    const ThsnParsedJson* parsed_json, ThsnValueArrayTable array_table,
    size_t element_no, ThsnValueHandle* /*out*/ element_handle);

/*! Return the first element in the array, moving the array_table beyond it.
 */
extern ThsnResult thsn_value_array_consume_element(
    const ThsnParsedJson* parsed_json,
    ThsnValueArrayTable* /*in/out*/ array_table,
    ThsnValueHandle* /*out*/ element_handle);

/*! Read JSON value at `value_handle` as an object. */
extern ThsnResult thsn_value_read_object(
    const ThsnParsedJson* parsed_json, ThsnValueHandle value_handle,
    ThsnValueObjectTable* /*out*/ object_table);

/*! Return number of elements in the object. */
extern size_t thsn_value_object_length(ThsnValueObjectTable object_table);

/*! Return key and handle for the n-th element of the object. */
extern ThsnResult thsn_value_object_element_handle(
    const ThsnParsedJson* parsed_json, ThsnValueObjectTable object_table,
    size_t element_no, ThsnSlice* /*out*/ key_str_slice,
    ThsnValueHandle* /*out*/ element_handle);

/*! Return the first element in the object, moving the object_table beyond it.
 */
extern ThsnResult thsn_value_object_consume_element(
    const ThsnParsedJson* parsed_json, ThsnValueObjectTable* object_table,
    ThsnSlice* /*out*/ key_slice, ThsnValueHandle* /*out*/ element_handle);

/*! Return value handle for the object's field with key equal to key_slice.
 * Sets handle to THSN_VALUE_HANDLE_NOT_FOUND if not found.*/
extern ThsnResult thsn_value_object_index(
    const ThsnParsedJson* parsed_json, ThsnValueObjectTable object_table,
    ThsnSlice key_slice, ThsnValueHandle* /*out*/ element_handle);
#endif
