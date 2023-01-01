#ifndef THSN_TEST_VALUE_H
#define THSN_TEST_VALUE_H

#include "parser.h"
#include "testing.h"
#include "value.h"
#include "vector.h"

TEST(indexes_object_by_key) {
    const char* object_str = "{ \"abc\": 12, \"xyz\":34, \"000\": 56}";
    ThsnSlice object_str_slice;
    ASSERT_SUCCESS(thsn_slice_from_c_str(object_str, &object_str_slice));
    ThsnVector parse_result = thsn_vector_make_empty();
    ASSERT_SUCCESS(thsn_parse_value(&object_str_slice, &parse_result));
    ThsnSlice result_slice = thsn_vector_as_slice(parse_result);
    ThsnValueType value_type;
    ASSERT_SUCCESS(
        thsn_value_type(result_slice, THSN_VALUE_HANDLE_FIRST, &value_type));
    ASSERT_EQ(value_type, THSN_VALUE_OBJECT);
    ThsnValueObjectTable object_table;
    ASSERT_SUCCESS(thsn_value_read_object(result_slice, THSN_VALUE_HANDLE_FIRST,
                                          &object_table));

    struct {
        const char* key;
        double expected;
    } existing_keys[] = {{.key = "000", .expected = 56.0},
                         {.key = "abc", .expected = 12.0},
                         {.key = "xyz", .expected = 34.0}};
    for (size_t i = 0; i < (sizeof(existing_keys) / sizeof(existing_keys[0]));
         ++i) {
        ThsnSlice key_slice;
        ASSERT_SUCCESS(thsn_slice_from_c_str(existing_keys[i].key, &key_slice));
        ThsnValueHandle element_handle;
        ASSERT_SUCCESS(thsn_value_object_index(result_slice, object_table,
                                               key_slice, &element_handle));
        ASSERT_NEQ(element_handle, THSN_VALUE_HANDLE_NOT_FOUND);
        ThsnValueType element_type;
        ASSERT_SUCCESS(
            thsn_value_type(result_slice, element_handle, &element_type));
        ASSERT_EQ(element_type, THSN_VALUE_NUMBER);
        double number;
        ASSERT_SUCCESS(
            thsn_value_read_number(result_slice, element_handle, &number));
        ASSERT_EQ(number, existing_keys[i].expected);
    }

    char* non_existing_keys[] = {"", "a", "abcd", "1", "some_key", "ab", "!"};
    for (size_t i = 0;
         i < sizeof(non_existing_keys) / sizeof(non_existing_keys[0]); ++i) {
        ThsnSlice key_slice;
        ASSERT_SUCCESS(thsn_slice_from_c_str(non_existing_keys[i], &key_slice));
        ThsnValueHandle element_handle;
        ASSERT_SUCCESS(thsn_value_object_index(result_slice, object_table,
                                               key_slice, &element_handle));
        ASSERT_EQ(element_handle, THSN_VALUE_HANDLE_NOT_FOUND);
    }
}

/* clang-format off */
TEST_SUITE(value)
    indexes_object_by_key,
END_TEST_SUITE()

#endif
