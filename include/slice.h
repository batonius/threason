#ifndef THSN_SLICE_H
#define THSN_SLICE_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "result.h"

typedef struct {
    size_t size;
    const char* data;
} ThsnSlice;

typedef struct {
    size_t size;
    char* data;
} ThsnMutSlice;

inline ThsnSlice thsn_slice_make_empty() {
    return (ThsnSlice){.data = NULL, .size = 0};
}

inline ThsnSlice thsn_slice_make(const char* data, size_t size) {
    return (ThsnSlice){.data = data, .size = size};
}

inline ThsnResult thsn_slice_from_c_str(const char* data,
                                        ThsnSlice* /*out*/ result_slice) {
    BAIL_ON_NULL_INPUT(data);
    BAIL_ON_NULL_INPUT(result_slice);
    *result_slice = (ThsnSlice){.data = data, .size = strlen(data)};
    return THSN_RESULT_SUCCESS;
}

#define THSN_SLICE_FROM_VAR(v) thsn_slice_make((const char*)&(v), sizeof(v))

inline ThsnResult thsn_slice_at_offset(ThsnSlice base_slice, size_t offset,
                                       size_t min_size,
                                       ThsnSlice* /*out*/ slice_at_offset) {
    BAIL_ON_NULL_INPUT(slice_at_offset);
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

inline bool thsn_slice_try_consume_char(ThsnSlice* /*in/out*/ slice,
                                        char* /*out*/ c) {
    BAIL_ON_NULL_INPUT(slice);
    BAIL_ON_NULL_INPUT(c);
    if (thsn_slice_is_empty(*slice)) {
        return false;
    }
    *c = thsn_slice_advance_char_unsafe(slice);
    return true;
}

inline ThsnMutSlice thsn_mut_slice_make(char* data, size_t size) {
    return (ThsnMutSlice){.data = data, .size = size};
}

#define THSN_MUT_SLICE_FROM_VAR(v) thsn_mut_slice_make((char*)&(v), sizeof(v))

inline ThsnResult thsn_mut_slice_write(ThsnMutSlice* /*in/out*/ mut_slice,
                                       ThsnSlice data_slice) {
    BAIL_ON_NULL_INPUT(mut_slice);

    if (mut_slice->size < data_slice.size) {
        return THSN_RESULT_INPUT_ERROR;
    }

    memcpy(mut_slice->data, data_slice.data, data_slice.size);
    mut_slice->data += data_slice.size;
    mut_slice->size -= data_slice.size;
    return THSN_RESULT_SUCCESS;
}

#define THSN_MUT_SLICE_WRITE_VAR(mut_slice, var) \
    thsn_mut_slice_write(&mut_slice, THSN_SLICE_FROM_VAR(var))

inline ThsnResult thsn_slice_read(ThsnSlice* data_slice,
                                  ThsnMutSlice mut_slice) {
    BAIL_ON_NULL_INPUT(data_slice);

    if (mut_slice.size > data_slice->size) {
        return THSN_RESULT_INPUT_ERROR;
    }

    memcpy(mut_slice.data, data_slice->data, mut_slice.size);
    data_slice->data += mut_slice.size;
    data_slice->size -= mut_slice.size;
    return THSN_RESULT_SUCCESS;
}

#define THSN_SLICE_READ_VAR(slice, var) \
    thsn_slice_read(&slice, THSN_MUT_SLICE_FROM_VAR(var))

#endif
