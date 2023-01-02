#ifndef THSN_RESULT_H
#define THSN_RESULT_H

typedef enum {
    THSN_RESULT_SUCCESS,
    THSN_RESULT_OUT_OF_MEMORY_ERROR,
    THSN_RESULT_INPUT_ERROR,
} ThsnResult;

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

#define BAIL_ON_ERROR(r)                       \
    do {                                       \
        ThsnResult __result = (r);             \
        if (__result != THSN_RESULT_SUCCESS) { \
            return __result;                   \
        }                                      \
    } while (0)

#define GOTO_ON_ERROR(r, label)           \
    do {                                  \
        if ((r) != THSN_RESULT_SUCCESS) { \
            goto label;                   \
        }                                 \
    } while (0)

#endif