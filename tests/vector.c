#include "testing.h"
#include "vector.h"

TEST(allocates_vector) {
    ThsnVector vector = {.buffer = NULL, .capacity = 0, .offset = 1};
    ASSERT_SUCCESS(thsn_vector_make(&vector, 1024));
    ASSERT_NEQ(vector.buffer, NULL);
    ASSERT_EQ(vector.capacity, 1024);
    ASSERT_EQ(vector.offset, 0);
    ASSERT_EQ(thsn_vector_make(NULL, 1024), THSN_RESULT_INPUT_ERROR);
}

TEST(frees_vector) {
    ThsnVector vector = {.buffer = NULL, .capacity = 0, .offset = 1};
    ASSERT_SUCCESS(thsn_vector_make(&vector, 1024));
    ASSERT_SUCCESS(thsn_vector_free(&vector));
    ASSERT_EQ(thsn_vector_free(NULL), THSN_RESULT_INPUT_ERROR);
}

TEST(grows_vector) {
    ThsnVector vector = THSN_VECTOR_INIT();
    ASSERT_SUCCESS(thsn_vector_make(&vector, 1024));
    ASSERT_SUCCESS(thsn_vector_grow(&vector, 1024));
    ASSERT_EQ(vector.offset, 1024);
    ASSERT_SUCCESS(thsn_vector_grow(&vector, 1024));
    ASSERT_EQ(vector.offset, 2048);
    ASSERT_EQ(thsn_vector_grow(NULL, 1024), THSN_RESULT_INPUT_ERROR);
    ASSERT_EQ(thsn_vector_grow(&vector, SIZE_MAX),
              THSN_RESULT_OUT_OF_MEMORY_ERROR);
}

TEST(shrinks_vector) {
    ThsnVector vector = THSN_VECTOR_INIT();
    ASSERT_SUCCESS(thsn_vector_make(&vector, 1024));
    ASSERT_EQ(THSN_VECTOR_SPACE_LEFT(vector), 1024);
    ASSERT_SUCCESS(thsn_vector_grow(&vector, 24));
    ASSERT_EQ(THSN_VECTOR_SPACE_LEFT(vector), 1000);
    ASSERT_SUCCESS(thsn_vector_shrink(&vector, 20));
    ASSERT_EQ(THSN_VECTOR_SPACE_LEFT(vector), 1020);
    ASSERT_EQ(thsn_vector_shrink(NULL, 10), THSN_RESULT_INPUT_ERROR);
}

TEST(pushes_and_pops) {
    ThsnVector vector = THSN_VECTOR_INIT();
    uint64_t a = 1;
    uint32_t b = 2;
    uint16_t c = 3;
    uint8_t d = 4;
    ASSERT_SUCCESS(thsn_vector_make(&vector, 0));
    ASSERT_SUCCESS(THSN_VECTOR_PUSH_VAR(vector, d));
    ASSERT_SUCCESS(THSN_VECTOR_PUSH_VAR(vector, c));
    ASSERT_SUCCESS(THSN_VECTOR_PUSH_VAR(vector, b));
    ASSERT_SUCCESS(THSN_VECTOR_PUSH_VAR(vector, a));
    a = b = c = d = 0;
    ASSERT_SUCCESS(THSN_VECTOR_POP_VAR(vector, a));
    ASSERT_SUCCESS(THSN_VECTOR_POP_VAR(vector, b));
    ASSERT_SUCCESS(THSN_VECTOR_POP_VAR(vector, c));
    ASSERT_SUCCESS(THSN_VECTOR_POP_VAR(vector, d));
    ASSERT_EQ(a, 1);
    ASSERT_EQ(b, 2);
    ASSERT_EQ(c, 3);
    ASSERT_EQ(d, 4);
}

TEST_MAIN()
TEST_SUITE("threadson vector")
allocates_vector, frees_vector, grows_vector, shrinks_vector, pushes_and_pops,
    END_TEST_SUITE() END_MAIN()
