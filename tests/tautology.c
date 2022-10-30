#include "testing.h"

TEST(zero_equals_zero) {
	ASSERT_EQ(0, 0);
}

TEST(abc_isnt_equal_to_xyz) {
	ASSERT_STRN_NEQ("abc", "zyz", 3);
}

TEST_MAIN("Testign framework itself")
	TEST_SUITE("Tautology")
		zero_equals_zero,
		abc_isnt_equal_to_xyz,
	END_SUITE()
END_MAIN()
