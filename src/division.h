#ifndef DIVISION_H /* <!-- !div */
#define DIVISION_H

/** Define the divisions of output; these go in separate sections, and
 approximately follow namespaces. */
#define DIVISION \
	X(DIV_PREAMBLE, "preamble", "Description"), \
	X(DIV_FUNCTION, "fn",       "Function Definitions"), \
	X(DIV_TAG,      "tag",      "Struct, Union, and Enum Definitions"), \
	X(DIV_TYPEDEF,  "typedef",  "Typedef Aliases"), \
	X(DIV_DATA,     "data",     "General Declarations")

#define X(a, b, c) a
enum division { DIVISION };
#undef X
#define X(a, b, c) b
static const char *const division_strings[] = { DIVISION };
#undef X
#define X(a, b, c) c
static const char *const division_desc[] = { DIVISION };
#undef X
#define X(a, b, c) #a
static const char *const divisions[] = { DIVISION };
#undef X

#endif /* !div --> */
