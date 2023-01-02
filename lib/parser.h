#ifndef THSN_PARSER_H
#define THSN_PARSER_H

#include "result.h"
#include "slice.h"
#include "vector.h"

ThsnResult thsn_parse_value(ThsnSlice* /*in/out*/ buffer_slice,
                            ThsnVector* /*in/out*/ result_vector);

#endif
