#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "result.h"

typedef struct {
    size_t size;
    const char* data;
} ThsnSlice;

inline ThsnSlice thsn_slice_make_empty() {
    return (ThsnSlice){.data = NULL, .size = 0};
}

inline ThsnSlice thsn_slice_make(const char* data, size_t size) {
    return (ThsnSlice){.data = data, .size = size};
}

inline ThsnSlice thsn_slice_from_c_str(const char* data) {
    return (ThsnSlice){.data = data, .size = strlen(data)};
}

inline ThsnResult thsn_slice_at_offset(ThsnSlice base_slice, size_t offset,
                                       size_t min_size,
                                       ThsnSlice* /*out*/ slice_at_offset) {
    if (offset >= base_slice.size || (base_slice.size - offset) < min_size) {
        return THSN_RESULT_INPUT_ERROR;
    }
    *slice_at_offset =
        thsn_slice_make(base_slice.data + offset, base_slice.size - offset);
    return THSN_RESULT_SUCCESS;
}

inline bool thsn_slice_is_empty(ThsnSlice slice) { return slice.size == 0; }

inline void thsn_slice_advance_unsafe(ThsnSlice* /*in/out*/ slice,
                                      size_t step) {
    slice->data += step;
    slice->size -= step;
}

inline char thsn_slice_advance_char_unsafe(ThsnSlice* /*in/out*/ slice) {
    --slice->size;
    return *slice->data++;
}

#define THSN_SLICE_FROM_VAR(v) thsn_slice_make((const char*)&(v), sizeof(v))
