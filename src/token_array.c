#include "scanner.h"
#include <stdio.h>
#include <limits.h>

struct token;
static int token_to_string(const struct token *, char (*)[12]);
static int token_compare(const struct token *, const struct token *);

#define DEFINE
#include "token_array.h"

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
