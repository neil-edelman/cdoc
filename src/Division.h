#ifndef DIVISION_H /* <-- !div */
#define DIVISION_H

#ifndef PARAM
#include "XMacro.h"
#endif

#define DIVISION(X) X(DIV_PREAMBLE), X(DIV_FUNCTION), X(DIV_TAG), \
	X(DIV_TYPEDEF), X(DIV_GENERAL_DECLARATION)
/** Define the divisions of output; these go in separate sections, and follow
 namespaces. */
enum Division { DIVISION(PARAM) };
static const char *const divisions[] = { DIVISION(STRINGISE) };

#endif /* !div --> */
