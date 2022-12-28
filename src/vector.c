#include "vector.h"

ThsnVector thsn_vector_make_empty();

size_t thsn_vector_space_left(ThsnVector vector);

size_t thsn_vector_current_offset(ThsnVector vector);

bool thsn_vector_is_empty(ThsnVector vector);

ThsnSlice thsn_vector_as_slice(ThsnVector vector);

ThsnResult thsn_vector_allocate(ThsnVector* /*out*/ vector,
                                size_t prealloc_size);

ThsnResult thsn_vector_free(ThsnVector* /*in/out*/ vector);

ThsnResult thsn_vector_grow(ThsnVector* /*in/out*/ vector, size_t data_size,
                            ThsnMutSlice* /*out*/ data_mut_slice);

ThsnResult thsn_vector_shrink(ThsnVector* /*in/out*/ vector, size_t shrink_size,
                              ThsnSlice* /*out*/ data_slice);

ThsnResult thsn_vector_slice_at_offset(ThsnVector vector, size_t offset,
                                       size_t size, ThsnSlice* /*out*/ slice);

ThsnResult thsn_vector_mut_slice_at_offset(ThsnVector vector, size_t offset,
                                           size_t size,
                                           ThsnMutSlice* /*out*/ mut_slice);

ThsnResult thsn_vector_push(ThsnVector* /*in/out*/ vector, ThsnSlice slice);

ThsnResult thsn_vector_pop(ThsnVector* /*in/out*/ vector,
                           ThsnMutSlice mut_slice);
