
#include "division.h"
#include "token_array.h"
#include "index_array.h"
#include "attribute_array.h"

/** Each report has a segment array. Each subheading is a segment. */
struct segment {
	enum division division;
	struct token_array doc, code;
	struct index_array code_params;
	struct attribute_array attributes;
};

void segment_to_string(const struct segment *const segment,
	char (*const a)[12]);
