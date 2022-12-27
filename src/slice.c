#include "slice.h"

ThsnResult thsn_slice_at_offset(ThsnSlice base_slice, size_t offset,
                                size_t min_size, ThsnSlice* slice_at_offset);

ThsnSlice thsn_slice_make_empty();

ThsnSlice thsn_slice_make(const char* data, size_t size);

ThsnResult thsn_slice_from_c_str(const char* data,
                                 ThsnSlice* /*out*/ result_slice);

bool thsn_slice_is_empty(ThsnSlice slice);

void thsn_slice_advance_unsafe(ThsnSlice* /*in/out*/ slice, size_t step);

char thsn_slice_advance_char_unsafe(ThsnSlice* /*in/out*/ slice);
