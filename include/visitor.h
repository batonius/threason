#ifndef THSN_VISITOR_H
#define THSN_VISITOR_H

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

#define THSN_VISITOR_VTABLE_DEFAULT() \
    (ThsnVisitorVTable) {}

ThsnResult thsn_visit(ThsnSlice parsed_data, const ThsnVisitorVTable* vtable,
                      void* user_data);

#endif
