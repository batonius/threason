#ifndef THSN_VISITOR_H
#define THSN_VISITOR_H

#include <stdbool.h>

#include "result.h"
#include "slice.h"
#include "threason.h"

#define THSN_VISITOR_VTABLE_DEFAULT() \
    (ThsnVisitorVTable) {}

ThsnResult thsn_visit(ThsnSlice parsed_data, const ThsnVisitorVTable* vtable,
                      void* user_data);

#endif
