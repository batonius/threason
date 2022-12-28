#include "vector.h"

ThsnResult thsn_vector_allocate(ThsnVector* /*out*/ vector,
                                size_t prealloc_size);

ThsnResult thsn_vector_free(ThsnVector* /*in/out*/ vector);

ThsnResult thsn_vector_grow(ThsnVector* /*in/out*/ vector, size_t data_size,
                            size_t* /*out*/ new_data_ptr);

ThsnResult thsn_vector_shrink(ThsnVector* /*in/out*/ vector, size_t shrink_size,
                              size_t* /*out*/ existing_data_ptr);

ThsnResult thsn_vector_push(ThsnVector* /*in/out*/ vector,
                            ThsnSlice /*in*/ slice);

ThsnResult thsn_vector_pop(ThsnVector* /*in/out*/ vector, char* data,
                           size_t size);

ThsnVector thsn_vector_make_empty();

size_t thsn_vector_space_left(ThsnVector vector);

size_t thsn_vector_current_offset(ThsnVector vector);

bool thsn_vector_is_empty(ThsnVector vector);

ThsnSlice thsn_vector_as_slice(ThsnVector vector);

ThsnResult thsn_vector_data_at_offset(ThsnVector vector, size_t offset,
                                      size_t size, char** data);
