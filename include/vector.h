#ifndef THSN_VECTOR_H
#define THSN_VECTOR_H

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

#define THSN_VECTOR_PUSH_VAR(vector, var) \
    thsn_vector_push(&(vector), THSN_SLICE_FROM_VAR(var))

#define THSN_VECTOR_PUSH_2_VARS(vector, var1, var2)               \
    THSN_VECTOR_PUSH_VAR((vector), (var1)) == THSN_RESULT_SUCCESS \
        ? THSN_VECTOR_PUSH_VAR((vector), (var2))                  \
        : THSN_RESULT_INPUT_ERROR

#define THSN_VECTOR_PUSH_3_VARS(vector, var1, var2, var3)         \
    THSN_VECTOR_PUSH_VAR((vector), (var1)) == THSN_RESULT_SUCCESS \
        ? THSN_VECTOR_PUSH_2_VARS((vector), (var2), (var3))       \
        : THSN_RESULT_INPUT_ERROR

#define THSN_VECTOR_POP_VAR(vector, var) \
    thsn_vector_pop(&(vector), (char*)&(var), sizeof(var))

#define THSN_VECTOR_POP_2_VARS(vector, var1, var2)               \
    THSN_VECTOR_POP_VAR((vector), (var1)) == THSN_RESULT_SUCCESS \
        ? THSN_VECTOR_POP_VAR((vector), (var2))                  \
        : THSN_RESULT_INPUT_ERROR

#define THSN_VECTOR_POP_3_VARS(vector, var1, var2, var3)         \
    THSN_VECTOR_POP_VAR((vector), (var1)) == THSN_RESULT_SUCCESS \
        ? THSN_VECTOR_POP_2_VARS((vector), (var2), (var3))       \
        : THSN_RESULT_INPUT_ERROR

inline ThsnVector thsn_vector_make_empty() {
    return (ThsnVector){.buffer = NULL, .capacity = 0, .offset = 0};
}

inline size_t thsn_vector_space_left(ThsnVector vector) {
    return vector.capacity - vector.offset;
}

inline size_t thsn_vector_current_offset(ThsnVector vector) {
    return vector.offset;
}

inline bool thsn_vector_is_empty(ThsnVector vector) {
    return vector.offset == 0;
}

inline ThsnSlice thsn_vector_as_slice(ThsnVector vector) {
    return thsn_slice_make(vector.buffer, vector.offset);
}

inline ThsnResult thsn_vector_allocate(ThsnVector* /*out*/ vector,
                                       size_t prealloc_size) {
    BAIL_ON_NULL_INPUT(vector);
    vector->buffer = malloc(prealloc_size);
    BAIL_ON_ALLOC_FAILURE(vector->buffer);
    vector->capacity = prealloc_size;
    vector->offset = 0;
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_vector_free(ThsnVector* /*in/out*/ vector) {
    BAIL_ON_NULL_INPUT(vector);
    if (vector->buffer != NULL) {
        free(vector->buffer);
        vector->buffer = NULL;
    }
    vector->capacity = 0;
    vector->offset = 0;
    return THSN_RESULT_SUCCESS;
}

/* Invalidates all the pointers within the vector */
inline ThsnResult thsn_vector_grow(ThsnVector* /*in/out*/ vector,
                                   size_t data_size,
                                   size_t* /*out*/ new_data_offset) {
    BAIL_ON_NULL_INPUT(vector);
    const size_t allocated_space_left = thsn_vector_space_left(*vector);
    if (allocated_space_left < data_size) {
        const size_t addr_space_left = SIZE_MAX - vector->offset;
        if (addr_space_left < data_size) {
            return THSN_RESULT_OUT_OF_MEMORY_ERROR;
        }
        const size_t grow_size =
            vector->capacity < data_size || vector->capacity > addr_space_left
                ? data_size
                : vector->capacity;
        vector->buffer = realloc(vector->buffer, vector->capacity + grow_size);
        BAIL_ON_ALLOC_FAILURE(vector->buffer);
        vector->capacity += grow_size;
    }
    if (new_data_offset != NULL) {
        *new_data_offset = vector->offset;
    }
    vector->offset += data_size;
    return THSN_RESULT_SUCCESS;
}

/* Never deallocates */
inline ThsnResult thsn_vector_shrink(ThsnVector* /*in/out*/ vector,
                                     size_t shrink_size,
                                     size_t* /*out*/ existing_data_offset) {
    BAIL_ON_NULL_INPUT(vector);
    if (vector->offset < shrink_size) {
        return THSN_RESULT_INPUT_ERROR;
    }
    vector->offset -= shrink_size;
    if (existing_data_offset != NULL) {
        *existing_data_offset = vector->offset;
    }
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_vector_data_at_offset(ThsnVector vector, size_t offset,
                                             size_t size, char** data) {
    BAIL_ON_NULL_INPUT(data);
    if (offset > vector.capacity || size > (vector.capacity - offset)) {
        return THSN_RESULT_INPUT_ERROR;
    }
    *data = vector.buffer + offset;
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_vector_push(ThsnVector* /*in/out*/ vector,
                                   ThsnSlice slice) {
    BAIL_ON_NULL_INPUT(vector);
    BAIL_ON_NULL_INPUT(slice.data);
    size_t new_data_offset;
    BAIL_ON_ERROR(thsn_vector_grow(vector, slice.size, &new_data_offset));
    char* new_data;
    BAIL_ON_ERROR(thsn_vector_data_at_offset(*vector, new_data_offset,
                                             slice.size, &new_data));
    memcpy(new_data, slice.data, slice.size);
    return THSN_RESULT_SUCCESS;
}

inline ThsnResult thsn_vector_pop(ThsnVector* /*in/out*/ vector,
                                  char* /*out*/ data, size_t size) {
    BAIL_ON_NULL_INPUT(vector);
    BAIL_ON_NULL_INPUT(data);
    size_t existing_data_offset;
    BAIL_ON_ERROR(thsn_vector_shrink(vector, size, &existing_data_offset));
    char* existing_data;
    BAIL_ON_ERROR(thsn_vector_data_at_offset(*vector, existing_data_offset,
                                             size, &existing_data));
    memcpy(data, existing_data, size);
    return THSN_RESULT_SUCCESS;
}

#endif
