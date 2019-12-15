#ifndef FORMAT_H /* <!-- !out */
#define FORMAT_H

#include "XMacro.h"

/* `punctuates` in `Style.c` is dependant on this order and size. */

#define FORMAT(X) \
	X(OUT_RAW), \
	X(OUT_HTML), \
	X(OUT_MD)

enum Format { FORMAT(PARAM) };
static const char *const format_strings[] = { FORMAT(STRINGISE) };

#endif /* !out --> */
