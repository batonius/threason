#pragma once

#include <stdbool.h>

#include "result.h"
#include "slice.h"

typedef enum {
    THSN_VISITOR_RESULT_ABORT_ERROR,
    THSN_VISITOR_RESULT_ABORT_SUCCESS,
    THSN_VISITOR_RESULT_SKIP,
    THSN_VISITOR_RESULT_CONTINUE,
} thsn_visitor_result_t;

typedef struct {
    thsn_slice_t key;
    bool in_array;
    bool last;
} thsn_visitor_context_t;

typedef struct {
    thsn_visitor_result_t (*visit_integer)(thsn_visitor_context_t* context,
                                           void* user_data, long long value);
    thsn_visitor_result_t (*visit_float)(thsn_visitor_context_t* context,
                                         void* user_data, double value);
    thsn_visitor_result_t (*visit_null)(thsn_visitor_context_t* context,
                                        void* user_data);
    thsn_visitor_result_t (*visit_bool)(thsn_visitor_context_t* context,
                                        void* user_data, bool value);
    thsn_visitor_result_t (*visit_string)(thsn_visitor_context_t* context,
                                        void* user_data, thsn_slice_t value);
    thsn_visitor_result_t (*visit_array_start)(thsn_visitor_context_t* context,
                                               void* user_data);
    thsn_visitor_result_t (*visit_array_end)(thsn_visitor_context_t* context,
                                             void* user_data);
    thsn_visitor_result_t (*visit_object_start)(thsn_visitor_context_t* context,
                                                void* user_data);
    thsn_visitor_result_t (*visit_object_end)(thsn_visitor_context_t* context,
                                              void* user_data);
} thsn_visitor_vtable_t;

#define THSN_VISITOR_VTABLE_DEFAULT() \
    (thsn_visitor_vtable_t) {}

thsn_result_t thsn_visit(thsn_slice_t parsed_data,
                         thsn_visitor_vtable_t* vtable, void* user_data);
