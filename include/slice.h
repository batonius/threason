#pragma once

#include <stdlib.h>

#include "result.h"

typedef struct {
    size_t size;
    char* data;
} ThsnSlice;

#define THSN_SLICE_MAKE_EMPTY() \
    (ThsnSlice) { .data = NULL, .size = 0 }

#define THSN_SLICE_MAKE(data_, size_) \
    (ThsnSlice) { .data = (data_), .size = (size_) }

#define THSN_SLICE_FROM_VAR(v) \
    (ThsnSlice) { .data = (char*)&(v), .size = sizeof(v) }

#define THSN_SLICE_FROM_C_STR(s) \
    (ThsnSlice) { .data = (s), .size = strlen((s)) }

#define THSN_SLICE_ADVANCE_UNSAFE(slice, n) \
    do {                                    \
        (slice).data += n;                  \
        (slice).size -= n;                  \
    } while (0)

#define THSN_SLICE_EMPTY(slice) ((slice).size == 0)

#define THSN_SLICE_CURRENT_CHAR(slice) (*(slice).data)

#define THSN_SLICE_NEXT_CHAR_UNSAFE(slice) (--(slice).size, *(slice).data++)

inline ThsnResult thsn_slice_at_offset(ThsnSlice base_slice, size_t offset,
                                       ThsnSlice* slice_at_offset) {
    if (offset >= base_slice.size) {
        return THSN_RESULT_INPUT_ERROR;
    }
    *slice_at_offset =
        THSN_SLICE_MAKE(base_slice.data + offset, base_slice.size - offset);
    return THSN_RESULT_SUCCESS;
}
