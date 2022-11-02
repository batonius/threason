#include "tags.h"
#include "testing.h"
#include <string.h>

#define ASSERT_SUCCESS(v) ASSERT_EQ((v), THSN_RESULT_SUCCESS)

TEST(stores_null) {
	thsn_vector_t vector;
	ASSERT_SUCCESS(thsn_vector_make(&vector, 1024));
	ASSERT_SUCCESS(thsn_vector_store_null(&vector));
	ASSERT_EQ(vector.offset, 1);
	ASSERT_EQ(vector.buffer[0], THSN_TAG_MAKE(THSN_TAG_NULL, THSN_TAG_SIZE_EMPTY));
	
}

TEST(stores_bools) {
	thsn_vector_t vector;
	ASSERT_SUCCESS(thsn_vector_make(&vector, 1024));
	ASSERT_SUCCESS(thsn_vector_store_bool(&vector, true));
	ASSERT_SUCCESS(thsn_vector_store_bool(&vector, false));
	ASSERT_SUCCESS(thsn_vector_store_bool(&vector, false));
	ASSERT_SUCCESS(thsn_vector_store_bool(&vector, true));
	ASSERT_EQ(vector.offset, 4);
	ASSERT_EQ(vector.buffer[0], THSN_TAG_MAKE(THSN_TAG_BOOL, THSN_TAG_SIZE_TRUE));
	ASSERT_EQ(vector.buffer[1], THSN_TAG_MAKE(THSN_TAG_BOOL, THSN_TAG_SIZE_FALSE));
	ASSERT_EQ(vector.buffer[2], THSN_TAG_MAKE(THSN_TAG_BOOL, THSN_TAG_SIZE_FALSE));
	ASSERT_EQ(vector.buffer[3], THSN_TAG_MAKE(THSN_TAG_BOOL, THSN_TAG_SIZE_TRUE));
}

TEST(stores_ints) {
	int8_t a = 123;
	int16_t b = 12345;
	int32_t c = 1234567;
	int64_t d = INT64_MAX;
	int64_t e = 0;
	thsn_vector_t vector;
	ASSERT_SUCCESS(thsn_vector_make(&vector, 1024));
	ASSERT_SUCCESS(thsn_vector_store_int(&vector, a));
	ASSERT_SUCCESS(thsn_vector_store_int(&vector, b));
	ASSERT_SUCCESS(thsn_vector_store_int(&vector, c));
	ASSERT_SUCCESS(thsn_vector_store_int(&vector, d));
	ASSERT_SUCCESS(thsn_vector_store_int(&vector, e));
	ASSERT_EQ(vector.offset, 5 + 1 + 2 + 4 + 8);
	ASSERT_EQ(vector.buffer[0], THSN_TAG_MAKE(THSN_TAG_INT, THSN_TAG_SIZE_INT8));
	memcpy(&a, vector.buffer + 1, sizeof(a));
	ASSERT_EQ(a, 123);
	ASSERT_EQ(vector.buffer[2], THSN_TAG_MAKE(THSN_TAG_INT, THSN_TAG_SIZE_INT16));
	memcpy(&b, vector.buffer + 3, sizeof(b));
	ASSERT_EQ(b, 12345);
	ASSERT_EQ(vector.buffer[5], THSN_TAG_MAKE(THSN_TAG_INT, THSN_TAG_SIZE_INT32));
	memcpy(&c, vector.buffer + 6, sizeof(c));
	ASSERT_EQ(c, 1234567);
	ASSERT_EQ(vector.buffer[10], THSN_TAG_MAKE(THSN_TAG_INT, THSN_TAG_SIZE_INT64));
	memcpy(&d, vector.buffer + 11, sizeof(d));
	ASSERT_EQ(d, INT64_MAX);
	ASSERT_EQ(vector.buffer[19], THSN_TAG_MAKE(THSN_TAG_INT, THSN_TAG_SIZE_ZERO));
}

TEST(stores_strings) {
	char* short_string = "a short string";
	char* long_string = "a somewhat longer yet not very long string";
	thsn_vector_t vector;
	thsn_slice_t short_string_slice = THSN_SLICE_FROM_C_STR(short_string);
	thsn_slice_t long_string_slice = THSN_SLICE_FROM_C_STR(long_string);
	ASSERT_SUCCESS(thsn_vector_make(&vector, 1024));
	ASSERT_SUCCESS(thsn_vector_store_string(&vector, THSN_SLICE_FROM_C_STR(short_string)));
	ASSERT_SUCCESS(thsn_vector_store_string(&vector, THSN_SLICE_FROM_C_STR(long_string)));
	ASSERT_EQ(vector.buffer[0], THSN_TAG_MAKE(THSN_TAG_SMALL_STRING, short_string_slice.size));
	ASSERT_STRN_EQ(vector.buffer + 1, short_string, short_string_slice.size);
	ASSERT_EQ(vector.buffer[1 + short_string_slice.size], THSN_TAG_MAKE(THSN_TAG_REF_STRING, THSN_TAG_SIZE_INBOUND));
	ASSERT_EQ(memcmp(vector.buffer + 2 + short_string_slice.size, &long_string_slice, sizeof(long_string_slice)), 0);
}

TEST_MAIN()
	TEST_SUITE("tags")
		stores_null,
		stores_bools,
		stores_ints,
		stores_strings,
	END_TEST_SUITE()
END_MAIN()
