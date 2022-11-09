#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "result.h"
#include "slice.h"

typedef struct {
    char* buffer;
    size_t capacity;
    size_t offset;
} ThsnVector;

#define THSN_VECTOR_INIT() \
    (ThsnVector) { .buffer = NULL, .capacity = 0, .offset = 0 }
#define THSN_VECTOR_SPACE_LEFT(vector) ((vector).capacity - (vector).offset)
#define THSN_VECTOR_OFFSET(vector) ((vector).offset)
#define THSN_VECTOR_AT_CURRENT_OFFSET(vector) \
    ((vector).buffer + (vector).offset)
#define THSN_VECTOR_AT_OFFSET(vector, offset) ((vector).buffer + (offset))
#define THSN_VECTOR_EMPTY(vector) ((vector).offset == 0)
#define THSN_VECTOR_AS_SLICE(vector) \
    THSN_SLICE_MAKE((vector).buffer, (vector).offset)

ThsnResult thsn_vector_make(ThsnVector* /*out*/ vector, size_t prealloc_size);

ThsnResult thsn_vector_free(ThsnVector* /*in/out*/ vector);

/* Invalidates all the pointers within the vector */
ThsnResult thsn_vector_grow(ThsnVector* /*in/out*/ vector, size_t data_size);

ThsnResult thsn_vector_shrink(ThsnVector* /*in/out*/ vector,
                              size_t shrink_size);

ThsnResult thsn_vector_push(ThsnVector* /*in/out*/ vector,
                            ThsnSlice /*in*/ slice);

#define THSN_VECTOR_PUSH_VAR(vector, var) \
    thsn_vector_push(&(vector), THSN_SLICE_FROM_VAR(var))

ThsnResult thsn_vector_pop(ThsnVector* /*in/out*/ vector,
                           ThsnSlice /*out*/ slice);

#define THSN_VECTOR_POP_VAR(vector, var) \
    thsn_vector_pop(&(vector), THSN_SLICE_FROM_VAR(var))
