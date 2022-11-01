#pragma once

typedef enum {
    THSN_RESULT_SUCCESS,
    THSN_RESULT_OUT_OF_MEMORY_ERROR,
    THSN_RESULT_INPUT_ERROR,
} thsn_result_t;

#define BAIL_ON_NULL_INPUT(v)               \
    do {                                    \
        if (v == NULL) {                    \
            return THSN_RESULT_INPUT_ERROR; \
        }                                   \
    } while (0)

#define BAIL_ON_ALLOC_FAILURE(v)                    \
    do {                                            \
        if (v == NULL) {                            \
            return THSN_RESULT_OUT_OF_MEMORY_ERROR; \
        }                                           \
    } while (0)

#define BAIL_ON_ERROR(r)                     \
    do {                                     \
        thsn_result_t result = (r);          \
        if (result != THSN_RESULT_SUCCESS) { \
            return result;                   \
        }                                    \
    } while (0)
