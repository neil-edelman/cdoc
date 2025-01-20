#include "scanner.h"
#include "token.h"
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int token_to_string(const struct token *t, char (*const a)[12]) {
	switch(t->symbol) {
	case WORD: { size_t len = t->length >= 9 ? 9 : t->length;
		if(len > INT_MAX) return errno = ERANGE, 0;
		sprintf(*a, "<%.*s>", (int)len, t->from); break; }
	case DOC_ID:
	case ID: { size_t len = t->length >= 8 ? 8 : t->length;
		if(len > INT_MAX) return errno = ERANGE, 0;
		sprintf(*a, "ID:%.*s", (int)len, t->from); break; }
	case SPACE: { (*a)[0] = '~', (*a)[1] = '\0'; break; }
	default:
		strncpy(*a, symbols[t->symbol], sizeof *a - 1);
		(*a)[sizeof *a - 1] = '\0'; break;
	}
	return 1;
}
/** Compares the _contents_ of the tokens. */
static int token_compare(const struct token *const a,
	const struct token *const b) {
	const int len_cmp = (a->length > b->length) - (b->length > a->length);
	const int str_cmp = strncmp(a->from, b->from,
		len_cmp >= 0 ? b->length : a->length);
	return str_cmp ? str_cmp : len_cmp;
}

#define DEFINE
#include "token_array.h"

/** This is used in `semantic.c.re` to get the first file:line for error. */
const char *tokens_first_label(const struct token_array *const tokens)
	{ return tokens->size ? tokens->data[0].label : "unlabelled"; }
size_t tokens_first_line(const struct token_array *const tokens)
	{ return tokens->size ? tokens->data[0].line : 0; }
/** This is used in `semantic.c.re` to get the size of the string for
 `tokens`. */
size_t tokens_mark_size(const struct token_array *const tokens) {
	if(!tokens) return 0;
	return tokens->size + 1;
}
/** @param[tokens] The `token_array` that converts to a string.
 @param[marks] Must be an at-least the size of `tokens`, or null.
 @return The size of the string, including null. */
void tokens_mark(const struct token_array *const tokens, char *mark) {
	size_t i;
	assert(mark);
	for(i = 0; i < tokens->size; i++)
		*mark++ = symbol_marks[tokens->data[i].symbol];
	*mark = '\0';
}
