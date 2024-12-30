#ifndef char_array_h
#	define char_array_h
#	ifdef DEFINE
#		undef DEFINE
#	else
#		define ARRAY_DECLARE_ONLY
#	endif
#	define ARRAY_NAME char
#	define ARRAY_TYPE char
#	define ARRAY_TO_STRING
#	define ARRAY_NON_STATIC
#	include "boxes/array.h"
#endif
