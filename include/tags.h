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
} ThsnTagType;

typedef uint8_t ThsnTag;

#define THSN_TAG_SIZE_FALSE 0
#define THSN_TAG_SIZE_TRUE 1
#define THSN_TAG_SIZE_EMPTY 0
#define THSN_TAG_SIZE_ZERO 0
#define THSN_TAG_SIZE_F64 0
#define THSN_TAG_SIZE_MAX 0xf
#define THSN_TAG_SIZE_INBOUND 1
#define THSN_TAG_SIZE_NEAR_OFFSET 1
#define THSN_TAG_SIZE_FAR_OFFSET 1

#define THSN_TAG_MAKE(type, size) (ThsnTag)(((type) << 4) | ((size)&0x0f))
#define THSN_TAG_TYPE(tag) ((tag) >> 4)
#define THSN_TAG_SIZE(tag) ((tag)&0x0f)

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
