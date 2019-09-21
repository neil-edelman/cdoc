#include "UrlEncode.h"

/* `SYMBOL` is declared in `Scanner.h`. */
static const int symbol_lspaces[] = { SYMBOL(PARAM5D) };
static const int symbol_rspaces[] = { SYMBOL(PARAM5E) };

static struct {
	int level;
	enum { IN_DEFAUT, IN_PARA, IN_LIST, IN_PRE } in;
	int is_sep_before, is_sep_forced;
	const char *start, *end, *sep;
	/* Should have `list_start`, but it's not clear what lists in the
	 attributes/titles should look like, so they are constant. */
} ostate = { 0, IN_DEFAUT, 0, 0, "$(", ")$", ";" };
static void state_to_default(void) {
	switch(ostate.in) {
	case IN_DEFAUT: return;
	case IN_PARA: printf("%s", ostate.end); break;
	case IN_LIST: printf("</li>%s</ul>", ostate.sep); break;
	case IN_PRE: printf("</pre>", ostate.sep); break;
	}
	ostate.is_sep_forced = 1;
	ostate.in = IN_DEFAUT;
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
		case IN_LIST: printf("</li>%s</ul>%s<pre>", ostate.sep, ostate.sep);
			break;
		case IN_PRE: printf("\n"); return;
	}
	ostate.in = IN_PRE;
}
static void state_from_default(void) {
	if(ostate.in == IN_DEFAUT) state_to_para();
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
	state_from_default();
	/* fixme: lists have ws at the end; this should go lazily. */
	fputc('~', stdout);
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
	assert(tokens && t && t->length > 0);
	state_from_default();
	printf("<lit:%.*s>", t->length, t->from);
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
	printf("<%.*s>%.*s",
		t->length - 1, t->from, param->length, param->from);
	*ptoken = TokenArrayNext(tokens, rparen);
	return 1;
	catch:
	fprintf(stderr, "%s: expected generic(id).\n", pos(t));
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
	printf("<%.*s>%.*s<%.*s>%.*s", type1_size, type1, param1->length,
		   param1->from, type2_size, type2, param2->length, param2->from);
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
	printf("<%.*s>%.*s<%.*s>%.*s<%.*s>%.*s", type1_size, type1,
		param1->length, param1->from, type2_size, type2, param2->length,
		param2->from, type3_size, type3, param3->length, param3->from);
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
	printf("[\\%c]", t->from[1]);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(url) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == URL);
	state_from_default();
	printf("<%.*s>", t->length, t->from);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(cite) {
	const struct Token *const t = *ptoken;
	const char *const url_encoded = UrlEncode(t->from, t->length);
	assert(tokens && t && t->symbol == CITE);
	if(!url_encoded) goto catch;
	state_from_default();
	printf("(%.*s)<https://scholar.google.ca/scholar?q=%s>",
		t->length, t->from, url_encoded);
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
	printf("(fixme)<#fn:%.*s>", fn->length, fn->from);
	*ptoken = TokenArrayNext(tokens, fn);
	return 1;
}
OUT(see_tag) {
	const struct Token *const tag = *ptoken;
	assert(tokens && tag && tag->symbol == SEE_FN);
	state_from_default();
	printf("(fixme)<#tag:%.*s>", tag->length, tag->from);
	*ptoken = TokenArrayNext(tokens, tag);
	return 1;
}
OUT(see_typedef) {
	const struct Token *const def = *ptoken;
	assert(tokens && def && def->symbol == SEE_FN);
	state_from_default();
	printf("(fixme)<#typedef:%.*s>", def->length, def->from);
	*ptoken = TokenArrayNext(tokens, def);
	return 1;
}
OUT(see_data) {
	const struct Token *const data = *ptoken;
	assert(tokens && data && data->symbol == SEE_FN);
	state_from_default();
	printf("(fixme)<#data:%.*s>", data->length, data->from);
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
	const struct Token *const t = *ptoken,
		*const desc = TokenArrayNext(tokens, t);
	assert(tokens && t && t->symbol == LINK);
	if(!desc || desc->symbol != LINK) goto catch;
	state_from_default();
	printf("[%.*s](%.*s)", desc->length, desc->from, t->length, t->from);
	*ptoken = TokenArrayNext(tokens, desc);
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
	printf("<pre:%.*s>", t->length, t->from);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}



/* `SYMBOL` is declared in `Scanner.h`. */
static const OutFn symbol_outs[] = { SYMBOL(PARAM5C) };



/** @param[highlights] Must be sorted if not null, creates an emphasis on those
 words. */
static void tokens_print(const struct TokenArray *const tokens,
	const struct TokenRefArray *const highlights) {
	const struct Token *token = TokenArrayNext(tokens, 0);
	struct Token **highlight = TokenRefArrayNext(highlights, 0);
	OutFn sym_out;
	int is_highlight;
	if(!token) return;
	while(token) {
		if(highlight && *highlight == token) {
			is_highlight = 1;
			highlight = TokenRefArrayNext(highlights, highlight);
		} else {
			is_highlight = 0;
		}
		sym_out = symbol_outs[token->symbol];
		if(!sym_out) {
			printf("[undefined token: %s]", symbols[token->symbol]);
			token = TokenArrayNext(tokens, token);
			continue;
		}
		state_sep_if_needed(token->symbol);
		if(is_highlight) printf("<em>");
		if(!sym_out(tokens, &token)) { errno = EILSEQ; return /* fixme */; }
		if(is_highlight) printf("</em>");
	}
}

void ReportDebug(void) {
	struct Segment *segment = 0;
	struct Attribute *att = 0;
	struct Segment {
	enum Division division;
		struct TokenArray doc, code;
		struct TokenRefArray params;
		struct AttributeArray attributes;
	};
	struct Attribute {
		enum Symbol symbol;
		struct TokenArray header;
		struct TokenArray contents;
	};	
	struct Token {
		enum Symbol symbol;
		const char *from;
		int length;
		size_t line;
	};
	while((segment = SegmentArrayNext(&report, segment))) {
		printf("Segment %s: %s;\n"
			"code: %s;\n"
			"params: %s;\n"
			"doc: %s.\n",
			segment->name,
			divisions[segment->division],
			TokenArrayToString(&segment->code),
			TokenRefArrayToString(&segment->params),
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

/** Seaches all `tokens` for `token`. */
static int search_token(const struct TokenArray *const tokens,
	const struct Token *const token) {
	const struct Token *t = 0;
	while((t = TokenArrayNext(tokens, t)))
		if(!token_compare(token, t)) return 1;
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
	printf("<attribute %s>", symbols[symbol]);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute))) {
		if(attribute->token.symbol != symbol) continue;
		printf("[");
		tokens_print(&attribute->header, 0);
		printf("]");
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
	printf("<attribute %s:%s>", symbols[symbol], a);
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
	state_reset("{", "}", ", ");
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
	tokens_print(&segment->code, &segment->params);
	state_to_default();
	printf("\n\n");
}

/** Prints all a `segment`.
 @implements division_act */
static void print_all(const struct Segment *const segment) {
	/* This is the title, the rest are params. */
	struct Token **param = TokenRefArrayNext(&segment->params, 0);
	printf("<%s %s: %s>\n", divisions[segment->division], segment->name,
		TokenRefArrayToString(&segment->params));
	print_code(segment);
	state_reset("<p>", "</p>", "\n\n");
	tokens_print(&segment->doc, 0);
	state_to_default();
	/* fixme: No ATT_TITLE. */
	while((param = TokenRefArrayNext(&segment->params, param)))
		print_attribute_header_maybe(segment, ATT_PARAM, *param);
	print_attribute_maybe(segment, ATT_RETURN);
	print_attribute_maybe(segment, ATT_IMPLEMENTS);
	print_attribute_maybe(segment, ATT_THROWS);
	print_attribute_maybe(segment, ATT_ORDER);
	print_attribute_maybe(segment, ATT_AUTHOR);
	print_attribute_maybe(segment, ATT_STD);
	print_attribute_maybe(segment, ATT_DEPEND);
	print_attribute_maybe(segment, ATT_VERSION);
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
}

/** Outputs a file. */
void ReportOut(void) {
	/* Header. */
	if(preamble_attribute_exists(ATT_TITLE)) {
		printf("<preamble:title># ");
		preamble_attribute_print(ATT_TITLE);
		printf(" #\n\n");
	} else {
		printf("<no title>\n\n");
	}
	if(preamble_attribute_exists(ATT_AUTHOR)) {
		printf("<preamble:author># ");
		preamble_attribute_print(ATT_AUTHOR);
		printf(" #\n\n");
	} else {
		printf("<no author>\n\n");
	}
	/* fixme: Anchors for things. */
	/* Preamble contents. */
	printf("<preamble:contents>");
	preamble_print_all_content();
	printf("\n\n");
	/* Print typedefs. */
	if(division_exists(DIV_TYPEDEF)) {
		printf("## Typedefs ##\n\n");
		division_act(DIV_TYPEDEF, &print_all);
	}
	/* Print tags. */
	if(division_exists(DIV_TAG)) {
		printf("## Tags ##\n\n");
		division_act(DIV_TAG, &print_all);
	}
	/* Print general declarations. */
	if(division_exists(DIV_DATA)) {
		printf("## Data Declarations ##\n\n");
		division_act(DIV_DATA, &print_all);
	}
	/* Print functions. */
	if(division_exists(DIV_FUNCTION)) {
		printf("## Functions ##\n\n");
		division_act(DIV_FUNCTION, &print_code);
		printf("## Function Detail ##\n\n");
		division_act(DIV_FUNCTION, &print_all);
	}
	fputc('\n', stdout);
}
