#include "token_array.h"
#include "index_array.h"

int reportprint_token_fill(struct token_array_cursor *, const char **);
int reportprint_token(struct token_array_cursor *);
int reportprint_highlight(const struct token_array *,
	const struct index_array *);
void reportprint_tokens(const struct token_array *)
