#include <stddef.h>

/* Indices are how we get indirection and are used in several places. Not
 _great_ for readability, but it was 2019. I suppose I should have stared a
 `deque`. Maybe later. */
#ifdef DEFINE
#	undef DEFINE
#else
#	define ARRAY_DECLARE_ONLY
#endif
#define ARRAY_NAME index
#define ARRAY_TYPE size_t
#define ARRAY_TO_STRING
#define ARRAY_NON_STATIC
#include "boxes/array.h"
