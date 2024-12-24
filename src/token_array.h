#ifndef token_array_h
#	define token_array_h
#	include "token.h"
#	ifdef DEFINE
#		undef DEFINE
#	else
#		define ARRAY_DECLARE_ONLY
#	endif
#	define ARRAY_NAME token
#	define ARRAY_TYPE struct token
/*	#define ARRAY_COMPARE*/
#	define ARRAY_TO_STRING
#	define ARRAY_NON_STATIC
#	include "boxes/array.h"
#endif
