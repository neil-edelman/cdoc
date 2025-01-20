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

struct token_array;

const char *tokens_first_label(const struct token_array *const tokens);
size_t tokens_first_line(const struct token_array *const tokens);
size_t tokens_mark_size(const struct token_array *const tokens);
void tokens_mark(const struct token_array *const tokens, char *const marks);

#endif
