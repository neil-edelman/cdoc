#include "symbol.h"
#include <stddef.h>

/** `token` has a `symbol` and is associated with an area of the text. */
struct token {
	enum symbol symbol;
	const char *from;
	size_t length;
	const char *label;
	size_t line;
};

#ifdef DEFINE
#	undef DEFINE
#else
#	define ARRAY_DECLARE_ONLY
#endif
#define ARRAY_NAME token
#define ARRAY_TYPE struct token
/*#define ARRAY_COMPARE*/
#define ARRAY_TO_STRING
#define ARRAY_NON_STATIC
#include "boxes/array.h"
