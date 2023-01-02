#include "test_slice.h"
#include "test_tags.h"
#include "test_tautology.h"
#include "test_tokenizer.h"
#include "test_value.h"
#include "test_vector.h"
#include "testing.h"

/* clang-format off */

TEST_MAIN()
	RUN_SUITE(tautology);
	RUN_SUITE(slice);
	RUN_SUITE(vector);
	RUN_SUITE(tags);
	RUN_SUITE(tokenizer);
	RUN_SUITE(value);
END_MAIN()
