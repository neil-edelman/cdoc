#ifndef FORMAT_H /* <!-- !out */
#define FORMAT_H

#include "XMacro.h"

#define FORMAT(X) \
	X(OUT_UNSPECIFIED), \
	X(OUT_HTML), \
	X(OUT_MD)

enum Format { FORMAT(PARAM) };
static const char *const format_strings[] = { FORMAT(STRINGISE) };

#endif /* !out --> */
