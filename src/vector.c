#include "vector.h"

ThsnResult thsn_vector_make(ThsnVector* /*out*/ vector, size_t prealloc_size) {
    BAIL_ON_NULL_INPUT(vector);
    vector->buffer = malloc(prealloc_size);
    BAIL_ON_ALLOC_FAILURE(vector->buffer);
    vector->capacity = prealloc_size;
    vector->offset = 0;
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_vector_free(ThsnVector* /*in/out*/ vector) {
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
ThsnResult thsn_vector_grow(ThsnVector* /*in/out*/ vector, size_t data_size) {
    BAIL_ON_NULL_INPUT(vector);
    const size_t allocated_space_left = THSN_VECTOR_SPACE_LEFT(*vector);
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
    vector->offset += data_size;
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_vector_shrink(ThsnVector* /*in/out*/ vector,
                              size_t shrink_size) {
    BAIL_ON_NULL_INPUT(vector);
    if (vector->offset < shrink_size) {
        return THSN_RESULT_INPUT_ERROR;
    }
    vector->offset -= shrink_size;
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_vector_push(ThsnVector* /*in/out*/ vector,
                            ThsnSlice /*in*/ slice) {
    BAIL_ON_NULL_INPUT(vector);
    BAIL_ON_NULL_INPUT(slice.data);
    size_t pregrow_offset = vector->offset;
    BAIL_ON_ERROR(thsn_vector_grow(vector, slice.size));
    memcpy(THSN_VECTOR_AT_OFFSET(*vector, pregrow_offset), slice.data,
           slice.size);
    return THSN_RESULT_SUCCESS;
}

ThsnResult thsn_vector_pop(ThsnVector* /*in/out*/ vector,
                           ThsnSlice /*out*/ slice) {
    BAIL_ON_NULL_INPUT(vector);
    BAIL_ON_NULL_INPUT(slice.data);
    BAIL_ON_ERROR(thsn_vector_shrink(vector, slice.size));
    memcpy(slice.data, THSN_VECTOR_AT_CURRENT_OFFSET(*vector), slice.size);
    return THSN_RESULT_SUCCESS;
}
