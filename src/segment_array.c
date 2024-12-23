static void segment_to_string(const struct segment *, char (*)[12]);
#define DEFINE
#include "segment_array.h"

static void segment_to_string(const struct segment *const segment,
	char (*const a)[12]) {
	const struct token_array *t;
	const struct token *const fallback = segment_fallback(segment, &t); /*?*/
	const char *temp = division[segment->division].symbol;
	size_t temp_len, i = 0;
	if(fallback) {
		struct token_array_cursor tok = token_array_begin(t);
		style_push(ST_TO_RAW);
		temp = print_token_s(&tok, fallback);
		style_pop();
	}
	temp_len = strlen(temp);
	if(temp_len > sizeof *a - 3) temp_len = sizeof *a - 3;
	(*a)[i++] = 'S';
	(*a)[i++] = '_';
	memcpy(*a + i, temp, temp_len);
	i += temp_len;
	(*a)[i++] = '\0';
	assert(i <= sizeof *a);
}
