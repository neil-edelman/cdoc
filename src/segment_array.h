#ifndef segment_array_h
#	define segment_array_h
#	include "segment.h"
#	ifdef DEFINE
#		undef DEFINE
#	else
#		define ARRAY_DECLARE_ONLY
#	endif
#	define ARRAY_NAME segment
#	define ARRAY_TYPE struct segment
#	define ARRAY_TO_STRING
/*#define ARRAY_NON_STATIC *//* segment_to_string… */
#	include "boxes/array.h"

void erase_segments(struct segment_array *segments);

#endif
