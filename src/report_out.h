/** Perform a 32 bit
 [Fowler/Noll/Vo FNV-1a hash](http://www.isthe.com/chongo/tech/comp/fnv/) on a
 string. This assumes that size of `int` is at least 32 bits; if this is not
 true, we may get a different answer, (`stdint.h` is `C99`.) */
static unsigned fnv_32a_str(const char *str) {
	const char *s = str;
	/* 32 bit FNV-1 and FNV-1a non-zero initial basis, FNV1_32A_INIT */
	unsigned hval = 0x811c9dc5;
	assert(str);
	/* FNV magic prime `FNV_32_PRIME 0x01000193`. */
	while(*s) {
		hval ^= (unsigned)*s++;
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
	}
	if(cdoc_get_debug() & DBG_HASH)
		fprintf(stderr, "fnv32: %s -> %u\n", str, hval);
	return hval & 0xffffffff;
}

/* This is very `GitHub`-2019 specific: all anchor names are pre-concatenated
 with this string _except_ those that already have it, but the fragment
 links are unchanged. To make them line up and also be a valid Markdown for
 other sites, we must always start the anchor with this string. (Seems not to
 affect the latest browsers; possibly some JavaScript trickery going on.) We
 also must use a very restricted version of Unicode, but those restrictions are
 undocumented, thus the hash. */
static const char *const md_fragment_extra = "user-content-";

/* `SYMBOL` is declared in `scanner.h`. */
#define X(a, b, c, d, e, f) f
static const char *symbol_attribute_titles[] = { SYMBOL };
#undef X

/* Some output functions need this. */
static const struct token *print_token(const struct token_array *const tokens,
	const struct token *token);

/** Selects `token` out of `tokens` and prints it and returns the next token. */
typedef int (*out_fn)(const struct token_array *const tokens,
	const struct token **const ptoken, const int is_buffer);
/* @param[ptoken] Is an [in/out] variable, it should get updated unless the
 return value is false.
 @return Success.
 @implements <Attribute>Predicate
 @fixme `is_buffer` should always be true for text-wrapping. */
#define OUT(name) static int out_##name(const struct token_array *const tokens,\
	const struct token **ptoken, const int is_buffer)

OUT(ws) {
	const struct token *const space = *ptoken;
	assert(tokens && space && space->symbol == SPACE && !is_buffer);
	style_separate();
	*ptoken = token_array_next(tokens, space);
	return 1;
}
OUT(par) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == NEWLINE && !is_buffer);
	style_pop_strong();
	style_push(ST_P);
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(lit) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->length > 0 && t->from);
	if(is_buffer) {
		style_encode_length_cat_to_buffer(t->length, t->from);
	} else {
		style_flush_symbol(t->symbol);
		style_encode_length(t->length, t->from);
	}
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(gen1) {
	const struct token *const t = *ptoken,
		*const lparen = token_array_next(tokens, t),
		*const param1 = token_array_next(tokens, lparen),
		*const rparen = token_array_next(tokens, param1);
	const char *b, *type1;
	size_t type1_size;
	const enum format f = style_format();
	const char *const format =
		f == OUT_HTML ? HTML_LT "%.*s" HTML_GT "%.*s" : "<%.*s>%.*s";
	assert(tokens && t && t->symbol == ID_ONE_GENERIC);
	if(!lparen || lparen->symbol != LPAREN || !param1 || !rparen
		|| rparen->symbol != RPAREN) goto catch;
	type1 = t->from;
	if(!(b = strchr(type1, '_'))) goto catch;
	type1_size = (size_t)(b - type1);
	assert(t->length == (size_t)(b + 1 - t->from));
	assert(type1_size <= INT_MAX && param1->length <= INT_MAX);
	if(is_buffer) {
		const char *const ltgt = f == OUT_HTML ? HTML_LT HTML_GT : "<>";
		char *a;
		if(!(a = buffer_prepare(strlen(ltgt) + type1_size + param1->length)))
			return 0;
		sprintf(a, format,
			(int)type1_size, type1, (int)param1->length, param1->from);
	} else {
		style_flush_symbol(t->symbol);
		assert(type1_size <= INT_MAX && param1->length <= INT_MAX);
		printf(format,
			(int)type1_size, type1, (int)param1->length, param1->from);
	}
	*ptoken = token_array_next(tokens, rparen);
	return 1;
catch:
	fprintf(stderr, "%s: expected generic(id) %s.\n", pos(t),
		token_array_to_string(tokens));
	return 0;
}
OUT(gen2) {
	const struct token *const t = *ptoken,
		*const lparen = token_array_next(tokens, t),
		*const param1 = token_array_next(tokens, lparen),
		*const comma = token_array_next(tokens, param1),
		*const param2 = token_array_next(tokens, comma),
		*const rparen = token_array_next(tokens, param2);
	const char *b, *type1, *type2;
	size_t type1_size, type2_size;
	const enum format f = style_format();
	const char *const format = f == OUT_HTML ? HTML_LT "%.*s" HTML_GT "%.*s"
		HTML_LT "%.*s" HTML_GT "%.*s" : "<%.*s>%.*s<%.*s>%.*s";
	assert(tokens && t && t->symbol == ID_TWO_GENERICS);
	if(!lparen || lparen->symbol != LPAREN || !param1 || !comma
		|| comma->symbol != COMMA || !param2 || !rparen
		|| rparen->symbol != RPAREN) goto catch;
	type1 = t->from;
	if(!(b = strchr(type1, '_'))) goto catch;
	type1_size = (size_t)(b - type1);
	type2 = b + 1;
	if(!(b = strchr(type2, '_'))) goto catch;
	type2_size = (size_t)(b - type2);
	assert(t->length == (size_t)(b + 1 - t->from));
	assert(type1_size < INT_MAX && param1->length < INT_MAX
		&& type2_size < INT_MAX && param2->length < INT_MAX);
	if(is_buffer) {
		const char *const ltgt = f == OUT_HTML ? HTML_LT HTML_GT : "<>";
		char *a;
		if(!(a = buffer_prepare(2 * strlen(ltgt) + type1_size + param1->length
			+ type2_size + param2->length))) return 0;
		sprintf(a, format,
			(int)type1_size, type1, (int)param1->length, param1->from,
			(int)type2_size, type2, (int)param2->length, param2->from);
	} else {
		style_flush_symbol(t->symbol);
		printf(format,
			(int)type1_size, type1, (int)param1->length, param1->from,
			(int)type2_size, type2, (int)param2->length, param2->from);
	}
	*ptoken = token_array_next(tokens, rparen);
	return 1;
catch:
	fprintf(stderr, "%s: expected generic2(id,id).\n", pos(t));
	return 0;
}
OUT(gen3) {
	const struct token *const t = *ptoken,
		*const lparen = token_array_next(tokens, t),
		*const param1 = token_array_next(tokens, lparen),
		*const comma1 = token_array_next(tokens, param1),
		*const param2 = token_array_next(tokens, comma1),
		*const comma2 = token_array_next(tokens, param2),
		*const param3 = token_array_next(tokens, comma2),
		*const rparen = token_array_next(tokens, param3);
	const char *b, *type1, *type2, *type3;
	size_t type1_size, type2_size, type3_size;
	const enum format f = style_format();
	const char *const format = f == OUT_HTML ? HTML_LT "%.*s" HTML_GT "%.*s"
		HTML_LT "%.*s" HTML_GT "%.*s" HTML_LT "%.*s" HTML_GT "%.*s"
		: "<%.*s>%.*s<%.*s>%.*s<%.*s>%.*s";
	assert(tokens && t && t->symbol == ID_THREE_GENERICS);
	if(!lparen || lparen->symbol != LPAREN || !param1 || !comma1
		|| comma1->symbol != COMMA || !param2 || !comma2 ||
		comma2->symbol != COMMA || !param3 || !rparen
		|| rparen->symbol != RPAREN) goto catch;
	type1 = t->from;
	if(!(b = strchr(type1, '_'))) goto catch;
	type1_size = (size_t)(b - type1);
	type2 = b + 1;
	if(!(b = strchr(type2, '_'))) goto catch;
	type2_size = (size_t)(b - type2);
	type3 = b + 1;
	if(!(b = strchr(type3, '_'))) goto catch;
	type3_size = (size_t)(b - type3);
	assert(t->length == (size_t)(b + 1 - t->from));
	assert(type1_size < INT_MAX && param1->length < INT_MAX
		&& type2_size < INT_MAX && param2->length < INT_MAX
		&& type3_size < INT_MAX && param3->length < INT_MAX);
	if(is_buffer) {
		const char *const ltgt = f == OUT_HTML ? HTML_LT HTML_GT : "<>";
		char *a;
		if(!(a = buffer_prepare(3 * strlen(ltgt) + type1_size + param1->length
			+ type2_size + param2->length
			+ type3_size + param3->length))) return 0;
		sprintf(a, format,
			(int)type1_size, type1, (int)param1->length, param1->from,
			(int)type2_size, type2, (int)param2->length, param2->from,
			(int)type3_size, type3, (int)param3->length, param3->from);
	} else {
		style_flush_symbol(t->symbol);
		printf(format,
			(int)type1_size, type1, (int)param1->length, param1->from,
			(int)type2_size, type2, (int)param2->length, param2->from,
			(int)type3_size, type3, (int)param3->length, param3->from);
	}
	*ptoken = token_array_next(tokens, rparen);
	return 1;
catch:
	fprintf(stderr, "%s: expected A_B_C_(id,id,id).\n", pos(t));
	return 0;
}
OUT(escape) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == ESCAPE && t->length == 2 && !is_buffer);
	style_flush_symbol(t->symbol);
	style_encode_length(1, t->from + 1);
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(url) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == URL && !is_buffer);
	style_flush_symbol(t->symbol);
	assert(t->length <= INT_MAX);
	/* I think it can't contain '<>()\"' by the parser. */
	if(style_format() == OUT_HTML) {
		printf("<a href = \"%.*s\">", (int)t->length, t->from);
		style_encode_length(t->length, t->from);
		printf("</a>");
	} else {
		printf("[");
		style_encode_length(t->length, t->from);
		printf("](%.*s)", (int)t->length, t->from);
	}
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(cite) {
	const struct token *const t = *ptoken;
	const char *const url_encoded = url_encode(t->from, t->length);
	assert(tokens && t && t->symbol == CITE && !is_buffer);
	if(!url_encoded) goto catch;
	style_flush_symbol(t->symbol);
	if(style_format() == OUT_HTML) {
		printf("<a href = \"https://scholar.google.ca/scholar?q=%s\">",
			url_encoded);
		style_encode_length(t->length, t->from);
		printf("</a>");
	} else {
		printf("[");
		style_encode_length(t->length, t->from);
		printf("](https://scholar.google.ca/scholar?q=%s)", url_encoded);
	}
	*ptoken = token_array_next(tokens, t);
	return 1;
catch:
	fprintf(stderr, "%s: expected <source>.\n", pos(t));
	return 0;
}
static int see(const struct token_array *const tokens,
	const struct token **ptoken, const int is_buffer,
	const enum division divn) {
	const struct token *const tok = *ptoken;
	assert(tokens && tok && !is_buffer
		&& ((tok->symbol == SEE_FN && divn == DIV_FUNCTION)
		|| (tok->symbol == SEE_TAG && divn == DIV_TAG)
		|| (tok->symbol == SEE_TYPEDEF && divn == DIV_TYPEDEF)
		|| (tok->symbol == SEE_DATA && divn == DIV_DATA)));
	style_flush_symbol(tok->symbol);
	if(style_format() == OUT_HTML) {
		printf("<a href = \"#%s:", division_strings[divn]);
		style_encode_length(tok->length, tok->from);
		printf("\">");
		style_encode_length(tok->length, tok->from);
		printf("</a>");
	} else {
		printf("[");
		style_push(ST_TO_HTML); /* <-- html: this is not escaped by Markdown. */
		style_encode_length(tok->length, tok->from);
		style_pop(); /* html --> */
		printf("](#%s%s-%x)", md_fragment_extra, division_strings[divn],
			fnv_32a_str(style_encode_length_raw_to_buffer(tok->length, tok->from)));
	}
	*ptoken = token_array_next(tokens, tok);
	return 1;
}
OUT(see_fn) { return see(tokens, ptoken, is_buffer, DIV_FUNCTION); }
OUT(see_tag) { return see(tokens, ptoken, is_buffer, DIV_TAG); }
OUT(see_typedef) { return see(tokens, ptoken, is_buffer, DIV_TYPEDEF); }
OUT(see_data) { return see(tokens, ptoken, is_buffer, DIV_DATA); }
OUT(math_begin) { /* Math and code. */
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_BEGIN && !is_buffer);
	style_push(ST_CODE);
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(math_end) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_END && !is_buffer);
	style_expect(ST_CODE);
	style_pop();
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(em_begin) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_BEGIN && !is_buffer);
	style_push(ST_EM);
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(em_end) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_END && !is_buffer);
	style_expect(ST_EM);
	style_pop();
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(link) {
	const struct token *const t = *ptoken, *text, *turl;
	const enum format f = style_format();
	const char *fn;
	size_t fn_len;
	FILE *fp;
	int success = 0;
	assert(tokens && t && t->symbol == LINK_START && !is_buffer);
	style_flush_symbol(t->symbol);
	/* The expected format is LINK_START [^URL]* URL. */
	for(turl = token_array_next(tokens, t);;turl = token_array_next(tokens, turl)) {
		if(!turl) goto catch;
		if(turl->symbol == URL) break;
	}
	/* We want to open this file to check if it's on the up-and-up. */
	if(!(errno = 0, fn = url_from_here(turl->length, turl->from)))
		{ if(errno) goto catch; else goto raw; }
	if(!(fp = fopen(fn, "r"))) { perror(fn); errno = 0; goto raw; } fclose(fp);
	/* Actually use the entire url. */
	if(!(errno = 0, fn = url_from_output(turl->length, turl->from)))
		{ if(errno) goto catch; else goto raw; }
	fn_len = strlen(fn);
	assert(fn_len <= INT_MAX);
	if(cdoc_get_debug() & DBG_OUTPUT)
		fprintf(stderr, "%s: local link %.*s.\n", pos(t), (int)fn_len, fn);
	goto output;
raw:
	/* Maybe it's an external link? Just put it unmolested. */
	fn = turl->from;
	fn_len = turl->length;
	if(cdoc_get_debug() & DBG_OUTPUT)
		fprintf(stderr, "%s: absolute link %.*s.\n", pos(t), (int)fn_len, fn);
output:
	assert(fn_len <= INT_MAX);
	if(f == OUT_HTML) printf("<a href = \"%.*s\">", (int)fn_len, fn);
	else printf("[");
	/* This is html even in Md because it's surrounded by `a`. */
	style_push(ST_TO_HTML), style_push(ST_PLAIN);
	for(text = token_array_next(tokens, t); text->symbol != URL; )
		if(!(text = print_token(tokens, text))) goto catch;
	style_pop(), style_pop();
	if(f == OUT_HTML) printf("</a>");
	else printf("](%.*s)", (int)fn_len, fn);
	success = 1;
	goto finally;
catch:
	fprintf(stderr, "%s: expected `[description](url)`.\n", pos(t));
finally:
	*ptoken = token_array_next(tokens, turl);
	return success;
}
OUT(image) {
	const struct token *const t = *ptoken, *text, *turl;
	const enum format f = style_format();
	const char *fn;
	unsigned width = 1, height = 1;
	int success = 0;
	assert(tokens && t && t->symbol == IMAGE_START && !is_buffer);
	style_flush_symbol(t->symbol);
	/* The expected format is IMAGE_START [^URL]* URL. */
	for(turl = token_array_next(tokens, t); turl->symbol != URL;
		turl = token_array_next(tokens, turl)) if(!turl) goto catch;
	printf("%s", f == OUT_HTML ? "<img alt = \"" : "![");
	/* This is html even in Md. */
	style_push(ST_TO_HTML), style_push(ST_PLAIN);
	for(text = token_array_next(tokens, t); text->symbol != URL; )
		if(!(text = print_token(tokens, text))) goto catch;
	style_pop(), style_pop();
	/* We want to open this file to check if it's on the up-and-up. */
	if(!(errno = 0, fn = url_from_here(turl->length, turl->from)))
		{ if(errno) goto catch; else goto raw; }
	if(!image_dimension(fn, &width, &height)) goto raw;
	/* We want the url to print, now. */
	if(!(errno = 0, fn = url_from_output(turl->length, turl->from)))
		{ if(errno) goto catch; else goto raw; }
	if(cdoc_get_debug() & DBG_OUTPUT)
		fprintf(stderr, "%s: local image %s.\n", pos(t), fn);
	if(f == OUT_HTML) {
		printf("\" src = \"%s\" width = %u height = %u>", fn, width, height);
	} else {
		printf("](%s)", fn);
	}
	success = 1;
	goto finally;
raw:
	/* Maybe it's an external link? */
	assert(turl->length <= INT_MAX);
	if(cdoc_get_debug() & DBG_OUTPUT) fprintf(stderr,
		"%s: remote image %.*s.\n", pos(t), (int)turl->length, turl->from);
	printf("%s%.*s%s", f == OUT_HTML ? "\" src = \"" : "](",
		(int)turl->length, turl->from, f == OUT_HTML ? "\">" : ")");
	success = 1;
	goto finally;
catch:
	fprintf(stderr, "%s: expected `[description](url)`.\n", pos(t));
finally:
	*ptoken = token_array_next(tokens, turl);
	return success;
}
OUT(nbsp) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == NBSP && !is_buffer);
	style_flush_symbol(t->symbol);
	printf("&nbsp;");
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(nbthinsp) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == NBTHINSP && !is_buffer);
	style_flush_symbol(t->symbol);
	printf("&#8239;" /* "&thinsp;" <- breaking? */);
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(mathcalo) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATHCALO && !is_buffer);
	style_flush_symbol(t->symbol);
	/* Omicron. It looks like a stylised "O"? The actual is "&#120030;" but
	 good luck finding a font that supports that. If one was using JavaScript
	 and had a constant connection, we could use MathJax. */
	printf("&#927;" /* "O" */);
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(ctheta) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == CTHETA && !is_buffer);
	style_flush_symbol(t->symbol);
	printf("&#920;" /* "&Theta;" This is supported on more browsers. */);
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(comega) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == COMEGA && !is_buffer);
	style_flush_symbol(t->symbol);
	printf("&#937;" /* "&Omega;" */);
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(times) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == TIMES && !is_buffer);
	style_flush_symbol(t->symbol);
	printf("&#215;");
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(cdot) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == CDOT && !is_buffer);
	style_flush_symbol(t->symbol);
	printf("&#183;" /* &middot; */);
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(list) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == LIST_ITEM && !is_buffer);
	if(style_Is_top(ST_LI)) {
		style_pop_push();
	} else {
		style_pop_strong();
		style_push(ST_UL), style_push(ST_LI);
	}
	*ptoken = token_array_next(tokens, t);
	return 1;
}
OUT(pre) {
	const struct token *const t = *ptoken;
	assert(tokens && t && t->symbol == PREFORMATTED && !is_buffer);
	if(style_Is_top(ST_PRELINE)) style_pop_strong();
	style_push(ST_PRE), style_push(ST_PRELINE);
	style_flush_symbol(t->symbol);
	style_encode_length(t->length, t->from);
	*ptoken = token_array_next(tokens, t);
	return 1;
}



/* `SYMBOL` is declared in `scanner.h`. */
#define X(a, b, c, d, e, f) c
static const out_fn symbol_outs[] = { SYMBOL };
#undef X



/** Prints one [multi-]token sequence.
 @param[tokens] The token array that `token` is a part of.
 @param[token] The start token.
 @param[a] If non-null, prints to a string instead of `stdout`. Only variable
 name tokens support this.
 @throws[EILSEQ] Sequence error. Must detect with `errno`.
 @return The next token. */
static const struct token *print_token_choose(
	const struct token_array *const tokens,
	const struct token *token, const int is_buffer) {
	const out_fn sym_out = symbol_outs[token->symbol];
	assert(tokens && token);
	if(!sym_out) return fprintf(stderr, "%s: symbol output undefined.\n",
		pos(token)), token_array_next(tokens, token);
	if(!sym_out(tokens, &token, is_buffer)) { errno = EILSEQ; return 0; }
	return token;
}

/** This is only for individual tokens.
 @return The temporary buffer that stores the token. */
static const char *print_token_s(const struct token_array *const tokens,
	const struct token *token) {
	buffer_clear();
	print_token_choose(tokens, token, 1); /* Ignore return. */
	return buffer_get();
}

/**
 @return The next token to be printed. */
static const struct token *print_token(const struct token_array *const tokens,
	const struct token *token) {
	return print_token_choose(tokens, token, 0);
}

/** @param[highlights] Must be sorted if not null, creates an emphasis on those
 words.
 @throws[EILSEQ] Sequence error. Must detect with `errno`. */
static void highlight_tokens(const struct token_array *const tokens,
	const struct index_array *const highlights) {
	const struct token *const first = token_array_next(tokens, 0), *token = first;
	size_t *highlight = index_array_next(highlights, 0);
	int is_highlight, is_first_highlight = 1;
	assert(tokens);
	if(!token) return;
	style_push(ST_PLAIN);
	while(token) {
		if(highlight && *highlight == (size_t)(token - first)) {
			is_highlight = 1;
			highlight = index_array_next(highlights, highlight);
		} else {
			is_highlight = 0;
		}
		if(is_highlight) style_highlight_on(is_first_highlight
			? ST_STRONG_HTML : ST_EM_HTML);
		token = print_token(tokens, token);
		if(is_highlight) style_highlight_off(), is_first_highlight = 0;
	}
	style_pop();
}

static void print_tokens(const struct token_array *const tokens)
	{ highlight_tokens(tokens, 0); }



/** Prints the documentation part of `brief` and erases it. */
static void print_brief(void) {
	print_tokens(&brief);
	token_array_clear(&brief);
}



typedef int (*divisionPredicate)(const enum division);

/** @implements divisionPredicate */
static int is_div_preamble(const enum division d) {
	return d == DIV_PREAMBLE;
}

/** @implements divisionPredicate */
static int is_not_div_preamble(const enum division d) {
	return d != DIV_PREAMBLE;
}

/** @return The first token in `tokens` that matches `token`. */
static const struct token *any_token(const struct token_array *const tokens,
	const struct token *const token) {
	const struct token *t = 0;
	while((t = token_array_next(tokens, t)))
		if(!token_compare(token, t)) return t;
	return 0;
}

/** @return If there exist an `attribute_symbol` with content within
 `segment`. */
static int segment_attribute_exists(const struct segment *const segment,
	const enum symbol attribute_symbol) {
	struct attribute *attribute = 0;
	assert(segment);
	while((attribute = attribute_array_next(&segment->attributes, attribute)))
		if(attribute->token.symbol == attribute_symbol
		&& attribute->contents.size) return 1;
	return 0;
}

/** @return Is `division` in the report? */
static int division_exists(const enum division division) {
	struct segment *segment = 0;
	while((segment = segment_array_next(&report, segment)))
		if(segment->division == division) return 1;
	return 0;
}

/** `act` on all `division`. */
static void division_act(const enum division division,
	void (*act)(const struct segment *const segment)) {
	const struct segment *segment = 0;
	assert(act);
	while((segment = segment_array_next(&report, segment)))
		if(segment->division == division) act(segment);
}

/** @return Is `attribute_symbol` in the report? (needed for `@licence`.) */
static int attribute_exists(const enum symbol attribute_symbol) {
	struct segment *segment = 0;
	while((segment = segment_array_next(&report, segment)))
		if(segment_attribute_exists(segment, attribute_symbol)) return 1;
	return 0;
}

/** Searches if attribute `symbol` exists within `segment` whose header
 contains `match`.
 @order O(`attributes`) */
static int segment_attribute_match_exists(const struct segment *const segment,
	const enum symbol symbol, const struct token *const match) {
	struct attribute *attribute = 0;
	assert(segment);
	while((attribute = attribute_array_next(&segment->attributes, attribute))) {
		if(attribute->token.symbol != symbol) continue;
		if(any_token(&attribute->header, match)) return 1;
	}
	return 0;
}

static void print_best_guess_at_modifiers(const struct segment *const segment) {
	const struct token *code = segment->code.data,
		*stop = code + *segment->code_params.data;
	assert(segment && segment->division == DIV_FUNCTION
		&& segment->code_params.size >= 1
		&& segment->code.size > *segment->code_params.data);
	while(code < stop) {
		if(code->symbol == LPAREN) {
			style_separate();
			style_push(ST_EM), style_flush();
			printf("function");
			style_pop();
			return;
		}
		code = print_token(&segment->code, code);
	}
}

/** Scan the `str`, used below. */
static void scan_doc_string(const char *const str) {
	struct scanner *scan;
	assert(str);
	/* Generally this will be a bug in the programme, not in the input. */
	if(!(scan = scanner("string", str, &notify_brief, START_DOC)))
		{ perror(str); assert(0); exit(EXIT_FAILURE); return; }
	style_flush(), print_brief();
	scanner_(&scan);
}

static unsigned log2u(unsigned i) {
	unsigned log = 0;
	while(i) log++, i >>= 1;
	return log;
}

/** For computing the number of digits. */
static unsigned compute_digits_x(unsigned x) {
	unsigned digits = (log2u(x) + 3) >> 2;
	/*fprintf(stderr, "digits log_16(%x) -> %u\n", x, digits);*/
	return digits ? digits : 1;
}

/* @param[label] Un-escaped label. */
static void print_fragment_for(const enum division d, const char *const label) {
	/* `effective_format` is NOT the thing we need; we need to raw format for
	 the link. */
	const enum format f = cdoc_get_format();
	const unsigned hash = fnv_32a_str(label);
	/* GCC is too smart but not smart enough. */
	const char *const fmt_html = "[%s](#%s:%s)",
		*const fmt_md = "[%s](#%s%s-%x)",
		*const division = division_strings[d];
	const size_t label_len = strlen(label), division_len = strlen(division),
		md_fragment_extra_len = strlen(md_fragment_extra),
		fmt_len = (f == OUT_HTML) ? strlen("[](#:)") + 2 * label_len
		+ division_len : strlen("[](#-)") + label_len + md_fragment_extra_len
		+ division_len + compute_digits_x(hash);
	size_t len;
	char *b;
	assert(label);
	/* (Potentially) calling this with `label` as the other buffer. */
	buffer_swap();
	buffer_clear();
	if(!(b = buffer_prepare(fmt_len)))
		{ perror(label); assert(0); exit(EXIT_FAILURE); return; }
	len = (size_t)(f == OUT_HTML ? sprintf(b, fmt_html, label, division, label)
		: sprintf(b, fmt_md, label, md_fragment_extra, division, hash));
	assert(len == fmt_len);
	scan_doc_string(b);
	buffer_swap();
}

static void print_custom_heading_fragment_for(const char *const division,
	const char *const desc) {
	const enum format f = style_format();
	/* "*.0s" does not work on all libraries? */
	const char *const fmt_html = "[%s](#%s:)", *const fmt_md = "[%s](#%s%s)";
	const size_t fmt_len = (f == OUT_HTML)
		? strlen("[](#:)") + strlen(desc) + strlen(division)
		: strlen("[](#)") + strlen(desc) + strlen(md_fragment_extra)
		+ strlen(division);
	size_t len;
	char *b;
	assert(division && desc);
	buffer_clear();
	if(!(b = buffer_prepare(fmt_len)))
		{ perror(division); assert(0); exit(EXIT_FAILURE); return; }
	len = (size_t)(f == OUT_HTML ? sprintf(b, fmt_html, desc, division) :
		sprintf(b, fmt_md, desc, md_fragment_extra, division));
	assert(len == fmt_len);
	scan_doc_string(b);
}

static void print_heading_fragment_for(const enum division d) {
	print_custom_heading_fragment_for(division_strings[d], division_desc[d]);
}

/** @param[label] In HTML. */
static void print_anchor_for(const enum division d, const char *const label) {
	const enum format f = style_format();
	const char *const division = division_strings[d];
	assert(label);
	style_push(ST_H3), style_flush();
	printf("<a ");
	if(f == OUT_HTML) {
		printf("id = \"%s:%s\" name = \"%s:%s\"",
			division, label, division, label);
	} else {
		const unsigned hash = fnv_32a_str(label);
		printf("id = \"%s%s-%x\" name = \"%s%s-%x\"", md_fragment_extra,
			division, hash, md_fragment_extra, division, hash);
	}
	fputc('>', stdout);
	style_push(ST_TO_HTML); /* The format is HTML because it's in an HTML tag. */
	style_encode(label);
	style_pop();
	fputs("</a>", stdout);
	style_pop(); /* h2 */
}

static void print_custom_heading_anchor_for(const char *const division,
	const char *const desc) {
	assert(division && desc);
	style_push(ST_H2);
	style_flush();
	printf("<a ");
	(style_format() == OUT_HTML)
		? printf("id = \"%s:\" name = \"%s:\"", division, division)
		: printf("id = \"%s%s\" name = \"%s%s\"", md_fragment_extra, division,
		md_fragment_extra, division);
	printf(">%s</a>", desc);
	style_pop(); /* h2 */
}

static void print_heading_anchor_for(enum division d) {
	print_custom_heading_anchor_for(division_strings[d], division_desc[d]);
}

/** Toc subcategories. */
static void print_toc_extra(const enum division d) {
	struct segment *segment = 0;
	size_t *idxs;
	struct token *params;
	const char *b;
	printf(": ");
	style_push(ST_CSV), style_push(ST_NO_STYLE);
	while((segment = segment_array_next(&report, segment))) {
		if(segment->division != d) continue;
		if(!segment->code_params.size) { fprintf(stderr,
			"%s: segment has no title.\n", divisions[segment->division]);
			continue; }
		idxs = segment->code_params.data;
		params = segment->code.data;
		assert(idxs[0] < segment->code.size);
		style_push(ST_TO_RAW);
		b = print_token_s(&segment->code, params + idxs[0]);
		style_pop();
		print_fragment_for(d, b);
		style_pop_push();
	}
	style_pop(), style_pop();
}

/** Functions as a bit-vector. */
enum AttShow { SHOW_NONE, SHOW_WHERE, SHOW_TEXT, SHOW_ALL };

static void segment_att_print_all(const struct segment *const segment,
	const enum symbol symbol, const struct token *const match,
	const enum AttShow show) {
	struct attribute *attribute = 0;
	assert(segment);
	if(!show) return;
	if(cdoc_get_debug() & DBG_ERASE)
		fprintf(stderr, "segment_att_print_all segment %s and symbol %s.\n", divisions[segment->division], symbols[symbol]);
	while((attribute = attribute_array_next(&segment->attributes, attribute))) {
		size_t *pindex;
		if(attribute->token.symbol != symbol
		   || (match && !any_token(&attribute->header, match))) continue;
		style_flush();
		if(show & SHOW_WHERE) {
			if((pindex = index_array_next(&segment->code_params, 0))
			   && *pindex < segment->code.size) {
				const struct token *token = segment->code.data + *pindex;
				print_fragment_for(segment->division,
					print_token_s(&segment->code, token));
			} else {
				/* Not going to happen -- cull takes care of it. */
				printf("%s", division_strings[segment->division]);
			}
		}
		if(show == SHOW_ALL) fputs(": ", stdout);
		if(show & SHOW_TEXT) print_tokens(&attribute->contents);
		style_pop_push();
		/* Only do one if `SHOW_TEXT` is not set; in practice, this affects
		 license, only showing one _per_ function. */
		if(!(show & SHOW_TEXT)) break;
	}
}

/** For each `division` segment, print all attributes that match `symbol`.
 @param[div_pred] If specified, only prints when it returns true.
 @param[symbol, show] Passed to <fn:segment_att_print_all>. */
static void div_att_print(const divisionPredicate div_pred,
	const enum symbol symbol, const enum AttShow show) {
	struct segment *segment = 0;
	if(!show) return;
	while((segment = segment_array_next(&report, segment)))
		if(!div_pred || div_pred(segment->division))
			segment_att_print_all(segment, symbol, 0, show);
}

static void dl_segment_att(const struct segment *const segment,
	const enum symbol attribute, const struct token *match,
	const enum style_punctuate p) {
	assert(segment && attribute);
	if((match && !segment_attribute_match_exists(segment, attribute, match))
	   || (!match && !segment_attribute_exists(segment, attribute))) return;
	style_push(ST_DT), style_push(ST_PLAIN), style_flush();
	printf("%s:", symbol_attribute_titles[attribute]);
	if(match) style_separate(), style_push(ST_EM),
		print_token(&segment->code, match), style_pop();
	style_pop(), style_pop();
	style_push(ST_DD), style_push(p), style_push(ST_PLAIN);
	segment_att_print_all(segment, attribute, match, SHOW_TEXT);
	if(cdoc_get_debug() & DBG_ERASE)
		fprintf(stderr, "dl_segment_att for %s.\n", symbols[attribute]);
	style_pop(), style_pop(), style_pop();
}

/** This is used in preamble for attributes inside a `dl`.
 @param[is_recursive]  */
static void dl_preamble_att(const enum symbol attribute,
	const enum AttShow show, const enum style_punctuate p) {
	assert(!style_is_empty());
	if(cdoc_get_debug() & DBG_ERASE)
		fprintf(stderr, "dl_preamble_att for %s.\n", symbols[attribute]);
	if(!attribute_exists(attribute)) return;
	style_push(ST_DT), style_flush();
	printf("%s:", symbol_attribute_titles[attribute]);
	style_pop();
	style_push(ST_DD), style_push(p), style_push(ST_PLAIN);
	div_att_print(&is_div_preamble, attribute, SHOW_TEXT);
	style_pop(), style_push(ST_PAREN), style_push(ST_CSV), style_push(ST_PLAIN);
	div_att_print(&is_not_div_preamble, attribute, show);
	style_pop(), style_pop(), style_pop(), style_pop(), style_pop();
}

static void dl_segment_specific_att(const struct attribute *const attribute) {
	assert(attribute);
	style_push(ST_DT), style_push(ST_PLAIN), style_flush();
	printf("%s:", symbol_attribute_titles[attribute->token.symbol]);
	if(attribute->header.size) {
		const struct token *token = 0;
		style_separate();
		style_push(ST_CSV);
		while((token = token_array_next(&attribute->header, token)))
			print_token(&attribute->header, token), style_separate();
		style_pop();
	}
	style_pop(), style_pop();
	style_push(ST_DD), style_push(ST_PLAIN);
	print_tokens(&attribute->contents);
	style_pop(), style_pop();
}



/** Prints all a `segment`.
 @implements division_act */
static void segment_print_all(const struct segment *const segment) {
	const struct token *param;
	const char *b;
	assert(segment && segment->division != DIV_PREAMBLE);
	style_push(ST_DIV);

	/* The title is generally the first param. Only single-words. */
	if((param = param_no(segment, 0))) {
		style_push(ST_TO_RAW); /* Anchors are always raw. */
		b = print_token_s(&segment->code, param);
		style_pop();
		print_anchor_for(segment->division, b);
		style_push(ST_P), style_push(ST_TO_HTML);
		style_flush();
		printf("<code>");
		highlight_tokens(&segment->code, &segment->code_params);
		printf("</code>");
		style_pop_strong();
	} else {
		style_push(ST_H3), style_flush();
		printf("Unknown");
		style_pop_strong();
	}

	/* Now text. */
	style_push(ST_P);
	print_tokens(&segment->doc);
	style_pop_strong();

	/* Attrubutes. */
	style_push(ST_DL);
	if(segment->division == DIV_FUNCTION) {
		const struct attribute *att = 0;
		size_t no;
		for(no = 1; (param = param_no(segment, no)); no++)
			dl_segment_att(segment, ATT_PARAM, param, ST_PLAIN);
		dl_segment_att(segment, ATT_RETURN, 0, ST_PLAIN);
		dl_segment_att(segment, ATT_IMPLEMENTS, 0, ST_CSV);
		while((att = attribute_array_next(&segment->attributes, att))) {
			if(att->token.symbol != ATT_THROWS) continue;
			dl_segment_specific_att(att);
		}
		dl_segment_att(segment, ATT_ORDER, 0, ST_PLAIN);
	} else if(segment->division == DIV_TAG) {
		const struct attribute *att = 0;
		while((att = attribute_array_next(&segment->attributes, att))) {
			if(att->token.symbol != ATT_PARAM) continue;
			dl_segment_specific_att(att);
		}
	}
	dl_segment_att(segment, ATT_AUTHOR, 0, ST_CSV);
	dl_segment_att(segment, ATT_STD, 0, ST_SSV);
	dl_segment_att(segment, ATT_DEPEND, 0, ST_SSV);
	dl_segment_att(segment, ATT_FIXME, 0, ST_PLAIN);
	dl_segment_att(segment, ATT_LICENSE, 0, ST_PLAIN);
	dl_segment_att(segment, ATT_CF, 0, ST_SSV);
	style_pop_strong(); /* dl */
	style_pop_strong(); /* div */
}



/** Outputs a report.
 @throws[EILSEQ] Sequence error.
 @return Success. */
int report_out(void) {
	const char *const summary = "summary",
		*const summary_desc = "Function Summary",
		*const license = "license",
		*const license_desc = symbol_attribute_titles[ATT_LICENSE];
	const int is_preamble = division_exists(DIV_PREAMBLE),
		is_function = division_exists(DIV_FUNCTION),
		is_tag = division_exists(DIV_TAG),
		is_typedef = division_exists(DIV_TYPEDEF),
		is_data = division_exists(DIV_DATA),
		is_license = attribute_exists(ATT_LICENSE),
		is_abstract = attribute_exists(ATT_ABSTRACT);
	const struct segment *segment = 0;
	const int is_html = style_format() == OUT_HTML;
	const char *const in_fn = cdoc_get_input(),
		*const base_fn = strrchr(in_fn, *url_dirsep),
		*const title = base_fn ? base_fn + 1 : in_fn;

	assert(in_fn && style_is_empty());

	/* Set `errno` here so that we don't have to test output each time. */
	errno = 0;
	if(is_html) {
		printf("<!doctype html public \"-//W3C//DTD HTML 4.01//EN\" "
			"\"http://www.w3.org/TR/html4/strict.dtd\">\n\n"
			"<html>\n\n"
			"<head>\n"
			"<meta http-equiv = \"Content-Type\""
			" content = \"text/html;charset=UTF-8\">\n"
			"<!-- Steal these colour values from JavaDocs. -->\n"
			"<style type = \"text/css\">\n"
			"\ta:link,  a:visited { color: #4a6782; }\n"
			"\ta:hover, a:focus   { color: #bb7a2a; }\n"
			"\ta:active           { color: #4A6782; }\n"
			"\ttr:nth-child(even) { background: #dee3e9; }\n"
			"\tdiv {\n"
			"\t\tmargin:  4px 0;\n"
			"\t\tpadding: 0 4px 4px 4px;\n");
		printf("\t}\n"
			"\ttable      { width: 100%%; }\n"
			"\ttd         { padding: 4px; }\n"
			"\th3, h1 {\n"
			"\t\tcolor: #2c4557;\n"
			"\t\tbackground-color: #dee3e9;\n"
			"\t\tpadding:          4px;\n"
			"\t}\n"
			"\th3 {\n"
			"\t\tmargin:           0 -4px;\n"
			"\t\tpadding:          4px;\n"
			"\t}\n"
			"</style>\n");
		style_push(ST_TITLE), style_push(ST_SSV), style_push(ST_PLAIN);
		style_flush(), style_encode(title), style_pop_strong();
		assert(style_is_empty());
		printf("</head>\n\n"
			"<body>\n\n");
	}

	/* Title. */
	style_push(ST_H1), style_push(ST_SSV), style_push(ST_PLAIN);
	style_flush(), style_encode(title), style_pop_strong();
	assert(style_is_empty());

	/* Abstract, (_ie_, tl;dr.) */
	if(is_abstract) {
		style_push(ST_P);
		div_att_print(&is_div_preamble, ATT_ABSTRACT, SHOW_TEXT);
		style_pop_strong();
	}

	/* Subtitle. */
	style_push(ST_H2), style_push(ST_SSV), style_push(ST_PLAIN);
	div_att_print(&is_div_preamble, ATT_SUBTITLE, SHOW_TEXT);
	style_pop_strong();
	assert(style_is_empty());

	/* TOC. */
	style_push(ST_UL), style_push(ST_LI);
	if(is_preamble) print_heading_fragment_for(DIV_PREAMBLE), style_pop_push();
	if(is_typedef) print_heading_fragment_for(DIV_TYPEDEF),
		print_toc_extra(DIV_TYPEDEF), style_pop_push();
	if(is_tag) print_heading_fragment_for(DIV_TAG),
		print_toc_extra(DIV_TAG), style_pop_push();
	if(is_data) print_heading_fragment_for(DIV_DATA),
		print_toc_extra(DIV_DATA), style_pop_push();
	if(is_function) print_custom_heading_fragment_for(summary, summary_desc),
		style_pop_push(), print_heading_fragment_for(DIV_FUNCTION),
		style_pop_push();
	if(is_license) print_custom_heading_fragment_for(license, license_desc),
		style_pop_push();
	style_pop_strong();
	assert(style_is_empty());

	/* Preamble contents; it shows up as the more-aptly named "description" but
	 I didn't want to type that much. */
	if(is_preamble) {
		style_push(ST_DIV), style_push(ST_NO_STYLE);
		print_heading_anchor_for(DIV_PREAMBLE);
		style_push(ST_P);
		while((segment = segment_array_next(&report, segment))) {
			if(segment->division != DIV_PREAMBLE) continue;
			print_tokens(&segment->doc);
			style_pop_push();
		}
		style_pop_strong(); /* P */
		style_push(ST_DL);
		/* `ATT_TITLE` is above. */
		while((segment = segment_array_next(&report, segment))) {
			const struct attribute *att = 0;
			if(segment->division != DIV_PREAMBLE) continue;
			while((att = attribute_array_next(&segment->attributes, att))) {
				if(att->token.symbol != ATT_PARAM) continue;
				dl_segment_specific_att(att);
			}
		}
		dl_preamble_att(ATT_AUTHOR, SHOW_ALL, ST_CSV);
		dl_preamble_att(ATT_STD, SHOW_ALL, ST_CSV);
		dl_preamble_att(ATT_DEPEND, SHOW_ALL, ST_CSV);
		dl_preamble_att(ATT_FIXME, SHOW_WHERE, ST_PLAIN);
		dl_preamble_att(ATT_CF, SHOW_ALL, ST_SSV);
		/* `ATT_RETURN`, `ATT_THROWS`, `ATT_IMPLEMENTS`, `ATT_ORDER`,
		 `ATT_ALLOW` have warnings. `ATT_LICENSE` is below. */
		style_pop_strong();
		style_pop_strong();
	}
	assert(style_is_empty());

	/* Print typedefs. */
	if(is_typedef) {
		print_heading_anchor_for(DIV_TYPEDEF);
		division_act(DIV_TYPEDEF, &segment_print_all);
	}
	/* Print tags. */
	if(is_tag) {
		print_heading_anchor_for(DIV_TAG);
		division_act(DIV_TAG, &segment_print_all);
	}
	/* Print general declarations. */
	if(is_data) {
		print_heading_anchor_for(DIV_DATA);
		division_act(DIV_DATA, &segment_print_all);
	}
	/* Print functions. */
	if(is_function) {
		/* Function table. */
		style_push(ST_DIV);
		print_custom_heading_anchor_for(summary, summary_desc);
		style_push(ST_TO_HTML);
		style_flush();
		printf("<table>\n\n"
			"<tr><th>Modifiers</th><th>Function Name</th>"
			"<th>Argument List</th></tr>\n\n");
		while((segment = segment_array_next(&report, segment))) {
			struct token *params;
			size_t *idxs, idxn, idx, paramn;
			const char *b;
			if(segment->division != DIV_FUNCTION
				|| !(idxn = segment->code_params.size)) continue;
			idxs = segment->code_params.data;
			params = segment->code.data;
			paramn = segment->code.size;
			assert(idxs[0] < paramn);
			printf("<tr><td align = right>");
			style_push(ST_PLAIN);
			print_best_guess_at_modifiers(segment);
			style_pop();
			printf("</td><td>");
			style_push(ST_TO_RAW); /* Always get raw; translate after. */
			b = print_token_s(&segment->code, params + idxs[0]);
			style_pop();
			print_fragment_for(DIV_FUNCTION, b);
			printf("</td><td>");
			for(idx = 1; idx < idxn; idx++) {
				assert(idxs[idx] < paramn);
				if(idx > 1) printf(", ");
				print_token(&segment->code, params + idxs[idx]);
			}
			printf("</td></tr>\n\n");
		}
		printf("</table>\n\n");
		style_pop(); /* to_html */
		style_pop(); /* div */
		assert(style_is_empty());

		/* Functions. */
		style_push(ST_DIV);
		print_heading_anchor_for(DIV_FUNCTION);
		division_act(DIV_FUNCTION, &segment_print_all);
		style_pop_strong();
	}
	/* License. */
	if(is_license) {
		style_push(ST_DIV);
		print_custom_heading_anchor_for(license, license_desc);
		style_push(ST_P);
		div_att_print(&is_div_preamble, ATT_LICENSE, SHOW_TEXT);
		style_pop_push();
		style_push(ST_LICENSE), style_push(ST_PLAIN);
		div_att_print(&is_not_div_preamble, ATT_LICENSE, SHOW_WHERE);
		style_pop_strong();
		style_pop_strong();
	}
	if(is_html) printf("</body>\n\n"
		"</html>\n");
	style_();
	return errno ? 0 : 1;
}
