#include "slice.h"

ThsnSlice thsn_slice_make_empty();

ThsnSlice thsn_slice_make(const char* data, size_t size);

const char* thsn_slice_end(ThsnSlice slice);

ThsnSlice thsn_slice_from_mut_slice(ThsnMutSlice mut_slice);

ThsnResult thsn_slice_from_c_str(const char* data, ThsnSlice* result_slice);

ThsnResult thsn_slice_at_offset(ThsnSlice base_slice, size_t offset,
                                size_t min_size, ThsnSlice* slice_at_offset);

ThsnResult thsn_slice_truncate(ThsnSlice* slice, size_t exact_size);

bool thsn_slice_is_empty(ThsnSlice slice);

void thsn_slice_advance_unsafe(ThsnSlice* slice, size_t step);

void thsn_slice_rewind_unsafe(ThsnSlice* slice, size_t step);

char thsn_slice_advance_char_unsafe(ThsnSlice* slice);

bool thsn_slice_try_consume_char(ThsnSlice* slice, char* c);

ThsnMutSlice thsn_mut_slice_make(char* data, size_t size);

ThsnMutSlice thsn_mut_slice_make_empty();

ThsnResult thsn_mut_slice_write(ThsnMutSlice* mut_slice, ThsnSlice data_slice);

ThsnResult thsn_mut_slice_at_offset(ThsnMutSlice base_slice, size_t offset,
                                    size_t min_size,
                                    ThsnMutSlice* mut_slice_at_offset);

ThsnResult thsn_slice_read(ThsnSlice* data_slice, ThsnMutSlice mut_slice);
