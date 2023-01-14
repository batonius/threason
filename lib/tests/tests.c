#include "test_document.h"
#include "test_segment.h"
#include "test_slice.h"
#include "test_tokenizer.h"
#include "test_vector.h"
#include "testing.h"

/* clang-format off */

TEST_MAIN()
	RUN_SUITE(slice);
	RUN_SUITE(vector);
	RUN_SUITE(segment);
	RUN_SUITE(tokenizer);
	RUN_SUITE(document);
END_MAIN()
