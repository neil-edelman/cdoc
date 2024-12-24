static void segment_to_string(const struct segment *, char (*)[12]);

void segment_to_string(const struct segment *const segment,
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

static void erase_segment(struct segment *const segment) {
	char a[12];
	assert(segment);
	segment_to_string(segment, &a);
	segment->division = DIV_PREAMBLE;
	token_array_(&segment->doc);
	token_array_(&segment->code);
	if(cdoc_get_debug() & DBG_ERASE && segment->code_params.size)
		fprintf(stderr, "*** Erasing %s: %s.\n",
		a, index_array_to_string(&segment->code_params));
	index_array_(&segment->code_params);
	attributes_(&segment->attributes);
}

/** Provides a default token for `segment` to print. */
static const struct token *segment_fallback(const struct segment *const segment,
	const struct token_array **const ta_ptr) {
	const struct token_array *ta = 0;
	const struct token *t = 0;
	assert(segment);
	if(segment->code_params.size) {
		const size_t i = segment->code_params.data[0];
		ta = &segment->code;
		assert(i < ta->size);
		t = ta->data + i;
	} else if(segment->code.size) {
		ta = &segment->code;
		t = ta->data;
	} else if(!ta_ptr && segment->doc.size) {
		/* Raw pointers in the text are problematic since maybe we will
		 convert it to a string and most text does not support that. */
		ta = &segment->doc;
		t = ta->data;
	} else if(!ta_ptr && segment->attributes.size) {
		ta = &segment->doc;
		t = &segment->attributes.data->token;
	}
	if(ta_ptr) *ta_ptr = ta;
	return t;
	/*return index_array_size(&segment->code_params)
		? segment->code.data
		+ segment->code_params.data[0]
		: token_array_size(&segment->code) ? segment->code.data
		: token_array_size(&segment->doc) ? segment->doc.data
		: attribute_array_size(&segment->attributes)
		? &attribute_array_get(&segment->attributes)->token : 0;*/
}

/* For <fn:segment_to_string>. */
static int print_token_s(struct token_array_cursor *const tok,
	const char **fill_buffer);

static const struct token *param_no(const struct segment *const segment,
	const size_t param) {
	size_t *pidx;
	assert(segment);
	if(param >= segment->code_params.size) return 0;
	pidx = segment->code_params.data + param;
	/* This is really careful. */
	if(*pidx >= segment->code.size) {
		char a[12];
		segment_to_string(segment, &a);
		fprintf(stderr, "%s: param index %lu is greater then code size.\n",
			a, (unsigned long)segment->code.size);
		return 0;
	}
	return segment->code.data + *pidx;
}

#define DEFINE
#include "segment_array.h"

/*static void segment_array_clear(struct segment_array *const sa) {
	struct segment *segment;
	if(!sa) return;
	while((segment = segment_arrayPop(sa)))
		token_array_Clear(&segment->doc), token_array_Clear(&segment->code),
		Indexarray_clear(&segment->code_params),
		attributes_(&segment->attributes);
}*/
void erase_segments(struct segment_array *segments) {
	struct segment *segment;
	while(segment = segment_array_pop(&segments)) erase_segment(segment);
	segment_array_(&segments);
}
