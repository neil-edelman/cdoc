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
/* segment_array_to_string, segment_array_keep_if… */
#	define ARRAY_NON_STATIC
#	include "boxes/array.h"

struct segment *segments_new(struct segment_array *const segments);
void erase_segments(struct segment_array *segments);

#endif
