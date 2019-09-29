#include "UrlEncode.h"

/* `SYMBOL` is declared in `Scanner.h`. */
static const int symbol_lspaces[] = { SYMBOL(PARAM5D) };
static const int symbol_rspaces[] = { SYMBOL(PARAM5E) };

static struct {
	int level;
	enum { IN_DEFAUT, IN_PARA, IN_LIST, IN_PRE } in;
	int is_sep_before, is_sep_forced, is_lazy_space;
	const char *start, *end, *sep;
} ostate = { 0, IN_DEFAUT, 0, 0, 0, "{", "}", ", " };
static void state_to_default(void) {
	switch(ostate.in) {
	case IN_DEFAUT: return;
	case IN_PARA: printf("%s", ostate.end); break;
	case IN_LIST: printf("</li>%s</ul>", ostate.sep); break;
	case IN_PRE: printf("</pre>"); break;
	}
	ostate.in = IN_DEFAUT;
	ostate.is_sep_forced = 1;
}
static void state_to_para(void) {
	switch(ostate.in) {
	case IN_DEFAUT: printf("%s", ostate.start); break;
	case IN_PARA: return;
	case IN_LIST: printf("</li>%s</ul>%s%s", ostate.sep, ostate.sep,
		ostate.start); break;
	case IN_PRE: printf("</pre>%s%s", ostate.sep, ostate.start); break;
	}
	ostate.in = IN_PARA;
}
static void state_to_list(void) {
	switch(ostate.in) {
	case IN_DEFAUT: printf("<ul>%s<li>", ostate.sep); break;
	case IN_PARA:
		printf("%s%s<ul>%s<li>", ostate.end, ostate.sep, ostate.sep);
		break;
	case IN_LIST:
		printf("</li>%s<li>", ostate.sep);
		ostate.is_lazy_space = 0; /* Don't want lazy spaces between <li>. */
		return;
	case IN_PRE: printf("</pre>%s<ul>%s<li>", ostate.sep, ostate.sep);
	}
	ostate.in = IN_LIST;
}
static void state_to_pre(void) {
	switch(ostate.in) {
	case IN_DEFAUT: printf("<pre>"); break;
	case IN_PARA: printf("%s%s<pre>", ostate.end, ostate.sep); break;
	case IN_LIST: printf("</li>%s</ul>%s<pre>", ostate.sep, ostate.sep); break;
	case IN_PRE: printf("\n"); return;
	}
	ostate.in = IN_PRE;
}
static void state_from_default(void) {
	switch(ostate.in) {
	case IN_DEFAUT: state_to_para(); break;
	case IN_PARA:
	case IN_LIST:
		if(ostate.is_lazy_space) { fputc(' ', stdout);
			ostate.is_lazy_space = 0; } break;
	default: break;
	}
}
static void reset_state(const char *const start,
	const char *const end, const char *const sep) {
	assert(start && end && sep);
	state_to_default();
	ostate.level = 0;
	ostate.is_sep_before = 0;
	ostate.is_sep_forced = 0;
	ostate.is_lazy_space = 0;
	ostate.start = start;
	ostate.end   = end;
	ostate.sep   = sep;
}
static void state_sep_if_needed(enum Symbol symbol) {
	if(ostate.is_sep_forced
		|| (ostate.is_sep_before && symbol_lspaces[symbol]))
		printf("%s", ostate.sep);
	ostate.is_sep_forced = 0;
	ostate.is_sep_before = symbol_rspaces[symbol];
}

static void encode(int length, const char *from) {
	assert(length >= 0 && from);
	while(length) {
		switch(*from) {
			case '<': fputs("&lt;", stdout); break;
			case '>': fputs("&gt;", stdout); break; 
			case '&': fputs("&amp;", stdout); break;
			case '\0': fprintf(stderr, "Encoded null.\n"); return;
			default: fputc(*from, stdout); break;
		}
		from++, length--;
	}
}

/* Some `OutFn` need this. */
static const struct Token *print_token(const struct TokenArray *const tokens,
	const struct Token *token);

/** Selects `token` out of `tokens` and prints it and returns the next token. */
typedef int (*OutFn)(const struct TokenArray *const tokens,
	const struct Token **const ptoken);
/* @param[ptoken] Is an [in/out] variable, it should get updated unless the
 return value is false.
 @return Success.
 @implements <Attribute>Predicate */
#define OUT(name) static int name(const struct TokenArray *const tokens, \
	const struct Token **ptoken)

OUT(ws) {
	const struct Token *const space = *ptoken;
	assert(tokens && space && space->symbol == SPACE);
	ostate.is_lazy_space = 1;
	*ptoken = TokenArrayNext(tokens, space);
	return 1;
}
OUT(par) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NEWLINE);
	state_to_default();
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(lit) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->length > 0 && t->from);
	state_from_default();
	/*printf("`%.*s'", t->length, t->from);*/
	encode(t->length, t->from);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(gen1) {
	const struct Token *const t = *ptoken,
		*const lparen = TokenArrayNext(tokens, t),
		*const param = TokenArrayNext(tokens, lparen),
		*const rparen = TokenArrayNext(tokens, param);
	const char *a, *type;
	int type_size;
	assert(tokens && t && t->symbol == ID_ONE_GENERIC);
	if(!lparen || lparen->symbol != LPAREN || !param || !rparen
	   || rparen->symbol != RPAREN) goto catch;
	type = t->from;
	if(!(a = strchr(type, '_'))) goto catch;
	type_size = (int)(a - type);
	assert(t->length == a + 1 - t->from);
	state_from_default();
	printf("&lt;%.*s&gt;%.*s",
		t->length - 1, t->from, param->length, param->from);
	*ptoken = TokenArrayNext(tokens, rparen);
	return 1;
catch:
	fprintf(stderr, "%s: expected generic(id) %s.\n", pos(t), TokenArrayToString(tokens));
	return 0;
}
OUT(gen2) {
	const struct Token *const t = *ptoken,
		*const lparen = TokenArrayNext(tokens, t),
		*const param1 = TokenArrayNext(tokens, lparen),
		*const comma = TokenArrayNext(tokens, param1),
		*const param2 = TokenArrayNext(tokens, comma),
		*const rparen = TokenArrayNext(tokens, param2);
	const char *a, *type1, *type2;
	int type1_size, type2_size;
	assert(tokens && t && t->symbol == ID_TWO_GENERICS);
	if(!lparen || lparen->symbol != LPAREN || !param1 || !comma
		|| comma->symbol != COMMA || !param2 || !rparen
		|| rparen->symbol != RPAREN) goto catch;
	type1 = t->from;
	if(!(a = strchr(type1, '_'))) goto catch;
	type1_size = (int)(a - type1);
	type2 = a + 1;
	if(!(a = strchr(type2, '_'))) goto catch;
	type2_size = (int)(a - type2);
	assert(t->length == a + 1 - t->from);
	state_from_default();
	printf("&lt;%.*s&gt;%.*s&lt;%.*s&gt;%.*s", type1_size, type1,
		param1->length, param1->from, type2_size, type2, param2->length,
		param2->from);
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
	const char *a, *type1, *type2, *type3;
	int type1_size, type2_size, type3_size;
	assert(tokens && t && t->symbol == ID_THREE_GENERICS);
	if(!lparen || lparen->symbol != LPAREN || !param1 || !comma1
	   || comma1->symbol != COMMA || !param2 || !comma2 ||
	   comma2->symbol != COMMA || !param3 || !rparen
	   || rparen->symbol != RPAREN) goto catch;
	type1 = t->from;
	if(!(a = strchr(type1, '_'))) goto catch;
	type1_size = (int)(a - type1);
	type2 = a + 1;
	if(!(a = strchr(type2, '_'))) goto catch;
	type2_size = (int)(a - type2);
	type3 = a + 1;
	if(!(a = strchr(type3, '_'))) goto catch;
	type3_size = (int)(a - type3);
	assert(t->length == a + 1 - t->from);
	state_from_default();
	printf("&lt;%.*s&gt;%.*s&lt;%.*s&gt;%.*s&lt;%.*s&gt;%.*s",
		type1_size, type1, param1->length, param1->from, type2_size, type2,
		param2->length, param2->from, type3_size, type3,
		param3->length, param3->from);
	*ptoken = TokenArrayNext(tokens, rparen);
	return 1;
catch:
	fprintf(stderr, "%s: expected A_B_C_(id,id,id).\n", pos(t));
	return 0;
}
OUT(escape) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == ESCAPE && t->length == 2);
	state_from_default();
	encode(1, t->from + 1);
	/*printf("%c", t->from[1]);*/
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(url) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == URL);
	state_from_default();
	printf("<a href = \"%.*s\">", t->length, t->from);
	encode(t->length, t->from);
	printf("</a>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(cite) {
	const struct Token *const t = *ptoken;
	const char *const url_encoded = UrlEncode(t->from, t->length);
	assert(tokens && t && t->symbol == CITE);
	if(!url_encoded) goto catch;
	state_from_default();
	printf("<a href = \"https://scholar.google.ca/scholar?q=%s\">",
		url_encoded);
	encode(t->length, t->from);
	printf("</a>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
catch:
	fprintf(stderr, "%s: expected <short source>.\n", pos(t));
	return 0;
}
OUT(see_fn) {
	const struct Token *const fn = *ptoken;
	assert(tokens && fn && fn->symbol == SEE_FN);
	state_from_default();
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_FUNCTION],
		fn->length, fn->from, fn->length, fn->from);
	*ptoken = TokenArrayNext(tokens, fn);
	return 1;
}
OUT(see_tag) {
	const struct Token *const tag = *ptoken;
	assert(tokens && tag && tag->symbol == SEE_TAG);
	state_from_default();
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_TAG],
		tag->length, tag->from, tag->length, tag->from);
	*ptoken = TokenArrayNext(tokens, tag);
	return 1;
}
OUT(see_typedef) {
	const struct Token *const def = *ptoken;
	assert(tokens && def && def->symbol == SEE_TYPEDEF);
	state_from_default();
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_TYPEDEF],
		def->length, def->from, def->length, def->from);
	*ptoken = TokenArrayNext(tokens, def);
	return 1;
}
OUT(see_data) {
	const struct Token *const data = *ptoken;
	assert(tokens && data && data->symbol == SEE_DATA);
	state_from_default();
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_DATA],
		data->length, data->from, data->length, data->from);
	*ptoken = TokenArrayNext(tokens, data);
	return 1;
}
OUT(math_begin) { /* Math and code. */
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_BEGIN);
	state_from_default();
	printf("<code>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(math_end) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_END);
	state_from_default();
	printf("</code>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_begin) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_BEGIN);
	state_from_default();
	printf("<em>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_end) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_END);
	state_from_default();
	printf("</em>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(link) {
	const struct Token *const t = *ptoken, *text, *turl;
	assert(tokens && t && t->symbol == LINK_START);
	state_from_default();
	for(turl = TokenArrayNext(tokens, t);;turl = TokenArrayNext(tokens, turl)) {
		if(!turl) goto catch;
		if(turl->symbol == URL) break;
	}
	printf("<a href = \"%.*s\">", turl->length, turl->from);
	for(text = TokenArrayNext(tokens, t); text->symbol != URL; )
		if(!(text = print_token(tokens, text))) goto catch;
	printf("</a>");
	*ptoken = TokenArrayNext(tokens, turl);
	return 1;
catch:
	fprintf(stderr, "%s: expected `[description](url)`.\n", pos(t));
	return 0;
}
OUT(nbsp) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NBSP);
	state_from_default();
	printf("&nbsp;");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(nbthinsp) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NBTHINSP);
	state_from_default();
	printf("&#8239;"/*"&thinsp;"<-I think it's breaking.*/);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(list) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == LIST_ITEM);
	state_to_list();
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(pre) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == PREFORMATTED);
	state_to_pre();
	encode(t->length, t->from);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}



/* `SYMBOL` is declared in `Scanner.h`. */
static const OutFn symbol_outs[] = { SYMBOL(PARAM5C) };



/** Prints one [multi-]token sequence.
 @throws[EILSEQ] Sequence error. Must detect with `errno`.
 @return The next token. */
static const struct Token *print_token(const struct TokenArray *const tokens,
	const struct Token *token) {
	const OutFn sym_out = symbol_outs[token->symbol];
	assert(tokens && token);
	if(!sym_out) return fprintf(stderr, "%s: symbol output undefined.\n",
		pos(token)), TokenArrayNext(tokens, token);
	if(!sym_out(tokens, &token)) { errno = EILSEQ; return 0; }
	return token;
}

/** @param[highlights] Must be sorted if not null, creates an emphasis on those
 words.
 @throws[EILSEQ] Sequence error. Must detect with `errno`. */
static void highlight_tokens(const struct TokenArray *const tokens,
	const struct SizeArray *const highlights) {
	const struct Token *const first = TokenArrayNext(tokens, 0), *token = first;
	size_t *highlight = SizeArrayNext(highlights, 0);
	int is_highlight;
	assert(tokens);
	if(!token) return;
	while(token) {
		if(highlight && *highlight == (size_t)(token - first)) {
			is_highlight = 1;
			highlight = SizeArrayNext(highlights, highlight);
		} else {
			is_highlight = 0;
		}
		state_sep_if_needed(token->symbol);
		if(is_highlight) printf("<em>");
		token = print_token(tokens, token);
		if(is_highlight) printf("</em>");
	}
}

static void print_tokens(const struct TokenArray *const tokens) {
	highlight_tokens(tokens, 0);
}

void ReportDebug(void) {
	struct Segment *segment = 0;
	struct Attribute *att = 0;
	fprintf(stderr, "Report debug:\n");
	while((segment = SegmentArrayNext(&report, segment))) {
		fprintf(stderr, "Segment division %s:\n"
			"code: %s;\n"
			"params: %s;\n"
			"doc: %s.\n",
			divisions[segment->division],
			TokenArrayToString(&segment->code),
			SizeArrayToString(&segment->code_params),
			TokenArrayToString(&segment->doc));
		while((att = AttributeArrayNext(&segment->attributes, att)))
			fprintf(stderr, "%s{%s} %s.\n", symbols[att->token.symbol],
			TokenArrayToString(&att->header),
			TokenArrayToString(&att->contents));
		fputc('\n', stderr);
	}
}

/* Lambdas would be nice! */

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

/** `act` on all contents os `attribute_symbol` with content within
 `segment`. */
static int segment_attribute_act(const struct Segment *const segment,
	const enum Symbol attribute_symbol,
	void (*act)(const struct TokenArray *const)) {
	struct Attribute *attribute = 0;
	assert(segment);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute))) {
		if(attribute->token.symbol != attribute_symbol
		   || !TokenArraySize(&attribute->contents)) continue;
		act(&attribute->contents);
	}
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

/** @return Is `attribute_symbol` under `division` in the report? */
static int division_attribute_exists(const enum Division division,
	const enum Symbol attribute_symbol) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != division) continue;
		if(segment_attribute_exists(segment, attribute_symbol)) return 1;
	}
	return 0;
}

/** `act` on all `attribute_symbol` under `division`. */
static void division_attribute_act(const enum Division division,
	const enum Symbol attribute_symbol,
	void (*act)(const struct TokenArray *const)) {
	const struct Segment *segment = 0;
	assert(act);
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != division) continue;
		segment_attribute_act(segment, attribute_symbol, act);
	}
}

/** @return The first token in `tokens` that matches `token`. */
static const struct Token *any_token(const struct TokenArray *const tokens,
	const struct Token *const token) {
	const struct Token *t = 0;
	while((t = TokenArrayNext(tokens, t)))
		if(!token_compare(token, t)) return t;
	return 0;
}

/** Seaches if attribute `symbol` exists within `segment` whose header contains
 `header`.
 @order O(`attributes`) */
static int segment_attribute_header_exists(const struct Segment *const segment,
	const enum Symbol symbol, const struct Token *const header) {
	struct Attribute *attribute = 0;
	assert(segment);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute))) {
		if(attribute->token.symbol != symbol) continue;
		if(any_token(&attribute->header, header)) return 1;
	}
	return 0;
}

/** For `segment`, print all attributes that match `symbol`.
 @order O(`attributes`) */
static void attribute_print(const struct Segment *const segment,
	const enum Symbol symbol) {
	struct Attribute *attribute = 0;
	assert(segment);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute))) {
		if(attribute->token.symbol != symbol) continue;
		print_tokens(&attribute->contents);
		state_to_default();
	}
}
/** For `segment`, print all attributes that match `symbol` whose header
 contains `header`.
 @order O(`attributes`) */
static void attribute_header_print(const struct Segment *const segment,
	const enum Symbol symbol, const struct Token *const header) {
	struct Attribute *attribute = 0;
	char a[12];
	assert(segment && header);
	token_to_string(header, &a);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute))) {
		if(attribute->token.symbol != symbol
			|| !any_token(&attribute->header, header)) continue;
		printf("[");
		print_tokens(&attribute->header);
		printf("]");
		print_tokens(&attribute->contents);
		state_to_default();
	}
}

/** For each `division` segment, print all attributes that match `symbol`.
 @order O(`segments` * `attributes`) */
static void division_attribute_print(const enum Division division,
	const enum Symbol symbol) {
	struct Segment *segment = 0;
	reset_state("", "", ", ");
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != division) continue;
		attribute_print(segment, symbol);
		state_to_default();
	}
}

static void print_attribute_maybe(const struct Segment *const segment,
	const enum Symbol symbol) {
	assert(segment && symbol);
	if(!segment_attribute_exists(segment, symbol)) return;
	printf("\n\n");
	reset_state("{", "}", ", ");
	attribute_print(segment, symbol);
	state_to_default();
}

static void print_attribute_header_maybe(const struct Segment *const segment,
	const enum Symbol symbol, const struct Token *header) {
	assert(segment && symbol);
	if(!segment_attribute_header_exists(segment, symbol, header)) return;
	printf("\n\n");
	reset_state("{", "}", ", ");
	attribute_header_print(segment, symbol, header);
	state_to_default();
}

/** Prints only the code of a `segment`.
 @implements division_act */
static void print_code(const struct Segment *const segment) {
	reset_state("", "", "&nbsp;");
	printf("<!-- code --><pre>");
	highlight_tokens(&segment->code, &segment->code_params);
	state_to_default();
	printf("</pre>\n\n");
}

/** Prints all a `segment`.
 @implements division_act */
static void segment_print_all(const struct Segment *const segment) {
	const struct Token *param;
	size_t no;
	/* The title is generally the first param. Only sigle-words. */
	if((param = param_no(segment, 0))) {
		reset_state("", "", "???");
		printf("<a name = \"%s:", division_strings[segment->division]);
		print_token(&segment->code, param);
		printf("\"><!-- --></a>\n");
		reset_state("<h3>", "</h3>", "???");
		print_token(&segment->code, param);
		state_to_default();
		printf("\n\n");
	}
	print_code(segment);
	reset_state("<p>", "</p>", "\n\n");
	print_tokens(&segment->doc);
	state_to_default();
	for(no = 1; (param = param_no(segment, no)); no++)
		print_attribute_header_maybe(segment, ATT_PARAM, param);
	print_attribute_maybe(segment, ATT_RETURN);
	print_attribute_maybe(segment, ATT_IMPLEMENTS);
	print_attribute_maybe(segment, ATT_THROWS);
	print_attribute_maybe(segment, ATT_ORDER);
	print_attribute_maybe(segment, ATT_AUTHOR);
	print_attribute_maybe(segment, ATT_STD);
	print_attribute_maybe(segment, ATT_DEPEND);
	print_attribute_maybe(segment, ATT_FIXME);
	print_attribute_maybe(segment, ATT_LICENSE);
	print_attribute_maybe(segment, ATT_ALLOW); /* fixme */
}

/** Prints preable segment's doc. */
static void preamble_print_all_content(void) {
	struct Segment *segment = 0;
	reset_state("<p>", "</p>", "\n\n");
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != DIV_PREAMBLE) continue;
		print_tokens(&segment->doc);
		state_to_default();
	}
	printf("\n\nAlso print attributes.\n\n");
	/* fixme: also print fn attributes for @since @std @depend, _etc_. */
}

/** Outputs a report.
 @throws[EILSEQ] Sequence error.
 @return Success. */
int ReportOut(void) {
	/* We set `errno` here so that we don't have to test output each time. */
	errno = 0;
	/* fixme: how to set utf-8? */
	printf("<!doctype html public \"-//W3C//DTD HTML 4.01//EN\" "
		"\"http://www.w3.org/TR/html4/strict.dtd\">\n\n"
		"<html>\n\n"
		"<head>\n"
		"<!-- Steal these colour values from JavaDocs. -->\n"
		"<style type = \"text/css\">\n"
		"\ta:link,  a:visited { color: #4a6782; }\n"
		"\ta:hover, a:focus   { color: #bb7a2a; }\n"
		"\ta:active           { color: #4A6782; }\n"
		"\ttr:nth-child(even) { background: #dee3e9; }\n"
		"\tdiv {\n"
		"\t\tmargin:  4px 0;\n"
		"\t\tpadding: 0 4px 4px 4px;\n"
		"\t}\n"
		"\ttable      { width: 100%%; }\n"
		"\ttd         { padding: 4px; }\n");
	printf("\th3, h1 {\n"
		"\t\tcolor: #2c4557;\n"
		"\t\tbackground-color: #dee3e9;\n"
		"\t\tpadding:          4px;\n"
		"\t}\n"
		"\th3 {\n"
		"\t\tmargin:           0 -4px;\n"
		"\t\tpadding:          4px;\n"
		"\t}\n"
		"</style>\n"
		"<title>");
	if(division_attribute_exists(DIV_PREAMBLE, ATT_TITLE)) {
		division_attribute_print(DIV_PREAMBLE, ATT_TITLE);
	} else {
		printf("Untitled");
	}
	printf("</title>\n"
		"</head>\n\n\n"
		"<body>\n\n");
	if(division_attribute_exists(DIV_PREAMBLE, ATT_TITLE)) {
		printf("<h1>");
		division_attribute_print(DIV_PREAMBLE, ATT_TITLE);
		printf("</h1>\n\n");
	}
	if(division_attribute_exists(DIV_PREAMBLE, ATT_AUTHOR)) {
		printf("<p>");
		division_attribute_print(DIV_PREAMBLE, ATT_AUTHOR);
		printf("</p>\n\n");
	}

	printf("<ul>\n");
	if(division_attribute_exists(DIV_PREAMBLE, ATT_LICENSE))
		printf("\t<li><a href = \"#license:\">License</a></li>\n");
	printf("\t<li><a href = \"#%s:\">Preamble</li>\n",
		division_strings[DIV_PREAMBLE]);
	if(division_exists(DIV_TYPEDEF))
		printf("\t<li><a href = \"#%s:\">Typedef Aliases</a></li>\n",
		division_strings[DIV_TYPEDEF]);
	if(division_exists(DIV_TAG))
		printf("\t<li><a href = \"#%s:\">Struct, Union, and Enum Definitions"
		"</a></li>\n", division_strings[DIV_TAG]);
	if(division_exists(DIV_DATA))
		printf("\t<li><a href = \"#%s:\">Data Declarations</a></li>\n",
		division_strings[DIV_DATA]);
	if(division_exists(DIV_FUNCTION))
		printf("\t<li><a href = \"#summary:\">Function Summary</a></li>\n"
		"\t<li><a href = \"#%s:\">Function Details</a></li>\n",
		division_strings[DIV_FUNCTION]);
	printf("</ul>\n\n");

	/* License. */
	if(division_attribute_exists(DIV_PREAMBLE, ATT_LICENSE)) {
		printf("<a name = \"license:\"><!-- --></a>\n"
			"<h2>License</h2>\n\n");
		reset_state("<p>", "</p>", "\n\n");
		division_attribute_act(DIV_PREAMBLE, ATT_LICENSE, &print_tokens);
		state_to_default();
		printf("\n\n");
	}

	/* Preamble contents. */
	printf("<a name = \"%s:\"><!-- --></a>\n"
		"<h2>Preamble</h2>\n\n",
		division_strings[DIV_PREAMBLE]);
	preamble_print_all_content();
	printf("\n\n");
	/* Print typedefs. */
	if(division_exists(DIV_TYPEDEF)) {
		printf("<a name = \"%s:\"><!-- --></a>\n"
			"<h2>Typedef Aliases</h2>\n\n", division_strings[DIV_TYPEDEF]);
		division_act(DIV_TYPEDEF, &segment_print_all);
	}
	/* Print tags. */
	if(division_exists(DIV_TAG)) {
		printf("<a name = \"%s:\"><!-- --></a>\n"
			"<h2>Struct, Union, and Enum Definitions</h2>\n\n",
			division_strings[DIV_TAG]);
		division_act(DIV_TAG, &segment_print_all);
	}
	/* Print general declarations. */
	if(division_exists(DIV_DATA)) {
		printf("<a name = \"%s:\"><!-- --></a>\n"
			"<h2>Data Delcarations</h2>\n\n", division_strings[DIV_DATA]);
		division_act(DIV_DATA, &segment_print_all);
	}
	/* Print functions. */
	if(division_exists(DIV_FUNCTION)) {
		printf("<a name = \"summary:\"><!-- --></a>\n"
			"<h2>Function Summary</h2>\n\n");
		division_act(DIV_FUNCTION, &print_code);
		printf("<a name = \"%s:\"><!-- --></a>\n"
			"<h2>Function Details</h2>\n\n", division_strings[DIV_FUNCTION]);
		division_act(DIV_FUNCTION, &segment_print_all);
	}
	printf("\n"
		"</body>\n"
		"</html>\n");
	return errno ? 0 : 1;
}
