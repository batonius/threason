#ifndef THSN_TEST_SEGMENT_H
#define THSN_TEST_SEGMENT_H

#include <limits.h>
#include <math.h>
#include <string.h>

#include "segment.h"
#include "testing.h"

TEST(stores_and_reads_null) {
    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_NULL_INPUT_ERROR(thsn_segment_store_null(NULL));
    ASSERT_SUCCESS(thsn_segment_store_null(&vector));
    ThsnTag tag;
    ThsnSlice value_slice;
    ASSERT_SUCCESS(thsn_segment_read_tagged_value(thsn_vector_as_slice(vector),
                                                  0, &tag, &value_slice));
    ASSERT_EQ(tag, thsn_tag_make(THSN_TAG_NULL, THSN_TAG_SIZE_EMPTY));
    ASSERT_EQ(value_slice.size, 0);
    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

TEST(stores_and_reads_bools) {
    ASSERT_NULL_INPUT_ERROR(thsn_segment_store_bool(NULL, false));

    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));

    ASSERT_SUCCESS(thsn_segment_store_bool(&vector, true));
    ASSERT_SUCCESS(thsn_segment_store_bool(&vector, false));
    ASSERT_SUCCESS(thsn_segment_store_bool(&vector, false));
    ASSERT_SUCCESS(thsn_segment_store_bool(&vector, true));
    ThsnSlice slice = thsn_vector_as_slice(vector);
    bool value;
    ASSERT_NULL_INPUT_ERROR(thsn_segment_read_bool(slice, 0, NULL));

    ASSERT_SUCCESS(thsn_segment_read_bool(slice, 0, &value));
    ASSERT_TRUE(value);
    ASSERT_SUCCESS(thsn_segment_read_bool(slice, 1, &value));
    ASSERT_FALSE(value);
    ASSERT_SUCCESS(thsn_segment_read_bool(slice, 2, &value));
    ASSERT_FALSE(value);
    ASSERT_SUCCESS(thsn_segment_read_bool(slice, 3, &value));
    ASSERT_TRUE(value);

    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

TEST(stores_and_reads_doubles) {
    ASSERT_NULL_INPUT_ERROR(thsn_segment_store_double(NULL, 0.0));

    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));

    double values_to_test[] = {123.456, 0.0, 10e100, INFINITY};
    size_t values_offsets[sizeof(values_to_test) / sizeof(values_to_test[0])] =
        {0};

    for (size_t i = 0; i < sizeof(values_to_test) / sizeof(values_to_test[0]);
         ++i) {
        values_offsets[i] = thsn_vector_current_offset(vector);
        ASSERT_SUCCESS(thsn_segment_store_double(&vector, values_to_test[i]));
    }

    ThsnSlice slice = thsn_vector_as_slice(vector);

    for (size_t i = 0; i < sizeof(values_to_test) / sizeof(values_to_test[0]);
         ++i) {
        double value;
        ASSERT_SUCCESS(
            thsn_segment_read_number(slice, values_offsets[i], &value));
        ASSERT_EQ(value, values_to_test[i]);
    }

    ASSERT_NULL_INPUT_ERROR(thsn_segment_read_number(slice, 0, NULL));

    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

TEST(stores_and_reads_ints) {
    ASSERT_NULL_INPUT_ERROR(thsn_segment_store_int(NULL, 0));

    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));

    long long values_to_test[] = {0,       1,         127,      INT_MAX,
                                  INT_MIN, LLONG_MAX, LLONG_MIN};
    size_t values_offsets[sizeof(values_to_test) / sizeof(values_to_test[0])] =
        {0};

    for (size_t i = 0; i < sizeof(values_to_test) / sizeof(values_to_test[0]);
         ++i) {
        values_offsets[i] = thsn_vector_current_offset(vector);
        ASSERT_SUCCESS(thsn_segment_store_int(&vector, values_to_test[i]));
    }

    ThsnSlice slice = thsn_vector_as_slice(vector);

    for (size_t i = 0; i < sizeof(values_to_test) / sizeof(values_to_test[0]);
         ++i) {
        double value;
        ASSERT_SUCCESS(
            thsn_segment_read_number(slice, values_offsets[i], &value));
        ASSERT_EQ(value, (double)values_to_test[i]);
    }

    ASSERT_NULL_INPUT_ERROR(thsn_segment_read_number(slice, 0, NULL));

    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

TEST(stores_and_reads_strings) {
    ASSERT_NULL_INPUT_ERROR(
        thsn_segment_store_string(NULL, thsn_slice_make_empty()));

    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));

    const char* values_to_test[] = {
        "", "a", "longer string",
        ("vvvvvvvvveeeeeeeeeeeeerrrrrrrrryyyyyyyyyyy"
         "y looonnnnnngggg strrrrriiiinnnnnnngggg"),
        "\n\t\""};

    size_t values_offsets[sizeof(values_to_test) / sizeof(values_to_test[0])] =
        {0};

    for (size_t i = 0; i < sizeof(values_to_test) / sizeof(values_to_test[0]);
         ++i) {
        values_offsets[i] = thsn_vector_current_offset(vector);
        ThsnSlice string_slice;
        ASSERT_SUCCESS(thsn_slice_from_c_str(values_to_test[i], &string_slice));
        ASSERT_SUCCESS(thsn_segment_store_string(&vector, string_slice));
    }

    ThsnSlice slice = thsn_vector_as_slice(vector);

    for (size_t i = 0; i < sizeof(values_to_test) / sizeof(values_to_test[0]);
         ++i) {
        ThsnSlice string_slice;
        ASSERT_SUCCESS(thsn_segment_read_string_ex(slice, values_offsets[i],
                                                   &string_slice, NULL));
        ASSERT_EQ(strlen(values_to_test[i]), string_slice.size);
        ASSERT_EQ(
            strncmp(values_to_test[i], string_slice.data, string_slice.size),
            0);
    }

    for (size_t i = 0; i < sizeof(values_to_test) / sizeof(values_to_test[0]);
         ++i) {
        ThsnSlice string_slice;
        size_t consumed_size;
        ASSERT_SUCCESS(thsn_segment_read_string_ex(
            slice, values_offsets[i], &string_slice, &consumed_size));
        ASSERT_NEQ(consumed_size, 0);
    }

    ASSERT_NULL_INPUT_ERROR(thsn_segment_read_string_ex(slice, 0, NULL, NULL));

    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

/* clang-format off */

TEST_SUITE(segment)
    stores_and_reads_null,
    stores_and_reads_bools, 
    stores_and_reads_doubles,
    stores_and_reads_ints, 
    stores_and_reads_strings,
END_TEST_SUITE()

#endif
