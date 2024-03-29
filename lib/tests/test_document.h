#ifndef THSN_TEST_DOCUMENT_H
#define THSN_TEST_DOCUMENT_H

#include "testing.h"
#include "threason.h"
#include "vector.h"

TEST(indexes_object_by_key) {
    const char* object_str = "{ \"abc\": 12, \"xyz\":34, \"000\": 56}";
    ThsnSlice object_str_slice = thsn_slice_from_c_str(object_str);
    ThsnDocument* document;
    ASSERT_SUCCESS(thsn_document_parse(&object_str_slice, &document));
    ThsnValueType value_type = THSN_VALUE_NULL;
    ASSERT_SUCCESS(thsn_document_value_type(document, thsn_value_handle_first(), &value_type));
    ASSERT_EQ(value_type, THSN_VALUE_OBJECT);
    ThsnValueObjectTable object_table;
    ASSERT_SUCCESS(
        thsn_document_read_object_sorted(document, thsn_value_handle_first(), &object_table));

    struct {
        const char* key;
        double expected;
    } existing_keys[] = {{.key = "000", .expected = 56.0},
                         {.key = "abc", .expected = 12.0},
                         {.key = "xyz", .expected = 34.0}};
    for (size_t i = 0; i < (sizeof(existing_keys) / sizeof(existing_keys[0])); ++i) {
        ThsnValueHandle element_handle = thsn_value_handle_not_found();
        ASSERT_SUCCESS(thsn_document_object_index(
            document, object_table, thsn_slice_from_c_str(existing_keys[i].key), &element_handle));
        ASSERT_FALSE(thsn_value_handle_is_not_found(element_handle));
        ThsnValueType element_type = THSN_VALUE_NULL;
        ASSERT_SUCCESS(thsn_document_value_type(document, element_handle, &element_type));
        ASSERT_EQ(element_type, THSN_VALUE_NUMBER);
        double number = 0.0;
        ASSERT_SUCCESS(thsn_document_read_number(document, element_handle, &number));
        ASSERT_EQ(number, existing_keys[i].expected);
    }

    char* non_existing_keys[] = {"", "a", "abcd", "1", "some_key", "ab", "!"};
    for (size_t i = 0; i < sizeof(non_existing_keys) / sizeof(non_existing_keys[0]); ++i) {
        ThsnValueHandle element_handle = thsn_value_handle_not_found();
        ASSERT_SUCCESS(thsn_document_object_index(
            document, object_table, thsn_slice_from_c_str(non_existing_keys[i]), &element_handle));
        ASSERT_TRUE(thsn_value_handle_is_not_found(element_handle));
    }
    thsn_document_free(&document);
}

TEST(parses_simple_documents) {
    ThsnDocument* document;
    const char* documents[] = {
        "",
        "1",
        "true",
        "null",
        "false",
        "\"abc\\\"def\"",
        "{ \"a\": 1e0 }",
        "[null, 0, \"\", {}, false, []]",
        "[{\"\": \"\"}, 100, true, [1]]",
    };
    for (size_t i = 0; i < sizeof(documents) / sizeof(documents[0]); ++i) {
        {
            ThsnSlice document_slice = thsn_slice_from_c_str(documents[i]);
            ASSERT_SUCCESS(thsn_document_parse(&document_slice, &document));
            ASSERT_SUCCESS(thsn_document_free(&document));
        }
        {
            ThsnSlice document_slice = thsn_slice_from_c_str(documents[i]);
            ASSERT_SUCCESS(thsn_document_parse_multithreaded(&document_slice, &document, 8));
            ASSERT_SUCCESS(thsn_document_free(&document));
        }
    }
}

TEST(fails_at_invalid_documents) {
    ThsnDocument* document;
    const char* documents[] = {",", ":",   "]",     "(",   "{",     "}",
                               "a", "1..", "True",  "nil", "faLse", "{ \"a\": 1e0",
                               "}", ": 1", "[1, 2}"};
    for (size_t i = 0; i < sizeof(documents) / sizeof(documents[0]); ++i) {
        {
            ThsnSlice document_slice = thsn_slice_from_c_str(documents[i]);
            ASSERT_INPUT_ERROR(thsn_document_parse(&document_slice, &document));
        }
        {
            ThsnSlice document_slice = thsn_slice_from_c_str(documents[i]);
            ASSERT_INPUT_ERROR(thsn_document_parse_multithreaded(&document_slice, &document, 8));
        }
    }
}

#define RAW(...) #__VA_ARGS__

TEST(parses_a_document_and_navigates_through_it) {
    const char* document_str = RAW([
        {"processed" : false, "items" : [ "apples", "oragnes" ], "cost" : 123.00},
        {"processed" : true, "items" : ["banana"], "cost" : 200},
        {"processed" : true, "items" : [], "cost" : 0}
    ]);
    ThsnSlice document_slice = thsn_slice_from_c_str(document_str);
    ThsnDocument* document;
    ASSERT_SUCCESS(thsn_document_parse(&document_slice, &document));
    ThsnValueType value_type;
    ASSERT_SUCCESS(thsn_document_value_type(document, thsn_value_handle_first(), &value_type));
    ASSERT_EQ(value_type, THSN_VALUE_ARRAY);
    ThsnValueArrayTable array_table;
    ASSERT_SUCCESS(thsn_document_read_array(document, thsn_value_handle_first(), &array_table));
    ASSERT_EQ(thsn_document_array_length(array_table), 3);
    ThsnValueHandle array_element_handle;
    /* First element */
    ASSERT_SUCCESS(
        thsn_document_array_consume_element(document, &array_table, &array_element_handle));
    ASSERT_SUCCESS(thsn_document_value_type(document, array_element_handle, &value_type));
    ASSERT_EQ(value_type, THSN_VALUE_OBJECT);
    /* Second element */
    ASSERT_SUCCESS(
        thsn_document_array_consume_element(document, &array_table, &array_element_handle));
    ASSERT_SUCCESS(thsn_document_value_type(document, array_element_handle, &value_type));
    ASSERT_EQ(value_type, THSN_VALUE_OBJECT);
    ThsnValueObjectTable object_table;
    ASSERT_SUCCESS(thsn_document_read_object_sorted(document, array_element_handle, &object_table));
    double cost = 0;
    ThsnValueHandle object_element_handle;
    ASSERT_SUCCESS(thsn_document_object_index(document, object_table, thsn_slice_from_c_str("cost"),
                                              &object_element_handle));
    ASSERT_SUCCESS(thsn_document_value_type(document, object_element_handle, &value_type));
    ASSERT_EQ(value_type, THSN_VALUE_NUMBER);
    ASSERT_SUCCESS(thsn_document_read_number(document, object_element_handle, &cost));
    ASSERT_EQ(cost, 200.0);
    /* Third element left */
    ASSERT_EQ(thsn_document_array_length(array_table), 1);
    ASSERT_SUCCESS(thsn_document_free(&document));
}
/* clang-format off */
TEST_SUITE(document)
    indexes_object_by_key,
    parses_simple_documents,
    fails_at_invalid_documents,
    parses_a_document_and_navigates_through_it,
END_TEST_SUITE()

#endif
