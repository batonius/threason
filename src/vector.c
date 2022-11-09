#include "vector.h"

ThsnResult thsn_vector_make(ThsnVector* /*out*/ vector, size_t prealloc_size);

ThsnResult thsn_vector_free(ThsnVector* /*in/out*/ vector);

/* Invalidates all the pointers within the vector */
ThsnResult thsn_vector_grow(ThsnVector* /*in/out*/ vector, size_t data_size);

ThsnResult thsn_vector_shrink(ThsnVector* /*in/out*/ vector,
                              size_t shrink_size);

ThsnResult thsn_vector_push(ThsnVector* /*in/out*/ vector,
                            ThsnSlice /*in*/ slice);

ThsnResult thsn_vector_pop(ThsnVector* /*in/out*/ vector,
                           ThsnSlice /*out*/ slice);
