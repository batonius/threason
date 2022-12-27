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
    ThsnSlice slice;
    ASSERT_SUCCESS(thsn_slice_from_c_str(string, &slice));
    ASSERT_EQ(slice.data, string);
    ASSERT_EQ(slice.size, 4);
    ASSERT_INPUT_ERROR(thsn_slice_from_c_str(NULL, &slice));
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

TEST(advances_slice) {
    char array[10] = {0};
    ThsnSlice array_slice = THSN_SLICE_FROM_VAR(array);
    thsn_slice_advance_unsafe(&array_slice, 10);
    ASSERT_EQ(array_slice.data, &(array[10]));
    ASSERT_EQ(array_slice.size, 0);
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

// clang-format off

TEST_SUITE(slice)
    creates_empty_slice,
    creates_slice_from_fields,
    creates_slice_from_c_str,
    creates_slice_from_var,
    creates_slice_at_offset,
    checks_if_slice_is_empty,
    advances_slice,
    advances_slice_by_char,
END_TEST_SUITE()

#endif
