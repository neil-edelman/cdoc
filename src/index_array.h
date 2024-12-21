#include <stddef.h>

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
