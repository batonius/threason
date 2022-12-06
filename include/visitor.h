#pragma once

#include <stdbool.h>

#include "result.h"
#include "slice.h"

typedef enum {
    THSN_VISITOR_RESULT_ABORT_ERROR,
    THSN_VISITOR_RESULT_ABORT_SUCCESS,
    THSN_VISITOR_RESULT_SKIP,
    THSN_VISITOR_RESULT_CONTINUE,
} ThsnVisitorResult;

typedef struct {
    ThsnSlice key;
    bool in_array;
    bool in_object;
    bool last;
} ThsnVisitorContext;

typedef struct {
    ThsnVisitorResult (*visit_integer)(ThsnVisitorContext* context,
                                       void* user_data, long long value);
    ThsnVisitorResult (*visit_double)(ThsnVisitorContext* context,
                                      void* user_data, double value);
    ThsnVisitorResult (*visit_null)(ThsnVisitorContext* context,
                                    void* user_data);
    ThsnVisitorResult (*visit_bool)(ThsnVisitorContext* context,
                                    void* user_data, bool value);
    ThsnVisitorResult (*visit_string)(ThsnVisitorContext* context,
                                      void* user_data, ThsnSlice value);
    ThsnVisitorResult (*visit_array_start)(ThsnVisitorContext* context,
                                           void* user_data);
    ThsnVisitorResult (*visit_array_end)(ThsnVisitorContext* context,
                                         void* user_data);
    ThsnVisitorResult (*visit_object_start)(ThsnVisitorContext* context,
                                            void* user_data);
    ThsnVisitorResult (*visit_object_end)(ThsnVisitorContext* context,
                                          void* user_data);
} ThsnVisitorVTable;

#define THSN_VISITOR_VTABLE_DEFAULT() \
    (ThsnVisitorVTable) {}

ThsnResult thsn_visit(ThsnSlice parsed_data, ThsnVisitorVTable* vtable,
                      void* user_data);
