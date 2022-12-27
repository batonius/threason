// clang-format off
#include "testing.h"

#include "test_tautology.h"
#include "test_vector.h"
#include "test_parser.h"
#include "test_tags.h"
#include "test_tokenizer.h"

TEST_MAIN()
	RUN_SUITE(tautology);
	RUN_SUITE(vector);
	RUN_SUITE(parser);
	RUN_SUITE(tags);
	RUN_SUITE(tokenizer);
END_MAIN()

