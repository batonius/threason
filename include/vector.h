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
} thsn_vector_t;

#define THSN_VECTOR_INIT() \
    (thsn_vector_t) { .buffer = NULL, .capacity = 0, .offset = 0 }
#define THSN_VECTOR_SPACE_LEFT(vector) ((vector).capacity - (vector).offset)
#define THSN_VECTOR_OFFSET(vector) ((vector).offset)
#define THSN_VECTOR_AT_CURRENT_OFFSET(vector) \
    ((vector).buffer + (vector).offset)
#define THSN_VECTOR_AT_OFFSET(vector, offset) ((vector).buffer + (offset))
#define THSN_VECTOR_EMPTY(vector) ((vector).offset == 0)
#define THSN_VECTOR_AS_SLICE(vector) \
    THSN_SLICE_MAKE((vector).buffer, (vector).offset)

thsn_result_t thsn_vector_make(thsn_vector_t* /*out*/ vector,
                               size_t prealloc_size);

thsn_result_t thsn_vector_free(thsn_vector_t* /*in/out*/ vector);

/* Invalidates all the pointers within the vector */
thsn_result_t thsn_vector_grow(thsn_vector_t* /*in/out*/ vector,
                               size_t data_size);

thsn_result_t thsn_vector_shrink(thsn_vector_t* /*in/out*/ vector,
                                 size_t shrink_size);

thsn_result_t thsn_vector_push(thsn_vector_t* /*in/out*/ vector,
                               thsn_slice_t /*in*/ slice);

#define THSN_VECTOR_PUSH_VAR(vector, var) \
    thsn_vector_push(&(vector), THSN_SLICE_FROM_VAR(var))

thsn_result_t thsn_vector_pop(thsn_vector_t* /*in/out*/ vector,
                              thsn_slice_t /*out*/ slice);

#define THSN_VECTOR_POP_VAR(vector, var) \
    thsn_vector_pop(&(vector), THSN_SLICE_FROM_VAR(var))
