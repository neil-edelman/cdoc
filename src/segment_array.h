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

#ifdef DEFINE
#	undef DEFINE
#else
#	define ARRAY_DECLARE_ONLY
#endif
#define ARRAY_NAME segment
#define ARRAY_TYPE struct segment
#define ARRAY_TO_STRING
//#define ARRAY_NON_STATIC
#include "boxes/array.h"
