#ifndef THSN_TEST_VECTOR_H
#define THSN_TEST_VECTOR_H

#include "testing.h"
#include "vector.h"

TEST(makes_empty_vector) {
    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_EQ(thsn_vector_space_left(vector), 0);
    ASSERT_EQ(thsn_vector_current_offset(vector), 0);
    ASSERT_EQ(thsn_vector_is_empty(vector), true);
    ASSERT_SUCCESS(thsn_vector_free(&vector));
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
    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

TEST(frees_vector) {
    ThsnVector vector = {.buffer = NULL, .capacity = 0, .offset = 1};
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_SUCCESS(thsn_vector_free(&vector));
    ASSERT_INPUT_ERROR(thsn_vector_free(NULL));
    ASSERT_SUCCESS(thsn_vector_free(&vector));
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
    ASSERT_SUCCESS(thsn_vector_free(&vector));
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
    ASSERT_SUCCESS(thsn_vector_free(&vector));
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
    ASSERT_INPUT_ERROR(THSN_VECTOR_POP_VAR(vector, a));
    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

TEST(returns_slice_at_offset) {
    char array[] = {1, 2, 3};
    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_SUCCESS(THSN_VECTOR_PUSH_VAR(vector, array));
    ThsnSlice slice;
    ASSERT_SUCCESS(thsn_vector_slice_at_offset(vector, 1, 20, &slice));
    ASSERT_EQ(slice.size, 20);
    char x = 0;
    ASSERT_SUCCESS(THSN_SLICE_READ_VAR(slice, x));
    ASSERT_EQ(x, 2);
    ASSERT_SUCCESS(THSN_SLICE_READ_VAR(slice, x));
    ASSERT_EQ(x, 3);
    ASSERT_SUCCESS(thsn_vector_slice_at_offset(vector, 20, 20, &slice));
    ASSERT_INPUT_ERROR(
        thsn_vector_slice_at_offset(vector, 1, 1024 * 1024, &slice));
    ASSERT_INPUT_ERROR(
        thsn_vector_slice_at_offset(vector, 1024 * 1024, 1, &slice));
    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

TEST(returns_mut_slice_at_offset) {
    char array[] = {1, 2, 3};
    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_SUCCESS(THSN_VECTOR_PUSH_VAR(vector, array));
    ASSERT_EQ(*(vector.buffer + 1), 2);
    ThsnMutSlice mut_slice;
    ASSERT_SUCCESS(thsn_vector_mut_slice_at_offset(vector, 1, 2, &mut_slice));
    ASSERT_EQ(mut_slice.size, 2);
    char x = 20;
    ASSERT_SUCCESS(THSN_MUT_SLICE_WRITE_VAR(mut_slice, x));
    ASSERT_EQ(*(vector.buffer + 1), 20);
    ASSERT_INPUT_ERROR(
        thsn_vector_mut_slice_at_offset(vector, 1, 3, &mut_slice));
    ASSERT_INPUT_ERROR(
        thsn_vector_mut_slice_at_offset(vector, 3, 1, &mut_slice));
    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

/* clang-format off */

TEST_SUITE(vector)
    makes_empty_vector,
    allocates_vector,
    frees_vector,
    grows_vector,
    shrinks_vector,
    pushes_and_pops,
    returns_slice_at_offset,
    returns_mut_slice_at_offset,
END_TEST_SUITE() 

#endif
