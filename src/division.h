#ifndef DIVISION_H
#	define DIVISION_H

/** Define the divisions of output; these go in separate sections, and
 approximately follow namespaces. */
#	define DIVISION \
	X(DIV_PREAMBLE, "preamble", "Description"), \
	X(DIV_FUNCTION, "fn",       "Function Definitions"), \
	X(DIV_TAG,      "tag",      "Struct, Union, and Enum Definitions"), \
	X(DIV_TYPEDEF,  "typedef",  "Typedef Aliases"), \
	X(DIV_DATA,     "data",     "General Declarations")

#	define X(a, b, c) a
enum division { DIVISION };
#	undef X
#	define X(a, b, c) { #a, b, c }
static const struct { const char *const symbol, *const keyword, *const desc; }
	division[] = { DIVISION };
#	undef X

#endif
