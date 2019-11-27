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
	style_push(&styles[ST_P][CdocGetFormat()]);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(lit) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->length > 0 && t->from);
	if(is_buffer) {
		encode_len_s(t->length, t->from);
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
	const enum Format f = CdocGetFormat();
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
	const enum Format f = CdocGetFormat();
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
	const enum Format f = CdocGetFormat();
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
	/* I think it can't contain '<>()\"' by parser. */
	if(CdocGetFormat() == OUT_HTML) {
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
	if(CdocGetFormat() == OUT_HTML) {
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
OUT(see_fn) {
	const struct Token *const fn = *ptoken;
	assert(tokens && fn && fn->symbol == SEE_FN && !is_buffer);
	style_prepare_output(fn->symbol);
	if(CdocGetFormat() == OUT_HTML) {
		printf("<a href = \"#%s:", division_strings[DIV_FUNCTION]);
		encode_len(fn->length, fn->from);
		printf("\">");
		encode_len(fn->length, fn->from);
		printf("</a>");
	} else {
		printf("[");
		encode_len(fn->length, fn->from);
		printf("](%s:", division_strings[DIV_FUNCTION]);
		encode_len(fn->length, fn->from);
		printf(")");
	}
	*ptoken = TokenArrayNext(tokens, fn);
	return 1;
}
OUT(see_tag) {
	const struct Token *const tag = *ptoken;
	assert(tokens && tag && tag->symbol == SEE_TAG && !is_buffer);
	style_prepare_output(tag->symbol);
	if(CdocGetFormat() == OUT_HTML) {
		printf("<a href = \"#%s:", division_strings[DIV_TAG]);
		encode_len(tag->length, tag->from);
		printf("\">");
		encode_len(tag->length, tag->from);
		printf("</a>");
	} else {
		printf("[");
		encode_len(tag->length, tag->from);
		printf("](%s:", division_strings[DIV_TAG]);
		encode_len(tag->length, tag->from);
		printf(")");
	}
	*ptoken = TokenArrayNext(tokens, tag);
	return 1;
}
OUT(see_typedef) {
	const struct Token *const def = *ptoken;
	assert(tokens && def && def->symbol == SEE_TYPEDEF && !is_buffer);
	style_prepare_output(def->symbol);
	if(CdocGetFormat() == OUT_HTML) {
		printf("<a href = \"#%s:", division_strings[DIV_TYPEDEF]);
		encode_len(def->length, def->from);
		printf("\">");
		encode_len(def->length, def->from);
		printf("</a>");
	} else {
		printf("[");
		encode_len(def->length, def->from);
		printf("](%s:", division_strings[DIV_TYPEDEF]);
		encode_len(def->length, def->from);
		printf(")");
	}
	*ptoken = TokenArrayNext(tokens, def);
	return 1;
}
OUT(see_data) {
	const struct Token *const data = *ptoken;
	assert(tokens && data && data->symbol == SEE_DATA && !is_buffer);
	style_prepare_output(data->symbol);
	if(CdocGetFormat() == OUT_HTML) {
		printf("<a href = \"#%s:", division_strings[DIV_DATA]);
		encode_len(data->length, data->from);
		printf("\">");
		encode_len(data->length, data->from);
		printf("</a>");
	} else {
		printf("[");
		encode_len(data->length, data->from);
		printf("](%s:", division_strings[DIV_DATA]);
		encode_len(data->length, data->from);
		printf(")");
	}
	*ptoken = TokenArrayNext(tokens, data);
	return 1;
}
OUT(math_begin) { /* Math and code. */
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_BEGIN && !is_buffer);
	style_push(&styles[ST_CODE][CdocGetFormat()]);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(math_end) {
	const struct Token *const t = *ptoken;
	const struct StyleText *const st = style_text_peek(),
		*const st_expect = &styles[ST_CODE][CdocGetFormat()];
	assert(tokens && t && t->symbol == MATH_END && !is_buffer);
	if(st != st_expect) {
		char st_str[12], st_expect_str[12];
		style_text_to_string(st, &st_str);
		style_text_to_string(st_expect, &st_expect_str);
		return fprintf(stderr, "Expected %s but got %s.\n",
			st_expect_str, st_str), 0;
	}
	style_pop();
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_begin) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_BEGIN && !is_buffer);
	style_push(&styles[ST_EM][CdocGetFormat()]);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_end) {
	const struct Token *const t = *ptoken;
	const struct StyleText *const st = style_text_peek(),
		*const st_expect = &styles[ST_EM][CdocGetFormat()];
	assert(tokens && t && t->symbol == EM_END && !is_buffer);
	if(st != st_expect) {
		char st_str[12], st_expect_str[12];
		style_text_to_string(st, &st_str);
		style_text_to_string(st_expect, &st_expect_str);
		return fprintf(stderr, "Expected %s but got %s.\n",
			st_expect_str, st_str), 0;
	}
	style_pop();
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(link) {
	const struct Token *const t = *ptoken, *text, *turl;
	const enum Format f = CdocGetFormat();
	const char *fn, *uri;
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
		fprintf(stderr, "%s: fixed link %.*s.\n", pos(t), (int)fn_len, fn);
output:
	assert(fn_len <= INT_MAX);
	if(!(uri = AnchorHref(fn_len, fn))) goto catch;
	if(f == OUT_HTML) printf("<a href = \"%s\">", uri);
	else printf("[");
	for(text = TokenArrayNext(tokens, t); text->symbol != URL; )
		if(!(text = print_token(tokens, text))) goto catch;
	if(f == OUT_HTML) printf("</a>");
	else printf("](%s)", uri);
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
	const enum Format f = CdocGetFormat();
	const char *fn;
	unsigned width = 1, height = 1;
	int success = 0;
	assert(tokens && t && t->symbol == IMAGE_START && !is_buffer);
	style_prepare_output(t->symbol);
	/* The expected format is IMAGE_START [^URL]* URL. */
	for(turl = TokenArrayNext(tokens, t); turl->symbol != URL;
		turl = TokenArrayNext(tokens, turl)) if(!turl) goto catch;
	printf("%s", f == OUT_HTML ? "<img alt = \"" : "![");
	for(text = TokenArrayNext(tokens, t); text->symbol != URL; )
		if(!(text = print_token(tokens, text))) goto catch;
	/* We want to open this file to check if it's on the up-and-up. */
	if(!(errno = 0, fn = PathFromHere(turl->length, turl->from)))
		{ if(errno) goto catch; else goto raw; }
	if(!ImageDimension(fn, &width, &height)) goto raw;
	/* We want the path to print, now. */
	if(!(errno = 0, fn = PathFromOutput(turl->length, turl->from)))
		{ if(errno) goto catch; else goto raw; }
	if(CdocGetDebug())
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
	const enum Format format = CdocGetFormat();
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
	const enum Format format = CdocGetFormat();
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
 @param[a] If non-null, prints to a string instead of `stdout`. Only certian
 tokens (variable names) support this.
 @throws[EILSEQ] Sequence error. Must detect with `errno`.
 @return The next token.
 @fixme This 256-byte buffer is lame-o; use path to build a real one. */
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

static const struct Token *print_token_s(const struct TokenArray *const tokens,
	const struct Token *token) {
	return print_token_choose(tokens, token, 1);
}

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
	const enum Format f = CdocGetFormat();
	assert(tokens);
	if(!token) return;
	while(token) {
		if(highlight && *highlight == (size_t)(token - first)) {
			is_highlight = 1;
			highlight = IndexArrayNext(highlights, highlight);
		} else {
			is_highlight = 0;
		}
		if(is_highlight) style_highlight_on(&styles[is_first_highlight
			? ST_CODE_STRONG : ST_CODE_EM][f]);
		token = print_token(tokens, token);
		if(is_highlight) style_highlight_off(), is_first_highlight = 0;
	}
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
	while((attribute = AttributeArrayNext(&segment->attributes, attribute))) {
		size_t *pindex;
		if(attribute->token.symbol != symbol
			|| (match && !any_token(&attribute->header, match))) continue;
		style_prepare_output(END);
		if(show & SHOW_WHERE) {
			if((pindex = IndexArrayNext(&segment->code_params, 0))
				&& *pindex < TokenArraySize(&segment->code)) {
				const struct Token *token
					= TokenArrayGet(&segment->code) + *pindex;
				style_push(&no_style);
				printf("<a href = \"#%s:", division_strings[segment->division]);
				print_token(&segment->code, token);
				printf("\">");
				print_token(&segment->code, token);
				printf("</a>");
				style_pop();
			} else {
				printf("%s", division_strings[segment->division]);
			}
		}
		if(show == SHOW_ALL) fputs(": ", stdout);
		if(show & SHOW_TEXT) print_tokens(&attribute->contents);
		style_pop_push();
	}
}

/** For each `division` segment, print all attributes that match `symbol`.
 @param[is_where] Prints where is is.
 @param[is_text] Prints the text. */
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
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != division) continue;
		act(segment);
	}
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
	const enum Format format = CdocGetFormat();
	assert(segment && attribute && style);
	if((match && !segment_attribute_match_exists(segment, attribute, match))
		|| (!match && !segment_attribute_exists(segment, attribute))) return;
	style_push(&styles[ST_DT][format]), style_push(&plain_text);
	style_prepare_output(END);
	printf("%s:", symbol_attribute_titles[attribute]);
	if(match) style_separate(), style_push(&styles[ST_EM][format]),
		print_token(&segment->code, match), style_pop();
	style_pop(), style_pop();
	style_push(&styles[ST_DD][format]), style_push(style),
		style_push(&plain_text);
	segment_att_print_all(segment, attribute, match, SHOW_TEXT);
	style_pop(), style_pop(), style_pop();
}

/** This is used in preamble for attributes inside a `dl`.
 @param[is_recursive]  */
static void dl_preamble_att(const enum Symbol attribute,
	const enum AttShow show, const struct StyleText *const style) {
	const enum Format format = CdocGetFormat();
	assert(style);
	/* `style_title` is static in `Styles.h`. */
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
	style_pop(), style_pop(), style_pop(), style_pop(), style_pop();
}

static void dl_segment_specific_att(const struct Attribute *const attribute) {
	const enum Format format = CdocGetFormat();
	assert(attribute);
	style_push(&styles[ST_DT][format]), style_push(&plain_text);
	style_prepare_output(END);
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


/** Prints all a `segment`.
 @implements division_act */
static void segment_print_all(const struct Segment *const segment) {
	const enum Format format = CdocGetFormat();
	const struct Token *param;
	assert(segment && segment->division != DIV_PREAMBLE);
	style_push(&styles[ST_DIV][format]);

	/* The title is generally the first param. Only single-words. */
	if((param = param_no(segment, 0))) {
		style_push(&styles[ST_H3][format]);
		style_prepare_output(END);
		printf("<a name = \"%s:", division_strings[segment->division]);
		print_token(&segment->code, param);
		printf("\">");
		print_token(&segment->code, param);
		printf("</a>");
		style_pop_level();
		style_push(&styles[ST_P][format]), style_push(&styles[ST_CODE][format]);
		highlight_tokens(&segment->code, &segment->code_params);
		style_pop_level();
	} else {
		style_push(&styles[ST_H3][format]);
		style_prepare_output(END);
		printf("Unknown");
		style_pop_level();
	}

	/* Now text. */
	style_push(&styles[ST_P][format]);
	print_tokens(&segment->doc);
	style_pop_level();

	/* Attrubutes. */
	style_push(&styles[ST_DL][format]);
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
	style_pop_level(); /* dl */
	style_pop_level(); /* div */
}

static void best_guess_at_modifiers(const struct Segment *const segment) {
	const struct Token *code = TokenArrayGet(&segment->code),
		*stop = code + *IndexArrayGet(&segment->code_params);
	assert(segment && segment->division == DIV_FUNCTION
		&& IndexArraySize(&segment->code_params) >= 1
		&& TokenArraySize(&segment->code)
		> *IndexArrayGet(&segment->code_params));
	while(code < stop) {
		if(code->symbol == LPAREN) {
			style_separate();
			style_push(&styles[ST_EM][CdocGetFormat()]);
			style_prepare_output(END);
			printf("function");
			style_pop();
			return;
		}
		code = print_token(&segment->code, code);
	}
}

/** Takes `url` and `desc` and makes a link. Don't go overboard on the buffer,
 only for fixed links. */
static int output_internal_link(const char *const label,
	const char *const desc) {
	const char *const fmt = "[%s](#%s:)";
	struct Scanner *scan_str;
	size_t size;
	char *b;
	BufferClear();
	size = snprintf(0, 0, fmt, desc, label);
	if(!(b = BufferPrepare(size))) return 0;
	sprintf(b, fmt, desc, label);
	if(!(scan_str = Scanner(label, b, &notify_brief, SSDOC))) return 0;
	style_prepare_output(END);
	print_brief();
	Scanner_(&scan_str);
	return 1;
}

/** Outputs a report.
 @throws[EILSEQ] Sequence error.
 @return Success. */
int ReportOut(void) {
	const int is_preamble = division_exists(DIV_PREAMBLE),
		is_function = division_exists(DIV_FUNCTION),
		is_tag = division_exists(DIV_TAG),
		is_typedef = division_exists(DIV_TYPEDEF),
		is_data = division_exists(DIV_DATA),
		is_license = attribute_exists(ATT_LICENSE);
	const struct Segment *segment = 0;
	const enum Format format = CdocGetFormat();
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
		style_prepare_output(END);
		encode(title);
		style_pop_level();
		assert(!StyleArraySize(&mode.styles));
		printf("</head>\n\n"
			"<body>\n\n");
	}
	/* Title. */
	style_push(&styles[ST_H1][format]), style_push(&plain_ssv),
		style_push(&plain_text);
	style_prepare_output(END);
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
	if(is_preamble) {
		if(!output_internal_link(division_strings[DIV_PREAMBLE],
			division_desc[DIV_PREAMBLE])) return 0;
		style_pop_push();
	}
	if(is_typedef) {
		if(!output_internal_link(division_strings[DIV_TYPEDEF],
			division_desc[DIV_TYPEDEF])) return 0;
		printf(": ");
		style_push(&plain_csv), style_push(&no_style);
		while((segment = SegmentArrayNext(&report, segment))) {
			size_t *idxs;
			struct Token *params;
			if(segment->division != DIV_TYPEDEF
				|| !IndexArraySize(&segment->code_params)) continue;
			idxs = IndexArrayGet(&segment->code_params);
			params = TokenArrayGet(&segment->code);
			assert(idxs[0] < TokenArraySize(&segment->code));
			style_prepare_output(END);
			printf("<a href = \"#%s:",
				division_strings[DIV_TYPEDEF]);
			print_token(&segment->code, params + idxs[0]);
			printf("\">");
			print_token(&segment->code, params + idxs[0]);
			printf("</a>");
			style_pop_push();
		}
		style_pop(), style_pop();
		style_pop_push();
	}
	if(is_tag) {
		if(!output_internal_link(division_strings[DIV_TAG],
			division_desc[DIV_TAG])) return 0;
		printf(": ");
		style_push(&plain_csv), style_push(&no_style);
		while((segment = SegmentArrayNext(&report, segment))) {
			size_t *idxs;
			struct Token *params;
			if(segment->division != DIV_TAG
				|| !IndexArraySize(&segment->code_params)) continue;
			idxs = IndexArrayGet(&segment->code_params);
			params = TokenArrayGet(&segment->code);
			assert(idxs[0] < TokenArraySize(&segment->code));
			style_prepare_output(END);
			printf("<a href = \"#%s:", division_strings[DIV_TAG]);
			print_token(&segment->code, params + idxs[0]);
			printf("\">");
			print_token(&segment->code, params + idxs[0]);
			printf("</a>");
			style_pop_push();
		}
		style_pop(), style_pop();
		style_pop_push();
	}
	if(is_data) {
		if(!output_internal_link(division_strings[DIV_DATA],
			division_desc[DIV_DATA])) return 0;
		printf(": ");
		style_push(&plain_csv), style_push(&no_style);
		while((segment = SegmentArrayNext(&report, segment))) {
			size_t *idxs;
			struct Token *params;
			if(segment->division != DIV_DATA
				|| !IndexArraySize(&segment->code_params)) continue;
			idxs = IndexArrayGet(&segment->code_params);
			params = TokenArrayGet(&segment->code);
			assert(idxs[0] < TokenArraySize(&segment->code));
			style_prepare_output(END);
			printf("<a href = \"#%s:", division_strings[DIV_DATA]);
			print_token(&segment->code, params + idxs[0]);
			printf("\">");
			print_token(&segment->code, params + idxs[0]);
			printf("</a>");
			style_pop_push();
		}
		style_pop(), style_pop();
		style_pop_push();
	}
	if(is_function) {
		if(!output_internal_link("summary", "Function Summary")) return 0;
		style_pop_push();
		if(!output_internal_link(division_strings[DIV_FUNCTION],
			division_desc[DIV_FUNCTION])) return 0;
		style_pop_push();
	}
	if(is_license) output_internal_link("license",
		symbol_attribute_titles[ATT_LICENSE]), style_pop_push();
	style_pop_level();
	assert(!StyleArraySize(&mode.styles));

	/* Preamble contents; it shows up as the more-aptly nammed "desciption" but
	 I didn't want to type that much. */
	if(is_preamble) {
		style_push(&styles[ST_DIV][format]), style_push(&styles[ST_H2][format]);
		style_prepare_output(END);
		printf("<a name = \"%s:\">Description</a>",
			division_strings[DIV_PREAMBLE]);
		style_pop(); /* h2 */
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
		/* `ATT_RETURN`, `ATT_THROWS`, `ATT_IMPLEMENTS`, `ATT_ORDER`,
		 `ATT_ALLOW` have warnings. `ATT_LICENSE` is below. */
		style_pop_level();
		style_pop_level();
	}
	assert(!StyleArraySize(&mode.styles));

	/* Print typedefs. */
	if(is_typedef) {
		style_push(&styles[ST_DIV][format]), style_push(&styles[ST_H2][format]);
		style_prepare_output(END);
		printf("<a name = \"%s:\">Typedef Aliases</a>",
			division_strings[DIV_TYPEDEF]);
		style_pop(); /* h2 */
		division_act(DIV_TYPEDEF, &segment_print_all);
		style_pop_level();
	}
	/* Print tags. */
	if(is_tag) {
		style_push(&styles[ST_DIV][format]), style_push(&styles[ST_H2][format]);
		style_prepare_output(END);
		printf("<a name = \"%s:\">Struct, Union, and Enum Definitions</a>",
			division_strings[DIV_TAG]);
		style_pop(); /* h2 */
		division_act(DIV_TAG, &segment_print_all);
		style_pop_level();
	}
	/* Print general declarations. */
	if(is_data) {
		style_push(&styles[ST_DIV][format]), style_push(&styles[ST_H2][format]);
		style_prepare_output(END);
		printf("<a name = \"%s:\">General Definitions</a>",
			division_strings[DIV_DATA]);
		style_pop(); /* h2 */
		division_act(DIV_DATA, &segment_print_all);
		style_pop_level();
	}
	/* Print functions. */
	if(is_function) {
		/* Function table. */
		style_push(&styles[ST_DIV][format]), style_push(&styles[ST_H2][format]);
		style_prepare_output(END);
		printf("<a name = \"summary:\">Function Summary</a>");
		style_pop(); /* h2 */
		style_prepare_output(END);
		printf("<table>\n\n"
			   "<tr><th>Modifiers</th><th>Function Name</th>"
			   "<th>Argument List</th></tr>\n\n");
		while((segment = SegmentArrayNext(&report, segment))) {
			size_t *idxs, idxn, idx, paramn;
			struct Token *params;
			if(segment->division != DIV_FUNCTION
			   || !(idxn = IndexArraySize(&segment->code_params))) continue;
			idxs = IndexArrayGet(&segment->code_params);
			params = TokenArrayGet(&segment->code);
			paramn = TokenArraySize(&segment->code);
			assert(idxs[0] < paramn);
			printf("<tr><td align = right>");
			style_push(&plain_text);
			best_guess_at_modifiers(segment);
			style_pop();
			printf("</td><td><a href = \"#%s:",
				   division_strings[DIV_FUNCTION]);
			print_token(&segment->code, params + idxs[0]);
			printf("\">");
			print_token(&segment->code, params + idxs[0]);
			printf("</a></td><td>");
			for(idx = 1; idx < idxn; idx++) {
				assert(idxs[idx] < paramn);
				if(idx > 1) printf(", ");
				print_token(&segment->code, params + idxs[idx]);
			}
			printf("</td></tr>\n\n");
		}
		printf("</table>\n\n");
		style_pop();
		assert(!StyleArraySize(&mode.styles));

		/* Functions. */
		style_push(&styles[ST_DIV][format]), style_push(&styles[ST_H2][format]);
		style_prepare_output(END);
		printf("<a name = \"%s:\">Function Definitions</a>",
			division_strings[DIV_FUNCTION]);
		style_pop(); /* h2 */
		division_act(DIV_FUNCTION, &segment_print_all);
		style_pop_level();
	}
	/* License. */
	if(is_license) {
		style_push(&styles[ST_DIV][format]), style_push(&styles[ST_H2][format]);
		style_prepare_output(END);
		printf("<a name = \"license:\">%s</a>",
			symbol_attribute_titles[ATT_LICENSE]);
		style_pop(); /* h2 */
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
