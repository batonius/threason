#include "slice.h"

ThsnSlice thsn_slice_make_empty();

ThsnSlice thsn_slice_make(const char* data, size_t size);

ThsnResult thsn_slice_from_c_str(const char* data,
                                 ThsnSlice* /*out*/ result_slice);

ThsnResult thsn_slice_at_offset(ThsnSlice base_slice, size_t offset,
                                size_t min_size,
                                ThsnSlice* /*out*/ slice_at_offset);

ThsnResult thsn_slice_truncate(ThsnSlice* slice, size_t exact_size);

bool thsn_slice_is_empty(ThsnSlice slice);

void thsn_slice_advance_unsafe(ThsnSlice* /*in/out*/ slice, size_t step);

char thsn_slice_advance_char_unsafe(ThsnSlice* /*in/out*/ slice);

bool thsn_slice_try_consume_char(ThsnSlice* /*in/out*/ slice, char* /*out*/ c);

ThsnMutSlice thsn_mut_slice_make(char* data, size_t size);

ThsnResult thsn_mut_slice_write(ThsnMutSlice* /*in/out*/ mut_slice,
                                ThsnSlice data_slice);

ThsnResult thsn_slice_read(ThsnSlice* data_slice, ThsnMutSlice mut_slice);
