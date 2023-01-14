#ifndef THSN_TEST_DOCUMENT_H
#define THSN_TEST_DOCUMENT_H

#include "testing.h"
#include "threason.h"
#include "vector.h"

TEST(indexes_object_by_key) {
    const char* object_str = "{ \"abc\": 12, \"xyz\":34, \"000\": 56}";
    ThsnSlice object_str_slice;
    ASSERT_SUCCESS(thsn_slice_from_c_str(object_str, &object_str_slice));
    ThsnDocument* document;
    ASSERT_SUCCESS(thsn_document_parse(&object_str_slice, &document));
    ThsnValueType value_type = THSN_VALUE_NULL;
    ASSERT_SUCCESS(thsn_document_value_type(document, thsn_value_handle_first(),
                                            &value_type));
    ASSERT_EQ(value_type, THSN_VALUE_OBJECT);
    ThsnValueObjectTable object_table;
    ASSERT_SUCCESS(thsn_document_read_object_sorted(
        document, thsn_value_handle_first(), &object_table));

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
        ThsnValueHandle element_handle = thsn_value_handle_not_found();
        ASSERT_SUCCESS(thsn_document_object_index(document, object_table,
                                                  key_slice, &element_handle));
        ASSERT_FALSE(thsn_value_handle_is_not_found(element_handle));
        ThsnValueType element_type = THSN_VALUE_NULL;
        ASSERT_SUCCESS(
            thsn_document_value_type(document, element_handle, &element_type));
        ASSERT_EQ(element_type, THSN_VALUE_NUMBER);
        double number = 0.0;
        ASSERT_SUCCESS(
            thsn_document_read_number(document, element_handle, &number));
        ASSERT_EQ(number, existing_keys[i].expected);
    }

    char* non_existing_keys[] = {"", "a", "abcd", "1", "some_key", "ab", "!"};
    for (size_t i = 0;
         i < sizeof(non_existing_keys) / sizeof(non_existing_keys[0]); ++i) {
        ThsnSlice key_slice;
        ASSERT_SUCCESS(thsn_slice_from_c_str(non_existing_keys[i], &key_slice));
        ThsnValueHandle element_handle;
        ASSERT_SUCCESS(thsn_document_object_index(document, object_table,
                                                  key_slice, &element_handle));
        ASSERT_TRUE(thsn_value_handle_is_not_found(element_handle));
    }
    thsn_document_free(&document);
}

/* clang-format off */
TEST_SUITE(document)
    indexes_object_by_key,
END_TEST_SUITE()

#endif
