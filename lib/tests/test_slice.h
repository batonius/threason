#ifndef THSN_TEST_SLICE_H
#define THSN_TEST_SLICE_H

#include "slice.h"
#include "testing.h"

TEST(creates_empty_slice) {
    ThsnSlice slice = thsn_slice_make_empty();
    ASSERT_EQ(slice.data, NULL);
    ASSERT_EQ(slice.size, 0);
}

TEST(creates_slice_from_fields) {
    ThsnSlice slice = thsn_slice_make(NULL, 0);
    ASSERT_EQ(slice.data, NULL);
    ASSERT_EQ(slice.size, 0);
    slice = thsn_slice_make((char*)&slice, sizeof(slice));
    ASSERT_EQ(slice.data, (char*)&slice);
    ASSERT_EQ(slice.size, sizeof(slice));
}

TEST(creates_slice_from_c_str) {
    const char* const string = "asdf";
    ThsnSlice slice = thsn_slice_from_c_str(string);
    ASSERT_EQ(slice.data, string);
    ASSERT_EQ(slice.size, 4);
    slice = thsn_slice_from_c_str(NULL);
    ASSERT_EQ(slice.data, NULL);
    ASSERT_EQ(slice.size, 0);
}

TEST(creates_slice_from_var) {
    uint8_t a = 10;
    ThsnSlice slice = THSN_SLICE_FROM_VAR(a);
    ASSERT_EQ(slice.data, (char*)&a);
    ASSERT_EQ(slice.size, sizeof(a));
    slice = THSN_SLICE_FROM_VAR(slice);
    ASSERT_EQ(slice.data, (char*)&slice);
    ASSERT_EQ(slice.size, sizeof(slice));
}

TEST(creates_slice_at_offset) {
    char array[10] = {0};
    ThsnSlice array_slice = THSN_SLICE_FROM_VAR(array);
    ThsnSlice slice;
    ASSERT_SUCCESS(thsn_slice_at_offset(array_slice, 5, 5, &slice));
    ASSERT_EQ(slice.data, &(array[5]));
    ASSERT_EQ(slice.size, 5);
    ASSERT_INPUT_ERROR(thsn_slice_at_offset(array_slice, 100, 0, &slice));
    ASSERT_INPUT_ERROR(thsn_slice_at_offset(array_slice, 5, 100, &slice));
}

TEST(checks_if_slice_is_empty) {
    ThsnSlice slice = thsn_slice_make_empty();
    ASSERT_EQ(thsn_slice_is_empty(slice), true);
    slice = THSN_SLICE_FROM_VAR(slice);
    ASSERT_EQ(thsn_slice_is_empty(slice), false);
}

TEST(advances_and_rewinds_slice) {
    char array[10] = {0};
    ThsnSlice array_slice = THSN_SLICE_FROM_VAR(array);
    thsn_slice_advance_unsafe(&array_slice, 10);
    ASSERT_EQ(array_slice.data, &(array[10]));
    ASSERT_EQ(array_slice.size, 0);
    thsn_slice_rewind_unsafe(&array_slice, 5);
    ASSERT_EQ(array_slice.data, &(array[5]));
    ASSERT_EQ(array_slice.size, 5);
}

TEST(advances_slice_by_char) {
    char array[] = {1, 2, 3, 4};
    ThsnSlice array_slice = THSN_SLICE_FROM_VAR(array);
    ASSERT_EQ(thsn_slice_advance_char_unsafe(&array_slice), 1);
    ASSERT_EQ(thsn_slice_advance_char_unsafe(&array_slice), 2);
    ASSERT_EQ(thsn_slice_advance_char_unsafe(&array_slice), 3);
    ASSERT_EQ(array_slice.data, &(array[3]));
    ASSERT_EQ(array_slice.size, 1);
}

TEST(creates_mut_slice) {
    char array[] = {1, 2, 3, 4};
    ThsnMutSlice mut_slice = thsn_mut_slice_make(array, sizeof(array));
    ASSERT_EQ(mut_slice.data, (char*)array);
    ASSERT_EQ(mut_slice.size, sizeof(array));
}

TEST(creates_mut_slice_from_var) {
    char array[10] = {0};
    ThsnMutSlice mut_slice = THSN_MUT_SLICE_FROM_VAR(array);
    ASSERT_EQ(mut_slice.data, (char*)array);
    ASSERT_EQ(mut_slice.size, sizeof(array));
}

TEST(writes_to_mut_slice) {
    char array[10] = {0};
    char x = 123;
    char y = 32;
    ThsnMutSlice mut_slice = THSN_MUT_SLICE_FROM_VAR(array);
    ASSERT_SUCCESS(thsn_mut_slice_write(&mut_slice, THSN_SLICE_FROM_VAR(x)));
    ASSERT_EQ(array[0], x);
    ASSERT_SUCCESS(THSN_MUT_SLICE_WRITE_VAR(mut_slice, y));
    ASSERT_SUCCESS(THSN_MUT_SLICE_WRITE_VAR(mut_slice, y));
    ASSERT_EQ(array[1], y);
    ASSERT_EQ(array[2], y);
    ASSERT_INPUT_ERROR(THSN_MUT_SLICE_WRITE_VAR(mut_slice, array));
    ASSERT_EQ(mut_slice.data, &array[3]);
    ASSERT_EQ(mut_slice.size, 7);
}

TEST(reads_from_slice) {
    const char array[] = {1, 2, 3, 4, 5};
    char x;
    ThsnSlice slice = THSN_SLICE_FROM_VAR(array);
    ASSERT_SUCCESS(thsn_slice_read(&slice, THSN_MUT_SLICE_FROM_VAR(x)));
    ASSERT_EQ(x, 1);
    ASSERT_SUCCESS(THSN_SLICE_READ_VAR(slice, x));
    ASSERT_EQ(x, 2);
    ASSERT_SUCCESS(THSN_SLICE_READ_VAR(slice, x));
    ASSERT_EQ(x, 3);
    ASSERT_INPUT_ERROR(THSN_SLICE_READ_VAR(slice, array));
    ASSERT_EQ(slice.data, &array[3]);
    ASSERT_EQ(slice.size, 2);
}

TEST(computes_the_end_of_a_slice) {
    ThsnSlice slice = thsn_slice_make_empty();
    ASSERT_EQ(thsn_slice_end(slice), 0);
    const char array[] = {1, 2, 3};
    slice = THSN_SLICE_FROM_VAR(array);
    ASSERT_EQ(thsn_slice_end(slice), array + 3);
}

TEST(truncates_slices) {
    ThsnSlice slice = thsn_slice_make_empty();
    ASSERT_NULL_INPUT_ERROR(thsn_slice_truncate(NULL, 0));
    ASSERT_SUCCESS(thsn_slice_truncate(&slice, 0));
    ASSERT_EQ(slice.data, NULL);
    ASSERT_EQ(slice.size, 0);
    ASSERT_INPUT_ERROR(thsn_slice_truncate(&slice, 1));
    ASSERT_EQ(slice.data, NULL);
    ASSERT_EQ(slice.size, 0);
    const char array[] = {1, 2, 3};
    slice = THSN_SLICE_FROM_VAR(array);
    ASSERT_SUCCESS(thsn_slice_truncate(&slice, 1));
    ASSERT_EQ(slice.data, array);
    ASSERT_EQ(slice.size, 1);
}

TEST(tries_to_consume_char) {
    ThsnSlice slice = thsn_slice_make_empty();
    char c;
    ASSERT_FALSE(thsn_slice_try_consume_char(NULL, &c));
    ASSERT_FALSE(thsn_slice_try_consume_char(&slice, NULL));
    ASSERT_FALSE(thsn_slice_try_consume_char(&slice, &c));
    const char array[] = {1, 2, 3};
    slice = THSN_SLICE_FROM_VAR(array);
    ASSERT_TRUE(thsn_slice_try_consume_char(&slice, &c));
    ASSERT_EQ(slice.data, array + 1);
    ASSERT_EQ(slice.size, 2);
    ASSERT_EQ(c, 1);
}

TEST(creates_mut_slice_at_offset) {
    char array[10] = {0};
    ThsnMutSlice array_slice = THSN_MUT_SLICE_FROM_VAR(array);
    ThsnMutSlice slice;
    ASSERT_SUCCESS(thsn_mut_slice_at_offset(array_slice, 5, 5, &slice));
    ASSERT_EQ(slice.data, &(array[5]));
    ASSERT_EQ(slice.size, 5);
    ASSERT_INPUT_ERROR(thsn_mut_slice_at_offset(array_slice, 100, 0, &slice));
    ASSERT_INPUT_ERROR(thsn_mut_slice_at_offset(array_slice, 5, 100, &slice));
}

/* clang-format off */

TEST_SUITE(slice)
    creates_empty_slice,
    creates_slice_from_fields,
    creates_slice_from_c_str,
    creates_slice_from_var,
    creates_slice_at_offset,
    checks_if_slice_is_empty,
    advances_and_rewinds_slice,
    advances_slice_by_char,
    creates_mut_slice,
    creates_mut_slice_from_var,
    writes_to_mut_slice,
    reads_from_slice,
    computes_the_end_of_a_slice,
    truncates_slices,
    tries_to_consume_char,
    creates_mut_slice_at_offset,
END_TEST_SUITE()

#endif
