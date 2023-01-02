#ifndef THSN_TEST_TAGS_H
#define THSN_TEST_TAGS_H

#include <string.h>

#include "tags.h"
#include "testing.h"

TEST(stores_null) {
    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_SUCCESS(thsn_vector_store_null(&vector));
    ASSERT_EQ(vector.offset, 1);
    ASSERT_EQ(vector.buffer[0],
              thsn_tag_make(THSN_TAG_NULL, THSN_TAG_SIZE_EMPTY));
}

TEST(stores_bools) {
    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_SUCCESS(thsn_vector_store_bool(&vector, true));
    ASSERT_SUCCESS(thsn_vector_store_bool(&vector, false));
    ASSERT_SUCCESS(thsn_vector_store_bool(&vector, false));
    ASSERT_SUCCESS(thsn_vector_store_bool(&vector, true));
    ASSERT_EQ(vector.offset, 4);
    ASSERT_EQ(vector.buffer[0],
              thsn_tag_make(THSN_TAG_BOOL, THSN_TAG_SIZE_TRUE));
    ASSERT_EQ(vector.buffer[1],
              thsn_tag_make(THSN_TAG_BOOL, THSN_TAG_SIZE_FALSE));
    ASSERT_EQ(vector.buffer[2],
              thsn_tag_make(THSN_TAG_BOOL, THSN_TAG_SIZE_FALSE));
    ASSERT_EQ(vector.buffer[3],
              thsn_tag_make(THSN_TAG_BOOL, THSN_TAG_SIZE_TRUE));
}

TEST(stores_ints) {
    int8_t a;
    int16_t b;
    int32_t c;
    int64_t d;
    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_SUCCESS(thsn_vector_store_int(&vector, 123));
    ASSERT_SUCCESS(thsn_vector_store_int(&vector, 12345));
    ASSERT_SUCCESS(thsn_vector_store_int(&vector, 1234567));
    ASSERT_SUCCESS(thsn_vector_store_int(&vector, INT64_MAX));
    ASSERT_SUCCESS(thsn_vector_store_int(&vector, 0));
    ASSERT_EQ(vector.offset, 5 + 1 + 2 + 4 + 8);
    ASSERT_EQ(vector.buffer[0], thsn_tag_make(THSN_TAG_INT, sizeof(int8_t)));
    memcpy(&a, vector.buffer + 1, sizeof(a));
    ASSERT_EQ(a, 123);
    ASSERT_EQ(vector.buffer[2], thsn_tag_make(THSN_TAG_INT, sizeof(int16_t)));
    memcpy(&b, vector.buffer + 3, sizeof(b));
    ASSERT_EQ(b, 12345);
    ASSERT_EQ(vector.buffer[5], thsn_tag_make(THSN_TAG_INT, sizeof(int32_t)));
    memcpy(&c, vector.buffer + 6, sizeof(c));
    ASSERT_EQ(c, 1234567);
    ASSERT_EQ(vector.buffer[10],
              thsn_tag_make(THSN_TAG_INT, sizeof(long long)));
    memcpy(&d, vector.buffer + 11, sizeof(d));
    ASSERT_EQ(d, INT64_MAX);
    ASSERT_EQ(vector.buffer[19],
              thsn_tag_make(THSN_TAG_INT, THSN_TAG_SIZE_ZERO));
}

TEST(stores_strings) {
    char* short_string = "a short string";
    char* long_string = "a somewhat longer yet not very long string";
    ThsnVector vector = thsn_vector_make_empty();
    ThsnSlice short_string_slice;
    ThsnSlice long_string_slice;
    ASSERT_SUCCESS(thsn_slice_from_c_str(short_string, &short_string_slice));
    ASSERT_SUCCESS(thsn_slice_from_c_str(long_string, &long_string_slice));
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_SUCCESS(thsn_vector_store_string(&vector, short_string_slice));
    ASSERT_SUCCESS(thsn_vector_store_string(&vector, long_string_slice));
    ASSERT_EQ(vector.buffer[0],
              thsn_tag_make(THSN_TAG_SMALL_STRING, short_string_slice.size));
    ASSERT_STRN_EQ(vector.buffer + 1, short_string, short_string_slice.size);
    ASSERT_EQ(vector.buffer[1 + short_string_slice.size],
              thsn_tag_make(THSN_TAG_REF_STRING, THSN_TAG_SIZE_INBOUND));
    ASSERT_EQ(memcmp(vector.buffer + 2 + short_string_slice.size,
                     &long_string_slice, sizeof(long_string_slice)),
              0);
}

/* clang-format off */

TEST_SUITE(tags)
    stores_null,
    stores_bools, 
    stores_ints, 
    stores_strings,
END_TEST_SUITE()

#endif
