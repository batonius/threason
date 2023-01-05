#include "parser.h"

typedef struct {
    size_t buffer_offset;
    size_t size;
    size_t value_offset;
} ThsnPreparsedValue;
