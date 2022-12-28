#ifndef THSN_TEST_VECTOR_H
#define THSN_TEST_VECTOR_H

#include "testing.h"
#include "vector.h"

TEST(makes_empty_vector) {
    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_EQ(thsn_vector_space_left(vector), 0);
    ASSERT_EQ(thsn_vector_current_offset(vector), 0);
    ASSERT_EQ(thsn_vector_is_empty(vector), true);
}

TEST(allocates_vector) {
    ThsnVector vector = {.buffer = NULL, .capacity = 0, .offset = 1};
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_NEQ(vector.buffer, NULL);
    ASSERT_EQ(vector.capacity, 1024);
    ASSERT_EQ(vector.offset, 0);
    ASSERT_EQ(thsn_vector_space_left(vector), 1024);
    ASSERT_EQ(thsn_vector_current_offset(vector), 0);
    ASSERT_EQ(thsn_vector_is_empty(vector), true);
    ASSERT_INPUT_ERROR(thsn_vector_allocate(NULL, 1024));
}

TEST(frees_vector) {
    ThsnVector vector = {.buffer = NULL, .capacity = 0, .offset = 1};
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_SUCCESS(thsn_vector_free(&vector));
    ASSERT_INPUT_ERROR(thsn_vector_free(NULL));
}

TEST(grows_vector) {
    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_SUCCESS(thsn_vector_grow(&vector, 1024, NULL));
    ASSERT_EQ(vector.offset, 1024);
    ThsnMutSlice mut_slice;
    ASSERT_SUCCESS(thsn_vector_grow(&vector, 1024, &mut_slice));
    ASSERT_EQ(vector.offset, 2048);
    ASSERT_EQ(mut_slice.size, 1024);
    ASSERT_NEQ(mut_slice.data, NULL);
    ASSERT_INPUT_ERROR(thsn_vector_grow(NULL, 1024, NULL));
    ASSERT_EQ(thsn_vector_grow(&vector, SIZE_MAX, NULL),
              THSN_RESULT_OUT_OF_MEMORY_ERROR);
}

TEST(shrinks_vector) {
    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1000));
    ASSERT_EQ(thsn_vector_space_left(vector), 1000);
    ASSERT_SUCCESS(thsn_vector_grow(&vector, 40, NULL));
    ASSERT_EQ(thsn_vector_space_left(vector), 960);
    ASSERT_SUCCESS(thsn_vector_shrink(&vector, 20, NULL));
    ASSERT_EQ(thsn_vector_space_left(vector), 980);
    ThsnSlice slice;
    ASSERT_SUCCESS(thsn_vector_shrink(&vector, 20, &slice));
    ASSERT_EQ(thsn_vector_space_left(vector), 1000);
    ASSERT_EQ(slice.size, 20);
    ASSERT_NEQ(slice.data, NULL);
    ASSERT_INPUT_ERROR(thsn_vector_shrink(NULL, 10, NULL));
}

TEST(pushes_and_pops) {
    ThsnVector vector = thsn_vector_make_empty();
    uint64_t a = 1;
    uint32_t b = 2;
    uint16_t c = 3;
    uint8_t d = 4;
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 0));
    ASSERT_SUCCESS(THSN_VECTOR_PUSH_VAR(vector, d));
    ASSERT_SUCCESS(THSN_VECTOR_PUSH_VAR(vector, c));
    ASSERT_SUCCESS(THSN_VECTOR_PUSH_VAR(vector, b));
    ASSERT_SUCCESS(THSN_VECTOR_PUSH_VAR(vector, a));
    a = b = c = d = 0;
    ASSERT_SUCCESS(THSN_VECTOR_POP_VAR(vector, a));
    ASSERT_SUCCESS(THSN_VECTOR_POP_VAR(vector, b));
    ASSERT_SUCCESS(THSN_VECTOR_POP_VAR(vector, c));
    ASSERT_SUCCESS(THSN_VECTOR_POP_VAR(vector, d));
    ASSERT_EQ(a, 1);
    ASSERT_EQ(b, 2);
    ASSERT_EQ(c, 3);
    ASSERT_EQ(d, 4);
}

// clang-format off

TEST_SUITE(vector)
    makes_empty_vector,
    allocates_vector,
    frees_vector,
    grows_vector,
    shrinks_vector,
    pushes_and_pops,
END_TEST_SUITE() 

#endif
