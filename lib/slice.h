#ifndef THSN_SLICE_H
#define THSN_SLICE_H

#include <stdbool.h>
#include <stdlib.h>

#include "result.h"
#include "threason.h"

#define THSN_SLICE_FROM_VAR(v) thsn_slice_make((const char*)&(v), sizeof(v))

#define THSN_MUT_SLICE_FROM_VAR(v) thsn_mut_slice_make((char*)&(v), sizeof(v))

#define THSN_MUT_SLICE_WRITE_VAR(mut_slice, var) \
    thsn_mut_slice_write(&mut_slice, THSN_SLICE_FROM_VAR(var))

#define THSN_SLICE_READ_VAR(slice, var) \
    thsn_slice_read(&slice, THSN_MUT_SLICE_FROM_VAR(var))

static inline ThsnSlice thsn_slice_make_empty(void) {
    return (ThsnSlice){.data = NULL, .size = 0};
}

static inline const char* thsn_slice_end(ThsnSlice slice) {
    return slice.data + slice.size;
}

static inline ThsnSlice thsn_slice_from_mut_slice(ThsnMutSlice mut_slice) {
    return thsn_slice_make(mut_slice.data, mut_slice.size);
}

static inline ThsnResult thsn_slice_at_offset(
    ThsnSlice base_slice, size_t offset, size_t min_size,
    ThsnSlice* /*out*/ slice_at_offset) {
    BAIL_ON_NULL_INPUT(slice_at_offset);
    BAIL_WITH_INPUT_ERROR_UNLESS(offset <= base_slice.size &&
                                 (base_slice.size - offset) >= min_size);
    *slice_at_offset =
        thsn_slice_make(base_slice.data + offset, base_slice.size - offset);
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_slice_truncate(ThsnSlice* /*mut*/ slice,
                                             size_t exact_size) {
    BAIL_ON_NULL_INPUT(slice);
    BAIL_WITH_INPUT_ERROR_UNLESS(exact_size <= slice->size);
    slice->size = exact_size;
    return THSN_RESULT_SUCCESS;
}

static inline bool thsn_slice_is_empty(ThsnSlice slice) {
    return slice.size == 0;
}

static inline void thsn_slice_advance_unsafe(ThsnSlice* /*mut*/ slice,
                                             size_t step) {
    slice->data += step;
    slice->size -= step;
}

static inline void thsn_slice_rewind_unsafe(ThsnSlice* /*mut*/ slice,
                                            size_t step) {
    slice->data -= step;
    slice->size += step;
}

static inline char thsn_slice_advance_char_unsafe(ThsnSlice* /*mut*/ slice) {
    --slice->size;
    return *slice->data++;
}

static inline bool thsn_slice_try_consume_char(ThsnSlice* /*mut*/ slice,
                                               char* c) {
    if (slice == NULL || c == NULL || thsn_slice_is_empty(*slice)) {
        return false;
    }
    *c = thsn_slice_advance_char_unsafe(slice);
    return true;
}

static inline ThsnMutSlice thsn_mut_slice_make(char* data, size_t size) {
    return (ThsnMutSlice){.data = data, .size = size};
}

static inline ThsnMutSlice thsn_mut_slice_make_empty(void) {
    return (ThsnMutSlice){.data = NULL, .size = 0};
}

static inline ThsnResult thsn_mut_slice_write(ThsnMutSlice* /*mut*/ mut_slice,
                                              ThsnSlice data_slice) {
    BAIL_ON_NULL_INPUT(mut_slice);
    BAIL_WITH_INPUT_ERROR_UNLESS(mut_slice->size >= data_slice.size);

    if (data_slice.size == 0) {
        return THSN_RESULT_SUCCESS;
    }

    memcpy(mut_slice->data, data_slice.data, data_slice.size);
    mut_slice->data += data_slice.size;
    mut_slice->size -= data_slice.size;
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_mut_slice_at_offset(
    ThsnMutSlice base_slice, size_t offset, size_t min_size,
    ThsnMutSlice* /*out*/ mut_slice_at_offset) {
    BAIL_ON_NULL_INPUT(mut_slice_at_offset);
    BAIL_WITH_INPUT_ERROR_UNLESS(offset <= base_slice.size &&
                                 (base_slice.size - offset) >= min_size);
    *mut_slice_at_offset =
        thsn_mut_slice_make(base_slice.data + offset, base_slice.size - offset);
    return THSN_RESULT_SUCCESS;
}

static inline ThsnResult thsn_slice_read(ThsnSlice* data_slice,
                                         ThsnMutSlice mut_slice) {
    BAIL_ON_NULL_INPUT(data_slice);
    BAIL_WITH_INPUT_ERROR_UNLESS(mut_slice.size <= data_slice->size);

    memcpy(mut_slice.data, data_slice->data, mut_slice.size);
    data_slice->data += mut_slice.size;
    data_slice->size -= mut_slice.size;
    return THSN_RESULT_SUCCESS;
}

#endif
