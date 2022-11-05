#include "slice.h"

thsn_result_t thsn_slice_at_offset(thsn_slice_t base_slice, size_t offset,
                                   thsn_slice_t* slice_at_offset) {
    if (offset >= base_slice.size) {
        return THSN_RESULT_INPUT_ERROR;
    }
    *slice_at_offset =
        THSN_SLICE_MAKE(base_slice.data + offset, base_slice.size - offset);
    return THSN_RESULT_SUCCESS;
}
