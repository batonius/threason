#pragma once

#include "result.h"
#include "slice.h"
#include "vector.h"

thsn_result_t thsn_parse_value(thsn_slice_t* /*in/out*/ buffer_slice,
                               thsn_vector_t* /*in/out*/ result_vector);
