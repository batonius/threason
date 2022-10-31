#include "testing.h"
#include <stdlib.h>

TEST(zero_equals_zero) {
	ASSERT_EQ(0, 0);
}

TEST(abc_isnt_equal_to_xyz) {
	ASSERT_STRN_NEQ("abc", "zyz", 3);
}

BENCH(malloc_and_free_1Kb)
	BENCH_MEASURE(1000)
		char* volatile data = malloc(1024);
		*data = 0;
		free(data);
	END_MEASURE()
END_BENCH()

BENCH(malloc_and_free_1Kb_loop)
	BENCH_MEASURE_LOOP(1000)
		char* volatile data = malloc(1024);
		*data = 0;
		free(data);
	END_MEASURE_LOOP()
END_BENCH()

BENCH(malloc_and_free_1Gb)
	BENCH_MEASURE(1000)
		char* volatile data = malloc(1024 * 1024 * 1024);
		*data = 0;
		free(data);
	END_MEASURE()
END_BENCH()

BENCH(malloc_and_free_1Gb_loop)
	BENCH_MEASURE_LOOP(1000)
		char* volatile data = malloc(1024 * 1024 * 1024);
		*data = 0;
		free(data);
	END_MEASURE_LOOP()
END_BENCH()

TEST_MAIN()
	TEST_SUITE("Tautology")
		zero_equals_zero,
		abc_isnt_equal_to_xyz,
	END_TEST_SUITE()
	BENCH_SUITE("Allocator")
		malloc_and_free_1Kb,
		malloc_and_free_1Gb,
		malloc_and_free_1Kb_loop,
		malloc_and_free_1Gb_loop,
	END_BENCH_SUITE()
END_MAIN()
