#include "attribute.h"
#ifdef DEFINE
#	undef DEFINE
#else
#	define ARRAY_DECLARE_ONLY
#endif
#define ARRAY_NAME attribute
#define ARRAY_TYPE struct attribute
#define ARRAY_TO_STRING
#define ARRAY_NON_STATIC /*—do we really need all the functions of
 attribute_array? */
#include "boxes/array.h"
