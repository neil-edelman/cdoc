#ifndef token_h
#	define token_h
#	include "symbol.h"
#	include <stddef.h>

/** The parser assigns an area of the text a symbol. */
struct token {
	enum symbol symbol;
	const char *from;
	size_t length;
	const char *label;
	size_t line;
};

#endif
