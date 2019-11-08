#ifndef OUTPUT_H /* <-- !out */
#define OUTPUT_H

#include "XMacro.h"

#define OUTPUT(X) \
	X(OUT_UNSPECIFIED, "unspecified"), \
	X(OUT_HTML, "html"), \
	X(OUT_MD, "md")

enum Output { OUTPUT(PARAM2A) };
static const char *const output_strings[] = { OUTPUT(PARAM2B) };
static const char *const output[] = { OUTPUT(STRINGISE2A) };

#endif /* !out --> */
