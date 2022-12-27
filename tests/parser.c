#include "parser.h"
#include "testing.h"

#define ASSERT_PARSE_STRING_AS_BYTES(string)                            \
    do {                                                                \
        ThsnVector result_vector = THSN_VECTOR_INIT();                  \
        ASSERT_SUCCESS(thsn_vector_make(&result_vector, 1024));         \
        ThsnSlice input_slice = thsn_slice_from_c_str((string));        \
        ASSERT_SUCCESS(thsn_parse_value(&input_slice, &result_vector)); \
    ASSERT_SLICE_EQ_BYTES(THSN_VECTOR_AS_SLICE(result_vector))

#define END_ASSERT_BYTES()            \
    END_BYTES();                      \
    thsn_vector_free(&result_vector); \
    }                                 \
    while (0)

TEST(scalar_values) {
    ASSERT_PARSE_STRING_AS_BYTES("1") 0x41, 0x01 END_ASSERT_BYTES();
    ASSERT_PARSE_STRING_AS_BYTES("   null ") 0x00 END_ASSERT_BYTES();
    ASSERT_PARSE_STRING_AS_BYTES("   true ") 0x11 END_ASSERT_BYTES();
    ASSERT_PARSE_STRING_AS_BYTES("   false  ") 0x10 END_ASSERT_BYTES();
    ASSERT_PARSE_STRING_AS_BYTES("   \"abc\"")
    0x23, 0x61, 0x62, 0x63 END_ASSERT_BYTES();
    ASSERT_PARSE_STRING_AS_BYTES("[]")
    0x60 END_ASSERT_BYTES();
    ASSERT_PARSE_STRING_AS_BYTES("{}")
    0x70 END_ASSERT_BYTES();
}

TEST_MAIN()
TEST_SUITE("parser")
scalar_values, END_TEST_SUITE() END_MAIN()
