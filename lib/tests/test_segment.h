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
    ThsnTag tag = THSN_TAG_VALUE_HANDLE;
    ThsnSlice value_slice;
    ASSERT_SUCCESS(thsn_segment_read_tagged_value(thsn_vector_as_slice(vector),
                                                  0, &tag, &value_slice));
    ASSERT_EQ(tag, thsn_tag_make(THSN_TAG_NULL, THSN_TAG_SIZE_EMPTY));
    ASSERT_EQ(value_slice.size, 0);
    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

TEST(stores_and_reads_value_handle) {
    ThsnVector vector = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_vector_allocate(&vector, 1024));
    ASSERT_NULL_INPUT_ERROR(
        thsn_segment_store_value_handle(NULL, thsn_value_handle_first()));
    ASSERT_SUCCESS(thsn_segment_store_value_handle(
        &vector, thsn_value_handle_not_found()));
    ThsnTag tag = THSN_TAG_NULL;
    ThsnSlice value_slice = thsn_slice_make_empty();
    ASSERT_SUCCESS(thsn_segment_read_tagged_value(thsn_vector_as_slice(vector),
                                                  0, &tag, &value_slice));
    ASSERT_EQ(tag, thsn_tag_make(THSN_TAG_VALUE_HANDLE, THSN_TAG_SIZE_ZERO));
    ThsnValueHandle value_handle = thsn_value_handle_first();
    ASSERT_SUCCESS(THSN_SLICE_READ_VAR(value_slice, value_handle));
    ASSERT_TRUE(thsn_value_handle_is_not_found(value_handle));
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
    bool value = false;
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
        double value = 0.0;
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
        ASSERT_SUCCESS(thsn_segment_store_string(
            &vector, thsn_slice_from_c_str(values_to_test[i])));
    }

    ThsnSlice slice = thsn_vector_as_slice(vector);

    for (size_t i = 0; i < sizeof(values_to_test) / sizeof(values_to_test[0]);
         ++i) {
        ThsnSlice string_slice = thsn_slice_make_empty();
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

TEST(stores_and_reads_arrays) {
    ThsnVector vector = thsn_vector_make_empty();
    size_t elements_table[] = {100, 101, 102};
    size_t header_offset = 0;
    ASSERT_SUCCESS(thsn_segment_store_composite_header(
        &vector, thsn_tag_make(THSN_TAG_ARRAY, THSN_TAG_SIZE_INBOUND), false,
        &header_offset));
    ASSERT_SUCCESS(thsn_segment_store_null(&vector));
    ASSERT_SUCCESS(thsn_segment_store_int(&vector, 0xffff));
    ASSERT_SUCCESS(thsn_segment_store_composite_elements_table(
        &vector, header_offset, THSN_SLICE_FROM_VAR(elements_table), false));
    ThsnSlice out_table = thsn_slice_make_empty();
    ASSERT_SUCCESS(thsn_segment_read_composite(thsn_vector_as_mut_slice(vector),
                                               header_offset, THSN_TAG_ARRAY,
                                               &out_table, false));
    for (size_t i = 0; i < sizeof(elements_table) / sizeof(elements_table[0]);
         ++i) {
        size_t element_offset = 0;
        ASSERT_SUCCESS(thsn_segment_composite_index_element_offset(
            out_table, i, &element_offset));
        ASSERT_EQ(element_offset, elements_table[i]);
    }
    for (size_t i = 0; i < sizeof(elements_table) / sizeof(elements_table[0]);
         ++i) {
        size_t element_offset = 0;
        ASSERT_SUCCESS(thsn_segment_composite_consume_element_offset(
            &out_table, &element_offset));
        ASSERT_EQ(element_offset, elements_table[i]);
    }
    ASSERT_TRUE(thsn_slice_is_empty(out_table));
    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

TEST(stores_and_reads_objects) {
    ThsnVector vector = thsn_vector_make_empty();
    size_t header_offset = 0;
    ASSERT_SUCCESS(thsn_segment_store_composite_header(
        &vector, thsn_tag_make(THSN_TAG_OBJECT, THSN_TAG_SIZE_INBOUND), true,
        &header_offset));
    size_t first_kv = thsn_vector_current_offset(vector);
    ASSERT_SUCCESS(
        thsn_segment_store_string(&vector, thsn_slice_from_c_str("z")));
    ASSERT_SUCCESS(thsn_segment_store_int(&vector, 1));
    size_t second_kv = thsn_vector_current_offset(vector);
    ASSERT_SUCCESS(thsn_segment_store_string(
        &vector, thsn_slice_from_c_str("random string")));
    ASSERT_SUCCESS(thsn_segment_store_null(&vector));
    size_t third_kv = thsn_vector_current_offset(vector);
    ThsnSlice string_slice = thsn_slice_from_c_str("a string");
    ASSERT_SUCCESS(thsn_segment_store_string(&vector, string_slice));
    ASSERT_SUCCESS(thsn_segment_store_string(&vector, string_slice));
    size_t elements_table[] = {first_kv, second_kv, third_kv};
    ASSERT_SUCCESS(thsn_segment_store_composite_elements_table(
        &vector, header_offset, THSN_SLICE_FROM_VAR(elements_table), true));
    ThsnSlice out_table = thsn_slice_make_empty();
    ASSERT_SUCCESS(thsn_segment_read_composite(thsn_vector_as_mut_slice(vector),
                                               header_offset, THSN_TAG_OBJECT,
                                               &out_table, false));
    for (size_t i = 0; i < sizeof(elements_table) / sizeof(elements_table[0]);
         ++i) {
        size_t element_offset = 0;
        ASSERT_SUCCESS(thsn_segment_composite_index_element_offset(
            out_table, i, &element_offset));
        ASSERT_EQ(element_offset, elements_table[i]);
    }
    for (size_t i = 0; i < sizeof(elements_table) / sizeof(elements_table[0]);
         ++i) {
        size_t element_offset = 0;
        ASSERT_SUCCESS(thsn_segment_composite_consume_element_offset(
            &out_table, &element_offset));
        ASSERT_EQ(element_offset, elements_table[i]);
    }
    ThsnSlice sorted_table = thsn_slice_make_empty();
    ASSERT_SUCCESS(thsn_segment_read_composite(thsn_vector_as_mut_slice(vector),
                                               header_offset, THSN_TAG_OBJECT,
                                               &sorted_table, true));

    size_t element_offset = 0;
    bool found = false;
    ASSERT_SUCCESS(thsn_segment_object_index(
        thsn_vector_as_slice(vector), sorted_table,
        thsn_slice_from_c_str("a string"), &element_offset, &found));
    ASSERT_TRUE(found);
    ASSERT_SUCCESS(thsn_segment_object_index(
        thsn_vector_as_slice(vector), sorted_table, thsn_slice_from_c_str("z"),
        &element_offset, &found));
    ASSERT_TRUE(found);
    double value = 0;
    ASSERT_SUCCESS(thsn_segment_read_number(thsn_vector_as_slice(vector),
                                            element_offset, &value));
    ASSERT_EQ(value, 1);
    ASSERT_SUCCESS(thsn_segment_object_index(
        thsn_vector_as_slice(vector), sorted_table,
        thsn_slice_from_c_str("random string"), &element_offset, &found));
    ASSERT_TRUE(found);
    ASSERT_SUCCESS(thsn_segment_object_index(
        thsn_vector_as_slice(vector), sorted_table,
        thsn_slice_from_c_str("another string"), &element_offset, &found));
    ASSERT_FALSE(found);
    ASSERT_SUCCESS(thsn_vector_free(&vector));
}

/* clang-format off */

TEST_SUITE(segment)
    stores_and_reads_null,
    stores_and_reads_bools, 
    stores_and_reads_doubles,
    stores_and_reads_ints, 
    stores_and_reads_strings,
    stores_and_reads_value_handle,
    stores_and_reads_arrays,
    stores_and_reads_objects,
END_TEST_SUITE()

#endif
