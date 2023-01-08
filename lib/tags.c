#include "tags.h"

ThsnTag thsn_tag_make(ThsnTagType type, ThsnTagSize size);

ThsnTagType thsn_tag_type(ThsnTag tag);

ThsnTagSize thsn_tag_size(ThsnTag tag);

ThsnResult thsn_vector_store_tagged_value(ThsnVector* vector, ThsnTag tag,
                                          ThsnSlice value_slice);

ThsnResult thsn_vector_store_null(ThsnVector* vector);

ThsnResult thsn_vector_store_bool(ThsnVector* vector, bool value);

ThsnResult thsn_vector_store_double(ThsnVector* vector, double value);

ThsnResult thsn_vector_store_int(ThsnVector* vector, long long value);

ThsnResult thsn_vector_store_value_handle(ThsnVector* vector,
                                          ThsnValueHandle value_handle);

ThsnResult thsn_vector_store_string(ThsnVector* vector, ThsnSlice string_slice);
