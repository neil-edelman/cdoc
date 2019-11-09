#ifndef OUTPUT_H /* <-- !out */
#define OUTPUT_H

#include "XMacro.h"

#define OUTPUT(X) \
	X(OUT_UNSPECIFIED), \
	X(OUT_HTML), \
	X(OUT_MD)

enum Output { OUTPUT(PARAM) };
static const char *const output[] = { OUTPUT(STRINGISE) };

#endif /* !out --> */
