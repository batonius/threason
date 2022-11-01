#pragma once

#include <stdlib.h>

typedef struct {
    char* data;
    size_t size;
} thsn_slice_t;

#define THSN_SLICE_MAKE(data, size) \
    (thsn_slice_t) { .data = (data), .size = (size) }

#define THSN_SLICE_FROM_VAR(v) \
    (thsn_slice_t) { .data = (char*)&(v), .size = sizeof(v) }

#define THSN_SLICE_ADVANCE_UNSAFE(slice, n) \
    do {                                    \
        (slice).data += n;                  \
        (slice).size -= n;                  \
    } while (0)

#define THSN_SLICE_EMPTY(slice) ((slice).size == 0)

#define THSN_SLICE_NEXT_CHAR_UNSAFE(slice) \
    ((slice).size--, *(slice).data++)
