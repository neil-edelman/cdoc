#ifndef DIVISION_H /* <-- !div */
#define DIVISION_H

#include "XMacro.h"

/** Define the divisions of output; these go in separate sections, and
 approximately follow namespaces. */
#define DIVISION(X) \
	X(DIV0, "do not use"), \
	X(DIV_PREAMBLE, "preamble"), \
	X(DIV_FUNCTION, "fn"), \
	X(DIV3, "do not use"), \
	X(DIV_TAG,      "tag"), \
	X(DIV5, "do not use"), X(DIV6, "do not use"), X(DIV7, "do not use"), \
	X(DIV_TYPEDEF,  "typedef"), \
	X(DIV9,  "do not use"), X(DIV10, "do not use"), X(DIV11, "do not use"), \
	X(DIV12, "do not use"), X(DIV13, "do not use"), X(DIV14, "do not use"), \
	X(DIV15, "do not use"), \
	X(DIV_DATA,     "data")

enum Division { DIVISION(PARAM2A) };
static const char *const division_strings[] = { DIVISION(PARAM2B) };
static const char *const divisions[] = { DIVISION(STRINGISE2A) };

#endif /* !div --> */
