#pragma once

#include "result.h"
#include "slice.h"
#include "stdbool.h"
#include "vector.h"

typedef enum {
    THSN_TAG_NULL,
    THSN_TAG_BOOL,
    THSN_TAG_SMALL_STRING,
    THSN_TAG_REF_STRING,
    THSN_TAG_INT,
    THSN_TAG_DOUBLE,
    THSN_TAG_ARRAY,
    THSN_TAG_OBJECT,
} thsn_tag_type_t;

typedef uint8_t thsn_tag_t;

#define THSN_TAG_SIZE_FALSE 0
#define THSN_TAG_SIZE_TRUE 1
#define THSN_TAG_SIZE_EMPTY 0
#define THSN_TAG_SIZE_ZERO 0
#define THSN_TAG_SIZE_F64 0
#define THSN_TAG_SIZE_MAX 0xf
#define THSN_TAG_SIZE_INBOUND 1
#define THSN_TAG_SIZE_NEAR_OFFSET 1
#define THSN_TAG_SIZE_FAR_OFFSET 1

#define THSN_TAG_MAKE(type, size) (thsn_tag_t)(((type) << 4) | ((size)&0x0f))
#define THSN_TAG_TYPE(tag) ((tag) >> 4)
#define THSN_TAG_SIZE(tag) ((tag) &0x0f)

thsn_result_t thsn_vector_store_tagged_value(thsn_vector_t* /*in/out*/ vector,
                                             thsn_tag_t tag,
                                             thsn_slice_t value_slice);

thsn_result_t thsn_vector_store_null(thsn_vector_t* /*in/out*/ vector);

thsn_result_t thsn_vector_store_bool(thsn_vector_t* /*in/out*/ vector,
                                     bool value);

thsn_result_t thsn_vector_store_double(thsn_vector_t* /*in/out*/ vector,
                                       double value);

thsn_result_t thsn_vector_store_int(thsn_vector_t* /*in/out*/ vector,
                                    long long value);

thsn_result_t thsn_vector_store_string(thsn_vector_t* /*in/out*/ vector,
                                       thsn_slice_t string_slice);
