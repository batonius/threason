#pragma once

#include <stdlib.h>

#include "result.h"

typedef struct {
    size_t size;
    char* data;
} thsn_slice_t;

#define THSN_SLICE_MAKE_EMPTY() \
    (thsn_slice_t) { .data = NULL, .size = 0 }

#define THSN_SLICE_MAKE(data_, size_) \
    (thsn_slice_t) { .data = (data_), .size = (size_) }

#define THSN_SLICE_FROM_VAR(v) \
    (thsn_slice_t) { .data = (char*)&(v), .size = sizeof(v) }

#define THSN_SLICE_FROM_C_STR(s) \
    (thsn_slice_t) { .data = (s), .size = strlen((s)) }

#define THSN_SLICE_ADVANCE_UNSAFE(slice, n) \
    do {                                    \
        (slice).data += n;                  \
        (slice).size -= n;                  \
    } while (0)

#define THSN_SLICE_EMPTY(slice) ((slice).size == 0)

#define THSN_SLICE_CURRENT_CHAR(slice) (*(slice).data)

#define THSN_SLICE_NEXT_CHAR_UNSAFE(slice) (--(slice).size, *(slice).data++)

thsn_result_t thsn_slice_at_offset(thsn_slice_t base_slice, size_t offset,
                                   thsn_slice_t* slice_at_offset);
