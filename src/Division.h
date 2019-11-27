#ifndef DIVISION_H /* <!-- !div */
#define DIVISION_H

#include "XMacro.h"

/** Define the divisions of output; these go in separate sections, and
 approximately follow namespaces. */
#define DIVISION(X) \
	X(DIV_PREAMBLE, "preamble", "Description"), \
	X(DIV_FUNCTION, "fn",       "Function Definitions"), \
	X(DIV_TAG,      "tag",      "Struct, Union, and Enum Definitions"), \
	X(DIV_TYPEDEF,  "typedef",  "Typedef Aliases"), \
	X(DIV_DATA,     "data",     "General Declarations")

enum Division { DIVISION(PARAM3A) };
static const char *const division_strings[] = { DIVISION(PARAM3B) };
static const char *const division_desc[] = { DIVISION(PARAM3C) };
static const char *const divisions[] = { DIVISION(STRINGISE3A) };

#endif /* !div --> */
