#include "reporttoken.h"
#include <stdio.h>

/* Some output functions need this. */
static int print_token(struct token_array_cursor *);

/** Selects `token` out of `tokens` and prints it and returns the next token. */
typedef int (*out_fn)(struct token_array_cursor *);
/*typedef int (*out_fn)(const struct token_array *const tokens,
	const struct token **const ptoken, const int is_buffer);*/
/* @param[ptoken] Is an [in/out] variable, it should get updated unless the
 return value is false.
 @return Success.
 @implements <Attribute>Predicate
 @fixme `is_buffer` should always be true for text-wrapping. */
/*#define OUT(name) static int out_##name(const struct token_array *const tokens,\
	const struct token **ptoken, const int is_buffer)*/

static int out_ws(struct token_array_cursor *tok) {
	(void)tok;
	style_separate();
	return 1;
}
static int out_par(struct token_array_cursor *tok) {
	(void)tok;
	style_pop_strong();
	style_push(ST_P);
	return 1;
}
static int out_lit(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->length > 0 && t->from);
	if(report_is_buffer) {
		style_encode_length_cat_to_buffer(t->length, t->from);
	} else {
		style_flush_symbol(t->symbol);
		style_encode_length(t->length, t->from);
	}
	return 1;
}
static int out_gen1(struct token_array_cursor *tok) {
	const char *const format = style_format() == OUT_HTML
		? HTML_LT "%.*s" HTML_GT "%.*s" : "<%.*s>%.*s";
	struct token *gen1, *lparen, *param1, *rparen;
	const char *cursor, *type1;
	size_t type1_size;
	gen1 = token_array_look(tok), assert(gen1->symbol == ID_ONE_GENERIC);
	if(!(token_array_next(tok), token_array_exists(tok))
		|| !(lparen = token_array_look(tok), lparen->symbol == LPAREN)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(param1 = token_array_look(tok), param1->symbol == ID)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(rparen = token_array_look(tok), rparen->symbol == RPAREN)
		) goto catch;
	/* fixme: Check to make sure by ensuring the entire file is <= INT_MAX. */
	type1 = gen1->from;
	cursor = strchr(type1, '_');
	type1_size = (size_t)(cursor - type1);
	assert(gen1->length == (size_t)(cursor + 1 - gen1->from)
		&& gen1->length > 1 && gen1->length <= INT_MAX
		&& type1_size <= INT_MAX && param1->length <= INT_MAX);
	if(report_is_buffer /* static */) {
		const char *const ltgt = style_format() == OUT_HTML
			? HTML_LT HTML_GT : "<>"; /* Only need the length. */
		char *a;
		if(!(a = buffer_prepare(strlen(ltgt) + type1_size + param1->length)))
			return 0;
		sprintf(a, format,
			(int)type1_size, type1, (int)param1->length, param1->from);
	} else {
		style_flush_symbol(gen1->symbol);
		printf(format,
			(int)type1_size, type1, (int)param1->length, param1->from);
	}
	return 1;
catch:
	fprintf(stderr, "%s: expected A_(id) %s.\n", pos(gen1),
		token_array_to_string(tok->a));
	return 0;
}
static int out_gen2(struct token_array_cursor *tok) {
	const char *const format = style_format() == OUT_HTML ? HTML_LT "%.*s"
		HTML_GT "%.*s" HTML_LT "%.*s" HTML_GT "%.*s" : "<%.*s>%.*s<%.*s>%.*s";
	struct token *gen2, *lparen, *param1, *comma, *param2, *rparen;
	const char *cursor, *type1, *type2;
	size_t type1_size, type2_size;
	gen2 = token_array_look(tok), assert(gen2->symbol == ID_TWO_GENERICS);
	if(!(token_array_next(tok), token_array_exists(tok))
		|| !(lparen = token_array_look(tok), lparen->symbol == LPAREN)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(param1 = token_array_look(tok), param1->symbol == ID)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(comma = token_array_look(tok), comma->symbol == COMMA)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(param2 = token_array_look(tok), param2->symbol == ID)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(rparen = token_array_look(tok), rparen->symbol == RPAREN)
		) goto catch;
	type1 = gen2->from;
	cursor = strchr(type1, '_');
	type1_size = (size_t)(cursor - type1);
	type2 = cursor + 1;
	cursor = strchr(type2, '_');
	type2_size = (size_t)(cursor - type2);
	assert(gen2->length == (size_t)(cursor + 1 - gen2->from)
		&& gen2->length > 2 && gen2->length <= INT_MAX
		&& type1_size <= INT_MAX && type2_size <= INT_MAX
		&& param1->length <= INT_MAX && param2->length <= INT_MAX);
	if(report_is_buffer /* static */) {
		const char *const ltgt = style_format() == OUT_HTML
			? HTML_LT HTML_GT : "<>";
		char *a;
		if(!(a = buffer_prepare(2 * strlen(ltgt)
			+ type1_size + param1->length
			+ type2_size + param2->length))) return 0;
		sprintf(a, format,
			(int)type1_size, type1, (int)param1->length, param1->from,
			(int)type2_size, type2, (int)param2->length, param2->from);
	} else {
		style_flush_symbol(gen2->symbol);
		printf(format,
			(int)type1_size, type1, (int)param1->length, param1->from,
			(int)type2_size, type2, (int)param2->length, param2->from);
	}
	return 1;
catch:
	fprintf(stderr, "%s: expected A_B_(id,id).\n", pos(gen2));
	return 0;
}
static int out_gen3(struct token_array_cursor *tok) {
	const char *const format = style_format() == OUT_HTML ? HTML_LT "%.*s"
		HTML_GT "%.*s" HTML_LT "%.*s" HTML_GT "%.*s" HTML_LT "%.*s" HTML_GT
		"%.*s" : "<%.*s>%.*s<%.*s>%.*s<%.*s>%.*s";
	struct token *gen3, *lparen, *param1, *comma1, *param2, *comma2, *param3,
		*rparen;
	const char *cursor, *type1, *type2, *type3;
	size_t type1_size, type2_size, type3_size;
	gen3 = token_array_look(tok), assert(gen3->symbol == ID_THREE_GENERICS);
	if(!(token_array_next(tok), token_array_exists(tok))
		|| !(lparen = token_array_look(tok), lparen->symbol == LPAREN)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(param1 = token_array_look(tok), param1->symbol == ID)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(comma1 = token_array_look(tok), comma1->symbol == COMMA)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(param2 = token_array_look(tok), param2->symbol == ID)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(comma2 = token_array_look(tok), comma1->symbol == COMMA)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(param3 = token_array_look(tok), param3->symbol == ID)
		|| !(token_array_next(tok), token_array_exists(tok))
		|| !(rparen = token_array_look(tok), rparen->symbol == RPAREN)
		) goto catch;
	type1 = gen3->from;
	cursor = strchr(type1, '_');
	type1_size = (size_t)(cursor - type1);
	type2 = cursor + 1;
	cursor = strchr(type2, '_');
	type2_size = (size_t)(cursor - type2);
	type3 = cursor + 1;
	cursor = strchr(type3, '_');
	type3_size = (size_t)(cursor - type3);
	assert(gen3->length == (size_t)(cursor + 1 - gen3->from)
		&& gen3->length > 2 && gen3->length <= INT_MAX
		&& type1_size <= INT_MAX && type2_size <= INT_MAX
		&& type3_size <= INT_MAX && param1->length <= INT_MAX
		&& param2->length <= INT_MAX && param3->length <= INT_MAX);
	if(report_is_buffer /* static */) {
		const char *const ltgt = style_format() == OUT_HTML
			? HTML_LT HTML_GT : "<>";
		char *a;
		if(!(a = buffer_prepare(3 * strlen(ltgt)
			+ type1_size + param1->length
			+ type2_size + param2->length
			+ type3_size + param3->length))) return 0;
		sprintf(a, format,
			(int)type1_size, type1, (int)param1->length, param1->from,
			(int)type2_size, type2, (int)param2->length, param2->from,
			(int)type3_size, type3, (int)param3->length, param3->from);
	} else {
		style_flush_symbol(gen3->symbol);
		printf(format,
			(int)type1_size, type1, (int)param1->length, param1->from,
			(int)type2_size, type2, (int)param2->length, param2->from,
			(int)type3_size, type3, (int)param3->length, param3->from);
	}
	return 1;
catch:
	fprintf(stderr, "%s: expected A_B_C_(id,id,id).\n", pos(gen3));
	return 0;
}
static int out_escape(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->symbol == ESCAPE && t->length == 2 && !report_is_buffer);
	style_flush_symbol(t->symbol);
	style_encode_length(1, t->from + 1);
	return 1;
}
static int out_url(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->symbol == URL && !report_is_buffer);
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
	return 1;
}
static int out_cite(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	const char *const url_encoded = url_encode(t->from, t->length);
	assert(t && t->symbol == CITE && !report_is_buffer);
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
	return 1;
catch:
	fprintf(stderr, "%s: expected <source>.\n", pos(t));
	return 0;
}
/** All the `see_*` are the same. */
static int see(struct token_array_cursor *const tok,
	const enum division divn) {
	const struct token *const t = token_array_look(tok);
	assert(!report_is_buffer
		&& ((t->symbol == SEE_FN && divn == DIV_FUNCTION)
		|| (t->symbol == SEE_TAG && divn == DIV_TAG)
		|| (t->symbol == SEE_TYPEDEF && divn == DIV_TYPEDEF)
		|| (t->symbol == SEE_DATA && divn == DIV_DATA)));
	style_flush_symbol(t->symbol);
	if(style_format() == OUT_HTML) {
		printf("<a href = \"#%s:", division[divn].keyword);
		style_encode_length(t->length, t->from);
		printf("\">");
		style_encode_length(t->length, t->from);
		printf("</a>");
	} else {
		printf("[");
		style_push(ST_TO_HTML); /* <-- html: this is not escaped by Markdown. */
		style_encode_length(t->length, t->from);
		style_pop(); /* html --> */
		printf("](#%s%s-%x)", md_fragment_extra, division[divn].keyword,
			fnv_32a_str(style_encode_length_raw_to_buffer(t->length, t->from)));
	}
	return 1;
}
static int out_see_fn(struct token_array_cursor *tok)
	{ return see(tok, DIV_FUNCTION); }
static int out_see_tag(struct token_array_cursor *tok)
	{ return see(tok, DIV_TAG); }
static int out_see_typedef(struct token_array_cursor *tok)
	{ return see(tok, DIV_TYPEDEF); }
static int out_see_data(struct token_array_cursor *tok)
	{ return see(tok, DIV_DATA); }
/** Math and code. */
static int out_math_begin(struct token_array_cursor *tok)
	{ return (void)tok, style_push(ST_CODE), 1; }
static int out_math_end(struct token_array_cursor *tok)
	{ return (void)tok, style_expect(ST_CODE), style_pop(), 1; }
static int out_em_begin(struct token_array_cursor *tok)
	{ return (void)tok, style_push(ST_EM), 1; }
static int out_em_end(struct token_array_cursor *tok)
	{ return (void)tok, style_expect(ST_EM), style_pop(), 1; }
static int out_link(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok), *text, *turl;
	const enum format f = style_format();
	const char *fn;
	size_t fn_len;
	FILE *fp;
	assert(t && t->symbol == LINK_START && !report_is_buffer);
	style_flush_symbol(t->symbol);
	/* The expected format is LINK_START [^URL]* URL. */
	/* for(turl = token_array_next(tok); token_array_exists(turl);
		token_array_next(turl)) {
		if(!turl) goto catch;
		if(turl->symbol == URL) break;
	} */
	/* 2024-12-19: What? why would they have random garbage inside the url?
	 lines are too long? What even is this? */
	if(!(token_array_next(tok), token_array_exists(tok))
		|| !(turl = token_array_look(tok), turl->symbol == URL)
		) goto catch;

	/* We want to open this file to check if it's on the up-and-up. */
	if(!(errno = 0, fn = url_from_here(turl->length, turl->from)))
		{ if(errno) goto catch; else goto raw; }
	if(!(fp = fopen(fn, "r")))
		{ fprintf(stderr, "In link. "); perror(fn); errno = 0; goto raw; }
	fclose(fp);
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
	/*for(text = token_array_next(tokens, t); text->symbol != URL; )
		if(!(text = print_token(tokens, text))) goto catch;*/
	(void)text;
	assert(0);
	style_pop(), style_pop();
	if(f == OUT_HTML) printf("</a>");
	else printf("](%.*s)", (int)fn_len, fn);
	return 1;
catch:
	fprintf(stderr, "%s: expected `[description](url)`.\n", pos(t));
	return 0;
}
static int out_image(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok), *text, *turl;
	const enum format f = style_format();
	const char *fn;
	unsigned width = 1, height = 1;
	int success = 0;
	assert(0);
	assert(t && t->symbol == IMAGE_START && !report_is_buffer);
	style_flush_symbol(t->symbol);
	/* The expected format is IMAGE_START [^URL]* URL. */
/*	for(turl = token_array_next(tokens, t); turl->symbol != URL;
		turl = token_array_next(tokens, turl)) if(!turl) goto catch;*/
	printf("%s", f == OUT_HTML ? "<img alt = \"" : "![");
	/* This is html even in Md. */
	style_push(ST_TO_HTML), style_push(ST_PLAIN);
/*	for(text = token_array_next(tokens, t); text->symbol != URL; )
		if(!(text = print_token(tokens, text))) goto catch;*/
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
	/* *ptoken = token_array_next(tokens, turl);*/
	return success;
}
static int out_nbsp(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->symbol == NBSP && !report_is_buffer);
	style_flush_symbol(t->symbol);
	printf("&nbsp;");
	return 1;
}
static int out_nbthinsp(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->symbol == NBTHINSP && !report_is_buffer);
	style_flush_symbol(t->symbol);
	printf("&#8239;" /* "&thinsp;" <- breaking? */);
	return 1;
}
static int out_mathcalo(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->symbol == MATHCALO && !report_is_buffer);
	style_flush_symbol(t->symbol);
	/* Omicron. It looks like a stylised "O"? The actual is "&#120030;" but
	 good luck finding a font that supports that. If one was using JavaScript
	 and had a constant connection, we could use MathJax. */
	printf("&#927;" /* "O" */);
	return 1;
}
static int out_ctheta(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->symbol == CTHETA && !report_is_buffer);
	style_flush_symbol(t->symbol);
	printf("&#920;" /* "&Theta;" This is supported on more browsers. */);
	return 1;
}
static int out_comega(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->symbol == COMEGA && !report_is_buffer);
	style_flush_symbol(t->symbol);
	printf("&#937;" /* "&Omega;" */);
	return 1;
}
static int out_times(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->symbol == TIMES && !report_is_buffer);
	style_flush_symbol(t->symbol);
	printf("&#215;");
	return 1;
}
static int out_cdot(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->symbol == CDOT && !report_is_buffer);
	style_flush_symbol(t->symbol);
	printf("&#183;" /* &middot; */);
	return 1;
}
static int out_log(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->symbol == LOG && !report_is_buffer);
	style_flush_symbol(t->symbol);
	printf("log");
	return 1;
}
static int out_list(struct token_array_cursor *tok) {
	if(style_Is_top(ST_LI)) {
		style_pop_push();
	} else {
		style_pop_strong();
		style_push(ST_UL), style_push(ST_LI);
	}
	return (void)tok, 1;
}
static int out_pre(struct token_array_cursor *tok) {
	const struct token *const t = token_array_look(tok);
	assert(t && t->symbol == PREFORMATTED && !report_is_buffer);
	if(style_Is_top(ST_PRELINE)) style_pop_strong();
	style_push(ST_PRE), style_push(ST_PRELINE);
	style_flush_symbol(t->symbol);
	style_encode_length(t->length, t->from);
	return 1;
}

#include "scanner.h"
#define X(a, b, c, d, e, f) c
static const out_fn symbol_outs[] = { SYMBOL };
#undef X

/** Given `cur`, prints and consumes one [multi-]token sequence.
 @param[fill_buffer] This is only for individual tokens, otherwise null. The
 address of pointer that receives a temporary string that represents the token
 instead of printing.
 @throws[EILSEQ] Sequence error. Must detect with `errno`.
 @return Success. */
int reportprint_token_fill(struct token_array_cursor *const tok,
	const char **fill_buffer) {
	const struct token *const t = token_array_look(tok);
	const out_fn sym_out = symbol_outs[t->symbol];
	assert(tok && token_array_exists(tok) && sym_out);
	if(fill_buffer) report_is_buffer = 1;
	if(!sym_out(tok)) return errno = EILSEQ, 0;
	if(fill_buffer) {
		report_is_buffer = 0;
		*fill_buffer = buffer_get();
	}
	token_array_next(tok);
	return 1;
}
/** Print the next `tok`. */
int reportprint_token(struct token_array_cursor *const tok)
	{ return reportprint_token_fill(tok, 0); }

/** @param[highlights] Must be sorted if not null, creates an emphasis on those
 words.
 @throws[EILSEQ] Sequence error. Must detect with `errno`. */
int reportprint_highlight(const struct token_array *const tokens,
	const struct index_array *const highlights) {
	struct token_array_cursor tok;
	struct index_array_cursor high;
	//const struct token *const first = token_array_next(tokens, 0), *token = first;
	//size_t *highlight = index_array_next(highlights, 0);
	size_t *highlight = 0;
	int has_next_highlight = 0, is_highlight, is_first_highlight = 1;
	assert(tokens);
	tok = token_array_begin(tokens);
	if(!token_array_exists(&tok)) return;
	/*if(!token) return;*/
	style_push(ST_PLAIN);
	high = index_array_begin(highlights);
	do {
		if(!has_next_highlight) {
			if(token_array_exists((const struct token_array_cursor */*???*/)&high)) highlight = index_array_look(&high);
			else highlight = 0;
			has_next_highlight = 1;
		}
		if(highlight && *highlight <= tok.i) {
			is_highlight = 1;
			index_array_next(&high);
			has_next_highlight = 0;
		} else {
			is_highlight = 0;
		}
		if(is_highlight) style_highlight_on(is_first_highlight
			? ST_STRONG_HTML : ST_EM_HTML);
		print_token(&tok);
		if(is_highlight) style_highlight_off(), is_first_highlight = 0;
	} while(token_array_exists(&tok));
	/*while(token) {
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
	}*/
	style_pop();
}

void reportprint_tokens(const struct token_array *const tokens)
	{ highlight_tokens(tokens, 0); }
