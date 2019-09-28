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
	case IN_PRE: printf("</pre>"/*, ostate.sep*/); break;
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
	case IN_PARA: printf("%s%s<ul>%s<li>", ostate.end, ostate.sep, ostate.sep);
		break;
	case IN_LIST: printf("</li>%s<li>", ostate.sep); return;
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
	case IN_PARA: if(ostate.is_lazy_space) { fputc(' ', stdout);
			ostate.is_lazy_space = 0; } break;
	default: break;
	}
}
static void state_reset(const char *const start,
	const char *const end, const char *const sep) {
	assert(start && end && sep);
	state_to_default();
	ostate.level = 0;
	ostate.is_sep_before = 0;
	ostate.is_sep_forced = 0;
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
static const struct Token *token_print(const struct TokenArray *const tokens,
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
	printf("<a href = \"#fn:%.*s\">%.*s</a>",
		fn->length, fn->from, fn->length, fn->from);
	*ptoken = TokenArrayNext(tokens, fn);
	return 1;
}
OUT(see_tag) {
	const struct Token *const tag = *ptoken;
	assert(tokens && tag && tag->symbol == SEE_FN);
	state_from_default();
	printf("<a href = \"#tag:%.*s\">%.*s</a>",
		tag->length, tag->from, tag->length, tag->from);
	*ptoken = TokenArrayNext(tokens, tag);
	return 1;
}
OUT(see_typedef) {
	const struct Token *const def = *ptoken;
	assert(tokens && def && def->symbol == SEE_FN);
	state_from_default();
	printf("<a href = \"#typedef:%.*s\">%.*s</a>",
		def->length, def->from, def->length, def->from);
	*ptoken = TokenArrayNext(tokens, def);
	return 1;
}
OUT(see_data) {
	const struct Token *const data = *ptoken;
	assert(tokens && data && data->symbol == SEE_FN);
	state_from_default();
	printf("<a href = \"#data:%.*s\">%.*s</a>",
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
		if(!(text = token_print(tokens, text))) goto catch;
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
static const struct Token *token_print(const struct TokenArray *const tokens,
	const struct Token *token) {
	const OutFn sym_out = symbol_outs[token->symbol];
	assert(tokens && token);
	if(!sym_out) return printf("[undefined token: %s]", symbols[token->symbol]),
		TokenArrayNext(tokens, token);
	{
		const struct TokenArray *ts = tokens;
		const struct Token *t = 0;
		while((t = TokenArrayNext(ts, t)) && t != token);
		assert(t);
	}
	if(!sym_out(tokens, &token)) { errno = EILSEQ; return 0; }
	return token;
}

/** @param[highlights] Must be sorted if not null, creates an emphasis on those
 words.
 @throws[EILSEQ] Sequence error. Must detect with `errno`. */
static void tokens_print(const struct TokenArray *const tokens,
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
		token = token_print(tokens, token);
		if(is_highlight) printf("</em>");
	}
}

void ReportDebug(void) {
	struct Segment *segment = 0;
	struct Attribute *att = 0;
	while((segment = SegmentArrayNext(&report, segment))) {
		printf("Segment %s: %s;\n"
			"code: %s;\n"
			"params: %s;\n"
			"doc: %s.\n",
			segment->name,
			divisions[segment->division],
			TokenArrayToString(&segment->code),
			SizeArrayToString(&segment->code_params),
			TokenArrayToString(&segment->doc));
		while((att = AttributeArrayNext(&segment->attributes, att)))
			printf("%s{%s} %s.\n", symbols[att->token.symbol],
			TokenArrayToString(&att->header),
			TokenArrayToString(&att->contents));
		fputc('\n', stdout);
	}
}

/** Searches if some segment's division is `division`.
 @order O(`segments`) */
static int division_exists(const enum Division division) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment)))
		if(segment->division == division) return 1;
	return 0;
}

static void division_act(const enum Division division,
	void (*act)(const struct Segment *const segment)) {
	const struct Segment *segment = 0;
	assert(act);
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != division) continue;
		act(segment);
	}
}

/** Seaches if attribute `symbol` exists within `segment`.
 @order O(`attributes`) */
static int attribute_content_exists(const struct Segment *const segment,
	const enum Symbol symbol) {
	struct Attribute *attribute = 0;
	assert(segment);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
		if(attribute->token.symbol == symbol
		&& TokenArraySize(&attribute->contents)) return 1;
	return 0;
}

/** Seaches all `tokens` for `token` and returns it. */
static const struct Token *search_token(const struct TokenArray *const tokens,
	const struct Token *const token) {
	const struct Token *t = 0;
	while((t = TokenArrayNext(tokens, t)))
		if(!token_compare(token, t)) return t;
	return 0;
}

/** Seaches if attribute `symbol` exists within `segment` whose header contains
 `header`.
 @order O(`attributes`) */
static int attribute_header_exists(const struct Segment *const segment,
	const enum Symbol symbol, const struct Token *const header) {
	struct Attribute *attribute = 0;
	assert(segment);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute))) {
		if(attribute->token.symbol != symbol) continue;
		if(search_token(&attribute->header, header)) return 1;
	}
	return 0;
}

/** Seaches if attribute `symbol` exists within all preamble segments.
 @order O(`segments` * `attributes`) */
static int preamble_attribute_exists(const enum Symbol symbol) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != DIV_PREAMBLE) continue;
		if(attribute_content_exists(segment, symbol)) return 1;
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
		/*tokens_print(&attribute->header, 0);*/
		tokens_print(&attribute->contents, 0);
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
			|| !search_token(&attribute->header, header)) continue;
		printf("[");
		tokens_print(&attribute->header, 0);
		printf("]");
		tokens_print(&attribute->contents, 0);
		state_to_default();
	}
}

/** For each preamble segment, print all attributes that match `symbol`.
 @order O(`segments` * `attributes`) */
static void preamble_attribute_print(const enum Symbol symbol) {
	struct Segment *segment = 0;
	state_reset("", "", ", ");
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != DIV_PREAMBLE) continue;
		attribute_print(segment, symbol);
		state_to_default();
	}
}

static void print_attribute_maybe(const struct Segment *const segment,
	const enum Symbol symbol) {
	assert(segment && symbol);
	if(!attribute_content_exists(segment, symbol)) return;
	printf("\n\n");
	state_reset("{", "}", ", ");
	attribute_print(segment, symbol);
	state_to_default();
}

static void print_attribute_header_maybe(const struct Segment *const segment,
	const enum Symbol symbol, const struct Token *header) {
	assert(segment && symbol);
	if(!attribute_header_exists(segment, symbol, header)) return;
	printf("\n\n");
	state_reset("{", "}", ", ");
	attribute_header_print(segment, symbol, header);
	state_to_default();
}

/** Prints only the code of a `segment`.
 @implements division_act */
static void print_code(const struct Segment *const segment) {
	state_reset("{", "}", ", ");
	printf("<print code>");
	tokens_print(&segment->code, &segment->code_params);
	state_to_default();
	printf("\n\n");
}

/** Prints all a `segment`.
 @implements division_act */
static void print_all(const struct Segment *const segment) {
	const struct Token *param;
	size_t no;
	/* This is the title, the rest are params. */
	printf("<%s %s><h2>", divisions[segment->division], segment->name);
	/* The title is generally the first param. */
	if((param = param_no(segment, 0))) {
		token_print(&segment->code, param);
	} else {
		printf("no params");
	}
	printf("</h2>\n");
	print_code(segment);
	state_reset("<p>", "</p>", "\n\n");
	tokens_print(&segment->doc, 0);
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
	print_attribute_maybe(segment, ATT_ALLOW); /* fixme */
	printf("\n</%s>\n\n", segment->name);
}

/** Prints preable segment's doc. */
static void preamble_print_all_content(void) {
	struct Segment *segment = 0;
	state_reset("<p>", "</p>", "\n\n");
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != DIV_PREAMBLE) continue;
		tokens_print(&segment->doc, 0);
		state_to_default();
	}
	printf("\n\nAlso print attributes.\n\n");
}

/** Outputs a report.
 @throws[EILSEQ] Sequence error.
 @return Success. */
int ReportOut(void) {
	/* We set `errno` here so that we don't have to test it each time. */
	errno = 0;
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
	if(preamble_attribute_exists(ATT_TITLE)) {
		preamble_attribute_print(ATT_TITLE);
	} else {
		printf("Untitled");
	}
	printf("</title>\n"
		"</head>\n\n\n"
		"<body>\n\n");
	if(preamble_attribute_exists(ATT_TITLE)) {
		printf("<h1>");
		preamble_attribute_print(ATT_TITLE);
		printf("</h1>\n\n");
	}
	if(preamble_attribute_exists(ATT_AUTHOR)) {
		printf("<h2>");
		preamble_attribute_print(ATT_AUTHOR);
		printf("</h2>\n\n");
	}
	printf("<ul>\n"
		"\t<li><a href = \"#_preamble\">preamble</li>\n");
	if(division_exists(DIV_TYPEDEF))
		printf("\t<li><a href = \"#_typedefs\">typedef definitions</a></li>\n");
	if(division_exists(DIV_TAG))
		printf("\t<li><a href = \"#_tags\">struct, union, and enum definitions"
		"</a></li>\n");
	if(division_exists(DIV_DATA))
		printf("\t<li><a href = \"#_data\">data declarations</a></li>\n");
	if(division_exists(DIV_FUNCTION))
		printf("\t<li><a href = \"#_summary\">function summary</a></li>\n"
		"\t<li><a href = \"#_detail\">function details</a></li>\n");
	printf("</ul>\n\n<a name = \"_preamble\"><!-- --></a>\n\n");
	/* Preamble contents. */
	preamble_print_all_content();
	printf("\n\n");
	/* Print typedefs. */
	if(division_exists(DIV_TYPEDEF)) {
		printf("<a name = \"_typedefs\"><!-- --></a>"
			"<h2>Typedef Definitions</h2>\n\n");
		division_act(DIV_TYPEDEF, &print_all);
	}
	/* Print tags. */
	if(division_exists(DIV_TAG)) {
		printf("<a name = \"_tags\"><!-- --></a>"
			"<h2>Struct, Union, and Enum Definitions</h2>\n\n");
		division_act(DIV_TAG, &print_all);
	}
	/* Print general declarations. */
	if(division_exists(DIV_DATA)) {
		printf("<a name = \"_data\"><!-- --></a>"
			"<h2>Data Delcarations</h2>\n\n");
		division_act(DIV_DATA, &print_all);
	}
	/* Print functions. */
	if(division_exists(DIV_FUNCTION)) {
		printf("<a name = \"_summary\"><!-- --></a>"
			   "<h2>Function Summary</h2>\n\n");
		division_act(DIV_FUNCTION, &print_code);
		printf("<a name = \"_detail\"><!-- --></a>"
			   "<h2>Function Detail</h2>\n\n");
		division_act(DIV_FUNCTION, &print_all);
	}
	printf("\n"
		"</body>\n"
		"</html>\n");
	return errno ? 0 : 1;
}
