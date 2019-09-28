#ifndef DIVISION_H /* <-- !div */
#define DIVISION_H

#include "XMacro.h"

/** Define the divisions of output; these go in separate sections, and
 approximately follow namespaces. */
#define DIVISION(X) \
	X(DIV_PREAMBLE, "preamble"), \
	X(DIV_FUNCTION, "fn"), \
	X(DIV_TAG,      "tag"), \
	X(DIV_TYPEDEF,  "typedef"), \
	X(DIV_DATA,     "data")

enum Division { DIVISION(PARAM2A) };
static const char *const division_strings[] = { DIVISION(PARAM2B) };
static const char *const divisions[] = { DIVISION(STRINGISE2A) };

#endif /* !div --> */
