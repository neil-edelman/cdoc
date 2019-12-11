/** Perform a 32 bit
 [Fowler/Noll/Vo FNV-1a hash](http://www.isthe.com/chongo/tech/comp/fnv/) on a
 string. This assumes that size of `int` is at least 32 bits; if this is not
 true, we may get a different answer, (`stdint.h` is `C99`.) */
static unsigned fnv_32a_str(const char *str) {
	const unsigned char *s = (const unsigned char *)str;
	/* 32 bit FNV-1 and FNV-1a non-zero initial basis, FNV1_32A_INIT */
	unsigned hval = 0x811c9dc5;
	assert(str);
	/* FNV magic prime `FNV_32_PRIME 0x01000193`. */
	while(*s) {
		hval ^= *s++;
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
	}
	if(CdocGetDebug()) fprintf(stderr, "fnv32: %s -> %u\n", str, hval);
	return hval & 0xffffffff;
}

/* This is very `GitHub` specific. */
static const char *const md_fragment_extra = "user-content-";

/* `SYMBOL` is declared in `Scanner.h`. */
static const char *symbol_attribute_titles[] = { SYMBOL(PARAM6F) };

/* Some `OutFn` need this. */
static const struct Token *print_token(const struct TokenArray *const tokens,
	const struct Token *token);

/** Selects `token` out of `tokens` and prints it and returns the next token. */
typedef int (*OutFn)(const struct TokenArray *const tokens,
	const struct Token **const ptoken, const int is_buffer);
/* @param[ptoken] Is an [in/out] variable, it should get updated unless the
 return value is false.
 @return Success.
 @implements <Attribute>Predicate
 @fixme `is_buffer` should always be true for text-wrapping. */
#define OUT(name) static int name(const struct TokenArray *const tokens, \
	const struct Token **ptoken, const int is_buffer)

OUT(ws) {
	const struct Token *const space = *ptoken;
	assert(tokens && space && space->symbol == SPACE && !is_buffer);
	style_separate();
	*ptoken = TokenArrayNext(tokens, space);
	return 1;
}
OUT(par) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NEWLINE && !is_buffer);
	style_pop_level();
	style_push(&styles[ST_P][effective_format()]);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(lit) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->length > 0 && t->from);
	if(is_buffer) {
		encode_cat_len_s(t->length, t->from);
	} else {
		style_prepare_output(t->symbol);
		encode_len(t->length, t->from);
	}
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(gen1) {
	const struct Token *const t = *ptoken,
		*const lparen = TokenArrayNext(tokens, t),
		*const param = TokenArrayNext(tokens, lparen),
		*const rparen = TokenArrayNext(tokens, param);
	const char *b, *type;
	int type_size;
	const enum Format f = effective_format();
	const char *const format =
		f == OUT_HTML ? HTML_LT "%.*s" HTML_GT "%.*s" : "<%.*s>%.*s";
	assert(tokens && t && t->symbol == ID_ONE_GENERIC);
	if(!lparen || lparen->symbol != LPAREN || !param || !rparen
		|| rparen->symbol != RPAREN) goto catch;
	type = t->from;
	if(!(b = strchr(type, '_'))) goto catch;
	type_size = (int)(b - type);
	assert(t->length == b + 1 - t->from);
	if(is_buffer) {
		size_t len;
		char *a;
		/* fixme: snprintf is not defined in C89. */
		if((len = snprintf(0, 0, format, t->length - 1, t->from, param->length,
			param->from)) <= 0 || !(a = BufferPrepare(len))) return 0;
		sprintf(a, format, t->length - 1, t->from, param->length, param->from);
	} else {
		style_prepare_output(t->symbol);
		printf(format, t->length - 1, t->from, param->length, param->from);
	}
	*ptoken = TokenArrayNext(tokens, rparen);
	return 1;
catch:
	fprintf(stderr, "%s: expected generic(id) %s.\n", pos(t),
		TokenArrayToString(tokens));
	return 0;
}
OUT(gen2) {
	const struct Token *const t = *ptoken,
		*const lparen = TokenArrayNext(tokens, t),
		*const param1 = TokenArrayNext(tokens, lparen),
		*const comma = TokenArrayNext(tokens, param1),
		*const param2 = TokenArrayNext(tokens, comma),
		*const rparen = TokenArrayNext(tokens, param2);
	const char *b, *type1, *type2;
	int type1_size, type2_size;
	const enum Format f = effective_format();
	const char *const format = f == OUT_HTML ? HTML_LT "%.*s" HTML_GT "%.*s"
		HTML_LT "%.*s" HTML_GT "%.*s" : "<%.*s>%.*s<%.*s>%.*s";
	assert(tokens && t && t->symbol == ID_TWO_GENERICS);
	if(!lparen || lparen->symbol != LPAREN || !param1 || !comma
		|| comma->symbol != COMMA || !param2 || !rparen
		|| rparen->symbol != RPAREN) goto catch;
	type1 = t->from;
	if(!(b = strchr(type1, '_'))) goto catch;
	type1_size = (int)(b - type1);
	type2 = b + 1;
	if(!(b = strchr(type2, '_'))) goto catch;
	type2_size = (int)(b - type2);
	assert(t->length == b + 1 - t->from);
	if(is_buffer) {
		size_t len;
		char *a;
		/* fixme: snprintf is not defined in C89. */
		if((len = snprintf(0, 0, format, type1_size, type1, param1->length,
			param1->from, type2_size, type2, param2->length, param2->from)) <= 0
			|| !(a = BufferPrepare(len))) return 0;
		sprintf(a, format, type1_size, type1, param1->length, param1->from,
			type2_size, type2, param2->length, param2->from);
	} else {
		style_prepare_output(t->symbol);
		printf(format, type1_size, type1, param1->length, param1->from,
			type2_size, type2, param2->length, param2->from);
	}
	*ptoken = TokenArrayNext(tokens, rparen);
	return 1;
catch:
	fprintf(stderr, "%s: expected generic2(id,id).\n", pos(t));
	return 0;
}
OUT(gen3) {
	const struct Token *const t = *ptoken,
		*const lparen = TokenArrayNext(tokens, t),
		*const param1 = TokenArrayNext(tokens, lparen),
		*const comma1 = TokenArrayNext(tokens, param1),
		*const param2 = TokenArrayNext(tokens, comma1),
		*const comma2 = TokenArrayNext(tokens, param2),
		*const param3 = TokenArrayNext(tokens, comma2),
		*const rparen = TokenArrayNext(tokens, param3);
	const char *b, *type1, *type2, *type3;
	int type1_size, type2_size, type3_size;
	const enum Format f = effective_format();
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
	type1_size = (int)(b - type1);
	type2 = b + 1;
	if(!(b = strchr(type2, '_'))) goto catch;
	type2_size = (int)(b - type2);
	type3 = b + 1;
	if(!(b = strchr(type3, '_'))) goto catch;
	type3_size = (int)(b - type3);
	assert(t->length == b + 1 - t->from);
	if(is_buffer) {
		size_t len;
		char *a;
		/* fixme: snprintf is not defined in C89. */
		if((len = snprintf(0, 0, format, type1_size, type1, param1->length,
			param1->from, type2_size, type2, param2->length, param2->from,
			type3_size, type3, param3->length, param3->from)) <= 0
			|| !(a = BufferPrepare(len))) return 0;
		sprintf(a, format, type1_size, type1, param1->length, param1->from,
			type2_size, type2, param2->length, param2->from, type3_size, type3,
			param3->length, param3->from);
	} else {
		style_prepare_output(t->symbol);
		printf(format, type1_size, type1, param1->length, param1->from,
			type2_size, type2, param2->length, param2->from, type3_size, type3,
			param3->length, param3->from);
	}
	*ptoken = TokenArrayNext(tokens, rparen);
	return 1;
catch:
	fprintf(stderr, "%s: expected A_B_C_(id,id,id).\n", pos(t));
	return 0;
}
OUT(escape) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == ESCAPE && t->length == 2 && !is_buffer);
	style_prepare_output(t->symbol);
	encode_len(1, t->from + 1);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(url) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == URL && !is_buffer);
	style_prepare_output(t->symbol);
	/* I think it can't contain '<>()\"' by the parser. */
	if(effective_format() == OUT_HTML) {
		printf("<a href = \"%.*s\">", t->length, t->from);
		encode_len(t->length, t->from);
		printf("</a>");
	} else {
		printf("[");
		encode_len(t->length, t->from);
		printf("](%.*s)", t->length, t->from);
	}
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(cite) {
	const struct Token *const t = *ptoken;
	const char *const url_encoded = UrlEncode(t->from, t->length);
	assert(tokens && t && t->symbol == CITE && !is_buffer);
	if(!url_encoded) goto catch;
	style_prepare_output(t->symbol);
	if(effective_format() == OUT_HTML) {
		printf("<a href = \"https://scholar.google.ca/scholar?q=%s\">",
			url_encoded);
		encode_len(t->length, t->from);
		printf("</a>");
	} else {
		printf("[");
		encode_len(t->length, t->from);
		printf("](https://scholar.google.ca/scholar?q=%s)", url_encoded);
	}
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
catch:
	fprintf(stderr, "%s: expected <short source>.\n", pos(t));
	return 0;
}
static int see(const struct TokenArray *const tokens,
	const struct Token **ptoken, const int is_buffer,
	const enum Division divn) {
	const struct Token *const tok = *ptoken;
	assert(tokens && tok && !is_buffer
		&& ((tok->symbol == SEE_FN && divn == DIV_FUNCTION)
		|| (tok->symbol == SEE_TAG && divn == DIV_TAG)
		|| (tok->symbol == SEE_TYPEDEF && divn == DIV_TYPEDEF)
		|| (tok->symbol == SEE_DATA && divn == DIV_DATA)));
	style_prepare_output(tok->symbol);
	if(effective_format() == OUT_HTML) {
		printf("<a href = \"#%s:", division_strings[divn]);
		encode_len(tok->length, tok->from);
		printf("\">");
		encode_len(tok->length, tok->from);
		printf("</a>");
	} else {
		printf("[");
		style_push(&to_html); /* This is not escaped by `GitHub` Markdown. */
		encode_len(tok->length, tok->from);
		style_pop();
		printf("](#%s%s-%x)", md_fragment_extra, division_strings[divn],
			   fnv_32a_str(encode_len_s_raw(tok->length, tok->from)));
	}
	*ptoken = TokenArrayNext(tokens, tok);
	return 1;
}
OUT(see_fn) { return see(tokens, ptoken, is_buffer, DIV_FUNCTION); }
OUT(see_tag) { return see(tokens, ptoken, is_buffer, DIV_TAG); }
OUT(see_typedef) { return see(tokens, ptoken, is_buffer, DIV_TYPEDEF); }
OUT(see_data) { return see(tokens, ptoken, is_buffer, DIV_DATA); }
OUT(math_begin) { /* Math and code. */
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_BEGIN && !is_buffer);
	style_push(&styles[ST_CODE][effective_format()]);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(math_end) {
	const struct Token *const t = *ptoken;
	const struct StyleText *const st = style_text_peek(),
		*const st_expect = &styles[ST_CODE][effective_format_will_be_popped()];
	assert(tokens && t && t->symbol == MATH_END && !is_buffer);
	if(st != st_expect) {
		char st_str[12], st_expect_str[12];
		style_text_to_string(st, &st_str);
		style_text_to_string(st_expect, &st_expect_str);
		return fprintf(stderr, "%s: expected %s but got %s.\n",
			StyleArrayToString(&mode.styles), st_expect_str, st_str), 0;
	}
	style_pop();
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_begin) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_BEGIN && !is_buffer);
	style_push(&styles[ST_EM][effective_format()]);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_end) {
	const struct Token *const t = *ptoken;
	const struct StyleText *const st = style_text_peek(),
		*const st_expect = &styles[ST_EM][effective_format_will_be_popped()];
	assert(tokens && t && t->symbol == EM_END && !is_buffer);
	if(st != st_expect) {
		char st_str[12], st_expect_str[12];
		style_text_to_string(st, &st_str);
		style_text_to_string(st_expect, &st_expect_str);
		return fprintf(stderr, "%s: expected %s but got %s.\n",
			StyleArrayToString(&mode.styles), st_expect_str, st_str), 0;
	}
	style_pop();
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(link) {
	const struct Token *const t = *ptoken, *text, *turl;
	const enum Format f = effective_format();
	const char *fn;
	size_t fn_len;
	FILE *fp;
	int success = 0;
	assert(tokens && t && t->symbol == LINK_START && !is_buffer);
	style_prepare_output(t->symbol);
	/* The expected format is LINK_START [^URL]* URL. */
	for(turl = TokenArrayNext(tokens, t);;turl = TokenArrayNext(tokens, turl)) {
		if(!turl) goto catch;
		if(turl->symbol == URL) break;
	}
	/* We want to open this file to check if it's on the up-and-up. */
	if(!(errno = 0, fn = PathFromHere(turl->length, turl->from)))
		{ if(errno) goto catch; else goto raw; }
	if(!(fp = fopen(fn, "r"))) { perror(fn); errno = 0; goto raw; } fclose(fp);
	/* Actually use the entire path. */
	if(!(errno = 0, fn = PathFromOutput(turl->length, turl->from)))
		{ if(errno) goto catch; else goto raw; }
	fn_len = strlen(fn);
	assert(fn_len < INT_MAX);
	if(CdocGetDebug())
		fprintf(stderr, "%s: local link %.*s.\n", pos(t), (int)fn_len, fn);
	goto output;
raw:
	/* Maybe it's an external link? Just put it unmolested. */
	fn = turl->from;
	fn_len = turl->length;
	if(CdocGetDebug())
		fprintf(stderr, "%s: absolute link %.*s.\n", pos(t), (int)fn_len, fn);
output:
	assert(fn_len <= INT_MAX);
	if(f == OUT_HTML) printf("<a href = \"%.*s\">", (int)fn_len, fn);
	else printf("[");
	/* This is html even in md in `GitHub`. */
	style_push(&to_html), style_push(&plain_text);
	for(text = TokenArrayNext(tokens, t); text->symbol != URL; )
		if(!(text = print_token(tokens, text))) goto catch;
	style_pop(), style_pop();
	if(f == OUT_HTML) printf("</a>");
	else printf("](%.*s)", (int)fn_len, fn);
	success = 1;
	goto finally;
catch:
	fprintf(stderr, "%s: expected `[description](url)`.\n", pos(t));
finally:
	*ptoken = TokenArrayNext(tokens, turl);
	return success;
}
OUT(image) {
	const struct Token *const t = *ptoken, *text, *turl;
	const enum Format f = effective_format();
	const char *fn;
	unsigned width = 1, height = 1;
	int success = 0;
	assert(tokens && t && t->symbol == IMAGE_START && !is_buffer);
	style_prepare_output(t->symbol);
	/* The expected format is IMAGE_START [^URL]* URL. */
	for(turl = TokenArrayNext(tokens, t); turl->symbol != URL;
		turl = TokenArrayNext(tokens, turl)) if(!turl) goto catch;
	printf("%s", f == OUT_HTML ? "<img alt = \"" : "![");
	/* This is html even in md in `GitHub`. */
	style_push(&to_html), style_push(&plain_text);
	for(text = TokenArrayNext(tokens, t); text->symbol != URL; )
		if(!(text = print_token(tokens, text))) goto catch;
	style_pop(), style_pop();
	/* We want to open this file to check if it's on the up-and-up. */
	if(!(errno = 0, fn = PathFromHere(turl->length, turl->from)))
		{ if(errno) goto catch; else goto raw; }
	if(!ImageDimension(fn, &width, &height)) goto raw;
	/* We want the path to print, now. */
	if(!(errno = 0, fn = PathFromOutput(turl->length, turl->from)))
		{ if(errno) goto catch; else goto raw; }
	if(CdocGetDebug()) fprintf(stderr, "%s: local image %s.\n", pos(t), fn);
	if(f == OUT_HTML) {
		printf("\" src = \"%s\" width = %u height = %u>", fn, width, height);
	} else {
		printf("](%s)", fn);
	}
	success = 1;
	goto finally;
raw:
	/* Maybe it's an external link? */
	if(CdocGetDebug()) fprintf(stderr, "%s: remote image %.*s.\n",
		pos(t), turl->length, turl->from);
	printf("%s%.*s%s", f == OUT_HTML ? "\" src = \"" : "](",
		turl->length, turl->from, f == OUT_HTML ? "\">" : ")");
	success = 1;
	goto finally;
catch:
	fprintf(stderr, "%s: expected `[description](url)`.\n", pos(t));
finally:
	*ptoken = TokenArrayNext(tokens, turl);
	return success;
}
OUT(nbsp) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NBSP && !is_buffer);
	style_prepare_output(t->symbol);
	printf("&nbsp;");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(nbthinsp) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NBTHINSP && !is_buffer);
	style_prepare_output(t->symbol);
	printf("&#8239;" /* "&thinsp;" <- breaking? */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(mathcalo) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATHCALO && !is_buffer);
	style_prepare_output(t->symbol);
	/* Omicron. It looks like a stylised "O"? The actual is "&#120030;" but
	 good luck finding a font that supports that. If one was using JavaScript
	 and had a constant connection, we could use MathJax. */
	printf("&#927;" /* "O" */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(ctheta) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == CTHETA && !is_buffer);
	style_prepare_output(t->symbol);
	printf("&#920;" /* "&Theta;" This is supported on more browsers. */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(comega) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == COMEGA && !is_buffer);
	style_prepare_output(t->symbol);
	printf("&#937;" /* "&Omega;" */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(times) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == TIMES && !is_buffer);
	style_prepare_output(t->symbol);
	printf("&#215;");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(cdot) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == CDOT && !is_buffer);
	style_prepare_output(t->symbol);
	printf("&#183;" /* &middot; */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(list) {
	const struct StyleText *const peek = style_text_peek();
	const struct Token *const t = *ptoken;
	const enum Format format = effective_format();
	const struct StyleText *const li = &styles[ST_LI][format],
		*const ul = &styles[ST_UL][format];
	assert(tokens && t && t->symbol == LIST_ITEM && peek && !is_buffer);
	if(peek == li) {
		style_pop_push();
	} else {
		style_pop_level();
		style_push(ul), style_push(li);
	}
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(pre) {
	const struct StyleText *const peek = style_text_peek();
	const struct Token *const t = *ptoken;
	const enum Format format = effective_format();
	const struct StyleText *const line = &styles[ST_PRELINE][format],
		*const pref = &styles[ST_PRE][format];
	assert(tokens && t && t->symbol == PREFORMATTED && peek && !is_buffer);
	if(peek != line) {
		style_pop_level();
		style_push(pref), style_push(line);
	}
	style_prepare_output(t->symbol);
	encode_len(t->length, t->from);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}



/* `SYMBOL` is declared in `Scanner.h`. */
static const OutFn symbol_outs[] = { SYMBOL(PARAM6C) };



/** Prints one [multi-]token sequence.
 @param[tokens] The token array that `token` is a part of.
 @param[token] The start token.
 @param[a] If non-null, prints to a string instead of `stdout`. Only variable
 name tokens support this.
 @throws[EILSEQ] Sequence error. Must detect with `errno`.
 @return The next token. */
static const struct Token *print_token_choose(
	const struct TokenArray *const tokens,
	const struct Token *token, const int is_buffer) {
	const OutFn sym_out = symbol_outs[token->symbol];
	assert(tokens && token);
	if(!sym_out) return fprintf(stderr, "%s: symbol output undefined.\n",
		pos(token)), TokenArrayNext(tokens, token);
	if(!sym_out(tokens, &token, is_buffer)) { errno = EILSEQ; return 0; }
	return token;
}

/** This is only for invididual tokens.
 @return The temporary buffer that stores the token. */
static const char *print_token_s(const struct TokenArray *const tokens,
	const struct Token *token) {
	BufferClear();
	print_token_choose(tokens, token, 1); /* Ignore return. */
	return BufferGet();
}

/**
 @return The next token to be printed. */
static const struct Token *print_token(const struct TokenArray *const tokens,
	const struct Token *token) {
	return print_token_choose(tokens, token, 0);
}

/** @param[highlights] Must be sorted if not null, creates an emphasis on those
 words.
 @throws[EILSEQ] Sequence error. Must detect with `errno`. */
static void highlight_tokens(const struct TokenArray *const tokens,
	const struct IndexArray *const highlights) {
	const struct Token *const first = TokenArrayNext(tokens, 0), *token = first;
	size_t *highlight = IndexArrayNext(highlights, 0);
	int is_highlight, is_first_highlight = 1;
	const enum Format f = effective_format();
	assert(tokens);
	if(!token) return;
	style_push(&plain_text);
	while(token) {
		if(highlight && *highlight == (size_t)(token - first)) {
			is_highlight = 1;
			highlight = IndexArrayNext(highlights, highlight);
		} else {
			is_highlight = 0;
		}
		if(is_highlight) style_highlight_on(&styles[is_first_highlight
			? ST_STRONG_HTML : ST_EM_HTML][f]);
		token = print_token(tokens, token);
		if(is_highlight) style_highlight_off(), is_first_highlight = 0;
	}
	style_pop();
}

static void print_tokens(const struct TokenArray *const tokens) {
	highlight_tokens(tokens, 0);
}



/** Prints the documentation part of `brief` and erases it. */
static void print_brief(void) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&brief, segment)))
		print_tokens(&segment->doc);
	segment_array_clear(&brief);
}



typedef int (*DivisionPredicate)(const enum Division);

/** @implements DivisionPredicate */
static int is_div_preamble(const enum Division d) {
	return d == DIV_PREAMBLE;
}

/** @implements DivisionPredicate */
static int is_not_div_preamble(const enum Division d) {
	return d != DIV_PREAMBLE;
}



/** @return The first token in `tokens` that matches `token`. */
static const struct Token *any_token(const struct TokenArray *const tokens,
	const struct Token *const token) {
	const struct Token *t = 0;
	while((t = TokenArrayNext(tokens, t)))
		if(!token_compare(token, t)) return t;
	return 0;
}

/** Functions as a bit-vector. */
enum AttShow { SHOW_NONE, SHOW_WHERE, SHOW_TEXT, SHOW_ALL };

static void segment_att_print_all(const struct Segment *const segment,
	const enum Symbol symbol, const struct Token *const match,
	const enum AttShow show) {
	struct Attribute *attribute = 0;
	assert(segment);
	if(!show) return;
	fprintf(stderr, "segment_att_print_all segment %s and symbol %s: %s.\n", divisions[segment->division], symbols[symbol], StyleArrayToString(&mode.styles));
	while((attribute = AttributeArrayNext(&segment->attributes, attribute))) {
		size_t *pindex;
		if(attribute->token.symbol != symbol
			|| (match && !any_token(&attribute->header, match))) continue;
		style_string_output();
		if(show & SHOW_WHERE) {
			if((pindex = IndexArrayNext(&segment->code_params, 0))
				&& *pindex < TokenArraySize(&segment->code)) {
				const struct Token *token
					= TokenArrayGet(&segment->code) + *pindex;
				style_push(&no_style);
				/* fixme: didn't I already do this? */
				if(effective_format() == OUT_HTML) {
					printf("<a href = \"#%s:",
						division_strings[segment->division]);
					print_token(&segment->code, token);
					printf("\">");
					print_token(&segment->code, token);
					printf("</a>");
				} else {
					/* fixme: this is wrong. */
					printf("[");
					print_token(&segment->code, token);
					printf("](#%s:", division_strings[segment->division]);
					print_token(&segment->code, token);
					printf(")");
				}
				style_pop();
			} else {
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
static void div_att_print(const DivisionPredicate div_pred,
	const enum Symbol symbol, const enum AttShow show) {
	struct Segment *segment = 0;
	if(!show) return;
	while((segment = SegmentArrayNext(&report, segment)))
		if(!div_pred || div_pred(segment->division))
		segment_att_print_all(segment, symbol, 0, show);
}



/** @return If there exist an `attribute_symbol` with content within
 `segment`. */
static int segment_attribute_exists(const struct Segment *const segment,
	const enum Symbol attribute_symbol) {
	struct Attribute *attribute = 0;
	assert(segment);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
		if(attribute->token.symbol == attribute_symbol
		   && TokenArraySize(&attribute->contents)) return 1;
	return 0;
}

/** @return Is `division` in the report? */
static int division_exists(const enum Division division) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment)))
		if(segment->division == division) return 1;
	return 0;
}

/** `act` on all `division`. */
static void division_act(const enum Division division,
	void (*act)(const struct Segment *const segment)) {
	const struct Segment *segment = 0;
	assert(act);
	while((segment = SegmentArrayNext(&report, segment)))
		if(segment->division == division) act(segment);
}

/** @return Is `attribute_symbol` in the report? (needed for `@licence`.) */
static int attribute_exists(const enum Symbol attribute_symbol) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment)))
		if(segment_attribute_exists(segment, attribute_symbol)) return 1;
	return 0;
}

/** Searches if attribute `symbol` exists within `segment` whose header
 contains `match`.
 @order O(`attributes`) */
static int segment_attribute_match_exists(const struct Segment *const segment,
	const enum Symbol symbol, const struct Token *const match) {
	struct Attribute *attribute = 0;
	assert(segment);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute))) {
		if(attribute->token.symbol != symbol) continue;
		if(any_token(&attribute->header, match)) return 1;
	}
	return 0;
}

static void dl_segment_att(const struct Segment *const segment,
	const enum Symbol attribute, const struct Token *match,
	const struct StyleText *const style) {
	const enum Format format = effective_format();
	assert(segment && attribute && style);
	if((match && !segment_attribute_match_exists(segment, attribute, match))
		|| (!match && !segment_attribute_exists(segment, attribute))) return;
	style_push(&styles[ST_DT][format]), style_push(&plain_text);
	style_string_output();
	printf("%s:", symbol_attribute_titles[attribute]);
	if(match) style_separate(), style_push(&styles[ST_EM][format]),
		print_token(&segment->code, match), style_pop();
	style_pop(), style_pop();
	style_push(&styles[ST_DD][format]), style_push(style),
		style_push(&plain_text);
	segment_att_print_all(segment, attribute, match, SHOW_TEXT);
	/* fixme */
	fprintf(stderr, "dl_segment_att for %s: %s.\n", symbols[attribute], StyleArrayToString(&mode.styles));
	style_pop(), style_pop(), style_pop();
}

/** This is used in preamble for attributes inside a `dl`.
 @param[is_recursive]  */
static void dl_preamble_att(const enum Symbol attribute,
	const enum AttShow show, const struct StyleText *const style) {
	const enum Format format = effective_format();
	assert(style);
	/* `style_title` is static in `Styles.h`. Hack. */
	if(format == OUT_HTML) sprintf(style_title, "\t<dt>%.128s:</dt>\n"
		"\t<dd>", symbol_attribute_titles[attribute]);
	else sprintf(style_title, " * %.128s:  \n   ",
		symbol_attribute_titles[attribute]);
	style_push(&styles[ST_DESC][format]), style_push(style),
		style_push(&plain_text);
	div_att_print(&is_div_preamble, attribute, SHOW_TEXT);
	style_pop(), style_push(&plain_parenthetic), style_push(&plain_csv),
	style_push(&plain_text);
	div_att_print(&is_not_div_preamble, attribute, show);
	/* fixme */
	fprintf(stderr, "dl_preamble_att for %s: %s.\n", symbols[attribute], StyleArrayToString(&mode.styles));
	style_pop(), style_pop(), style_pop(), style_pop(), style_pop();
}

static void dl_segment_specific_att(const struct Attribute *const attribute) {
	const enum Format format = effective_format();
	assert(attribute);
	style_push(&styles[ST_DT][format]), style_push(&plain_text);
	style_string_output();
	printf("%s:", symbol_attribute_titles[attribute->token.symbol]);
	if(TokenArraySize(&attribute->header)) {
		const struct Token *token = 0;
		style_separate();
		style_push(&plain_csv);
		while((token = TokenArrayNext(&attribute->header, token)))
			print_token(&attribute->header, token), style_separate();
		style_pop();
	}
	style_pop(), style_pop();
	style_push(&styles[ST_DD][format]), style_push(&plain_text);
	print_tokens(&attribute->contents);
	style_pop(), style_pop();
}

static void print_best_guess_at_modifiers(const struct Segment *const segment) {
	const struct Token *code = TokenArrayGet(&segment->code),
		*stop = code + *IndexArrayGet(&segment->code_params);
	assert(segment && segment->division == DIV_FUNCTION
		&& IndexArraySize(&segment->code_params) >= 1
		&& TokenArraySize(&segment->code)
		> *IndexArrayGet(&segment->code_params));
	while(code < stop) {
		if(code->symbol == LPAREN) {
			style_separate();
			style_push(&styles[ST_EM][effective_format()]);
			style_string_output();
			printf("function");
			style_pop();
			return;
		}
		code = print_token(&segment->code, code);
	}
}



/** Scan the `str`, used below. */
static void scan_doc_string(const char *const str) {
	struct Scanner *scan;
	assert(str);
	if(!(scan = Scanner("string", str, &notify_brief, SSDOC)))
		{ perror(str); unrecoverable(); return; }
	style_string_output(), print_brief();
	Scanner_(&scan);
}

/* @param[label] Un-escaped label. */
static void print_fragment_for(const enum Division d, const char *const label) {
	/* `effective_format` is NOT the thing we need; we need to raw format for
	 the link. */
	const enum Format f = CdocGetFormat();
	/* GCC is too smart but not smart enough. */
	const char *const fmt_html = "[%s](#%s:%s)",
		*const fmt_md = "[%s](#%s%s-%x)",
		*const division = division_strings[d];
	const unsigned hash = fnv_32a_str(label);
	size_t size;
	char *b;
	assert(label);
	/* fixme: snprintf is not defined in C89. */
	size = f == OUT_HTML ? snprintf(0, 0, fmt_html, label, division, label)
		: snprintf(0, 0, fmt_md, label, md_fragment_extra, division, hash);
	assert(size > 0);
	/* (Potentally) calling this with `label` as the other buffer. */
	BufferSwap();
	BufferClear();
	if(!(b = BufferPrepare(size))) { perror(label); unrecoverable(); return; }
	size = f == OUT_HTML ? sprintf(b, fmt_html, label, division, label)
		: sprintf(b, fmt_md, label, md_fragment_extra, division, hash);
	scan_doc_string(b);
	BufferSwap();
}

static void print_custom_heading_fragment_for(const char *const division,
	const char *const desc) {
	const char *const fmt
		= (effective_format() == OUT_HTML) ? "[%s](#%.0s%s:)" : "[%s](#%s%s)";
	/* fixme: snprintf is not defined in C89. */
	const size_t size = snprintf(0, 0, fmt, desc, md_fragment_extra, division);
	char *b;
	assert(division && desc && size > 0);
	BufferClear();
	if(!(b = BufferPrepare(size)))
		{ perror(division); unrecoverable(); return; }
	sprintf(b, fmt, desc, md_fragment_extra, division);
	scan_doc_string(b);
}

static void print_heading_fragment_for(const enum Division d) {
	print_custom_heading_fragment_for(division_strings[d], division_desc[d]);
}



/** @param[label] In HTML. */
static void print_anchor_for(const enum Division d, const char *const label) {
	const enum Format f = effective_format();
	const char *const division = division_strings[d];
	assert(label);
	style_push(&styles[ST_H3][f]);
	style_string_output();
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
	style_push(&to_html); /* The format is HTML because it's in an HTML tag. */
	encode(label);
	style_pop();
	fputs("</a>", stdout);
	style_pop(); /* h2 */
}

static void print_custom_heading_anchor_for(const char *const division,
	const char *const desc) {
	const enum Format f = effective_format();
	assert(division && desc);
	style_push(&styles[ST_H2][f]);
	style_string_output();
	printf("<a ");
	(f == OUT_HTML)
		? printf("id = \"%s:\" name = \"%s:\"", division, division)
		: printf("id = \"%s%s\" name = \"%s%s\"", md_fragment_extra, division,
		md_fragment_extra, division);
	printf(">%s</a>", desc);
	style_pop(); /* h2 */
}

static void print_heading_anchor_for(enum Division d) {
	print_custom_heading_anchor_for(division_strings[d], division_desc[d]);
}



/** Toc subcategories. */
static void print_toc_extra(const enum Division d) {
	struct Segment *segment = 0;
	size_t *idxs;
	struct Token *params;
	const char *b;
	printf(": ");
	style_push(&plain_csv), style_push(&no_style);
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != d) continue;
		if(!IndexArraySize(&segment->code_params)) { fprintf(stderr,
			"%s: segment has no title.\n", divisions[segment->division]);
			continue; }
		idxs = IndexArrayGet(&segment->code_params);
		params = TokenArrayGet(&segment->code);
		assert(idxs[0] < TokenArraySize(&segment->code));
		style_push(&to_raw);
		b = print_token_s(&segment->code, params + idxs[0]);
		style_pop();
		print_fragment_for(d, b);
		style_pop_push();
	}
	style_pop(), style_pop();
}



/** Prints all a `segment`.
 @implements division_act */
static void segment_print_all(const struct Segment *const segment) {
	const enum Format f = effective_format();
	const struct Token *param;
	const char *b;
	assert(segment && segment->division != DIV_PREAMBLE);
	style_push(&styles[ST_DIV][f]);

	/* The title is generally the first param. Only single-words. */
	if((param = param_no(segment, 0))) {
		style_push(&to_raw); /* Anchors are always raw. */
		b = print_token_s(&segment->code, param);
		style_pop();
		print_anchor_for(segment->division, b);
		style_push(&styles[ST_P][f]), style_push(&to_html);
		style_string_output();
		printf("<code>");
		highlight_tokens(&segment->code, &segment->code_params);
		printf("</code>");
		style_pop_level();
	} else {
		style_push(&styles[ST_H3][f]);
		style_string_output();
		printf("Unknown");
		style_pop_level();
	}

	/* Now text. */
	style_push(&styles[ST_P][f]);
	print_tokens(&segment->doc);
	style_pop_level();

	/* Attrubutes. */
	style_push(&styles[ST_DL][f]);
	if(segment->division == DIV_FUNCTION) {
		const struct Attribute *att = 0;
		size_t no;
		for(no = 1; (param = param_no(segment, no)); no++)
			dl_segment_att(segment, ATT_PARAM, param, &plain_text);
		dl_segment_att(segment, ATT_RETURN, 0, &plain_text);
		dl_segment_att(segment, ATT_IMPLEMENTS, 0, &plain_csv);
		while((att = AttributeArrayNext(&segment->attributes, att))) {
			if(att->token.symbol != ATT_THROWS) continue;
			dl_segment_specific_att(att);
		}
		dl_segment_att(segment, ATT_ORDER, 0, &plain_text);
	} else if(segment->division == DIV_TAG) {
		const struct Attribute *att = 0;
		while((att = AttributeArrayNext(&segment->attributes, att))) {
			if(att->token.symbol != ATT_PARAM) continue;
			dl_segment_specific_att(att);
		}
	}
	dl_segment_att(segment, ATT_AUTHOR, 0, &plain_csv);
	dl_segment_att(segment, ATT_STD, 0, &plain_ssv);
	dl_segment_att(segment, ATT_DEPEND, 0, &plain_ssv);
	dl_segment_att(segment, ATT_FIXME, 0, &plain_text);
	dl_segment_att(segment, ATT_LICENSE, 0, &plain_text);
	dl_segment_att(segment, ATT_CF, 0, &plain_ssv);
	style_pop_level(); /* dl */
	style_pop_level(); /* div */
}



/** Outputs a report.
 @throws[EILSEQ] Sequence error.
 @return Success. */
int ReportOut(void) {
	const char *const summary = "summary",
		*const summary_desc = "Function Summary",
		*const license = "license",
		*const license_desc = symbol_attribute_titles[ATT_LICENSE];
	const int is_preamble = division_exists(DIV_PREAMBLE),
		is_function = division_exists(DIV_FUNCTION),
		is_tag = division_exists(DIV_TAG),
		is_typedef = division_exists(DIV_TYPEDEF),
		is_data = division_exists(DIV_DATA),
		is_license = attribute_exists(ATT_LICENSE);
	const struct Segment *segment = 0;
	const enum Format format = effective_format();
	const char *const in_fn = CdocGetInput(),
		*const base_fn = strrchr(in_fn, *path_dirsep),
		*const title = base_fn ? base_fn + 1 : in_fn;

	assert(in_fn);

	/* Set `errno` here so that we don't have to test output each time. */
	errno = 0;
	if(format == OUT_HTML) {
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
		style_push(&html_title), style_push(&plain_ssv),
			style_push(&plain_text);
		style_string_output();
		encode(title);
		style_pop_level();
		assert(!StyleArraySize(&mode.styles));
		printf("</head>\n\n"
			"<body>\n\n");
	}
	/* Title. */
	style_push(&styles[ST_H1][format]), style_push(&plain_ssv),
		style_push(&plain_text);
	style_string_output();
	encode(title);
	style_pop_level();
	assert(!StyleArraySize(&mode.styles));

	/* Subtitle. */
	style_push(&styles[ST_H2][format]), style_push(&plain_ssv),
	style_push(&plain_text);
	div_att_print(&is_div_preamble, ATT_SUBTITLE, SHOW_TEXT);
	style_pop_level();
	assert(!StyleArraySize(&mode.styles));

	/* TOC. */
	style_push(&styles[ST_UL][format]), style_push(&styles[ST_LI][format]);
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
	style_pop_level();
	assert(!StyleArraySize(&mode.styles));

	/* Preamble contents; it shows up as the more-aptly nammed "desciption" but
	 I didn't want to type that much. */
	if(is_preamble) {
		style_push(&styles[ST_DIV][format]);
		print_heading_anchor_for(DIV_PREAMBLE);
		while((segment = SegmentArrayNext(&report, segment))) {
			if(segment->division != DIV_PREAMBLE) continue;
			style_push(&styles[ST_P][format]);
			print_tokens(&segment->doc);
			style_pop_level();
		}
		style_push(&styles[ST_DL][format]);
		/* `ATT_TITLE` is above. */
		while((segment = SegmentArrayNext(&report, segment))) {
			const struct Attribute *att = 0;
			if(segment->division != DIV_PREAMBLE) continue;
			while((att = AttributeArrayNext(&segment->attributes, att))) {
				if(att->token.symbol != ATT_PARAM) continue;
				dl_segment_specific_att(att);
			}
		}
		dl_preamble_att(ATT_AUTHOR, SHOW_ALL, &plain_csv);
		dl_preamble_att(ATT_STD, SHOW_ALL, &plain_csv);
		dl_preamble_att(ATT_DEPEND, SHOW_ALL, &plain_csv);
		dl_preamble_att(ATT_FIXME, SHOW_WHERE, &plain_text);
		dl_preamble_att(ATT_CF, SHOW_ALL, &plain_ssv);
		/* `ATT_RETURN`, `ATT_THROWS`, `ATT_IMPLEMENTS`, `ATT_ORDER`,
		 `ATT_ALLOW` have warnings. `ATT_LICENSE` is below. */
		style_pop_level();
		style_pop_level();
	}
	assert(!StyleArraySize(&mode.styles));

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
		style_push(&styles[ST_DIV][format]);
		print_custom_heading_anchor_for(summary, summary_desc);
		style_push(&to_html);
		style_string_output();
		printf("<table>\n\n"
			   "<tr><th>Modifiers</th><th>Function Name</th>"
			   "<th>Argument List</th></tr>\n\n");
		while((segment = SegmentArrayNext(&report, segment))) {
			struct Token *params;
			size_t *idxs, idxn, idx, paramn;
			const char *b;
			if(segment->division != DIV_FUNCTION
			   || !(idxn = IndexArraySize(&segment->code_params))) continue;
			idxs = IndexArrayGet(&segment->code_params);
			params = TokenArrayGet(&segment->code);
			paramn = TokenArraySize(&segment->code);
			assert(idxs[0] < paramn);
			printf("<tr><td align = right>");
			style_push(&plain_text);
			print_best_guess_at_modifiers(segment);
			style_pop();
			printf("</td><td>");
			style_push(&to_raw); /* Always get raw; translate after. */
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
		assert(!StyleArraySize(&mode.styles));

		/* Functions. */
		style_push(&styles[ST_DIV][format]);
		print_heading_anchor_for(DIV_FUNCTION);
		division_act(DIV_FUNCTION, &segment_print_all);
		style_pop_level();
	}
	/* License. */
	if(is_license) {
		style_push(&styles[ST_DIV][format]);
		print_custom_heading_anchor_for(license, license_desc);
		style_push(&styles[ST_P][format]);
		div_att_print(&is_div_preamble, ATT_LICENSE, SHOW_TEXT);
		style_pop_push();
		style_push(&plain_see_license), style_push(&plain_text);
		div_att_print(&is_not_div_preamble, ATT_LICENSE, SHOW_WHERE);
		style_pop_level();
		style_pop_level();
	}

	if(format == OUT_HTML) printf("</body>\n\n"
		"</html>\n");
	style_clear();
	return errno ? 0 : 1;
}
