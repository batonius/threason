#include "tags.h"

ThsnResult thsn_vector_store_tagged_value(ThsnVector* /*in/out*/ vector,
                                          ThsnTag tag, ThsnSlice value_slice);

ThsnResult thsn_vector_store_null(ThsnVector* /*in/out*/ vector);

ThsnResult thsn_vector_store_bool(ThsnVector* /*in/out*/ vector, bool value);

ThsnResult thsn_vector_store_double(ThsnVector* /*in/out*/ vector,
                                    double value);

ThsnResult thsn_vector_store_int(ThsnVector* /*in/out*/ vector,
                                 long long value);

ThsnResult thsn_vector_store_string(ThsnVector* /*in/out*/ vector,
                                    ThsnSlice string_slice);

ThsnResult thsn_slice_read_string(ThsnSlice stored_string_slice,
                                  ThsnSlice* /*out*/ string_slice,
                                  size_t* /*out*/ stored_length);
