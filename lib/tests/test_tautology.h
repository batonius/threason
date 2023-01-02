#ifndef THSN_TEST_TAUTOLOGY_H
#define THSN_TEST_TAUTOLOGY_H

#include <stdlib.h>

#include "testing.h"

TEST(zero_equals_zero) { ASSERT_EQ(0, 0); }

TEST(abc_isnt_equal_to_xyz) { ASSERT_STRN_NEQ("abc", "zyz", 3); }

/* clang-format off */

TEST_SUITE(tautology)
	zero_equals_zero,
	abc_isnt_equal_to_xyz,
END_TEST_SUITE()

#endif
