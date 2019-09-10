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
	fprintf(stderr, "Expected: generic(id); %s.\n", pos(t));
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
	fprintf(stderr, "Expected: generic2(id,id); %s.\n", pos(t));
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
	fprintf(stderr, "Expected: A_B_C_(id,id,id); %s.\n", pos(t));
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
	fprintf(stderr, "Expected: <short source>; %s.\n", pos(t));
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
OUT(math) { /* Math and code. */
	const struct Token *const begin = *ptoken;
	struct Token *next = TokenArrayNext(tokens, begin);
	assert(tokens && begin && begin->symbol == MATH_BEGIN);
	state_from_default();
	printf("<code>");
	while(next->symbol != MATH_END) {
		printf("%.*s", next->length, next->from);
		if(!(next = TokenArrayNext(tokens, next))) goto catch;
	}
	printf("</code>");
	*ptoken = TokenArrayNext(tokens, next);
	return 1;
catch:
	fprintf(stderr, "Expected: `<math/code>`; %s.\n", pos(begin));
	return 0;
}
OUT(em) {
	const struct Token *const begin = *ptoken;
	struct Token *next = TokenArrayNext(tokens, begin);
	assert(tokens && begin && begin->symbol == EM_BEGIN);
	state_from_default();
	printf("<em>");
	while(next->symbol != EM_END) {
		printf("%.*s", next->length, next->from);
		if(!(next = TokenArrayNext(tokens, next))) goto catch;
	}
	printf("</em>");
	*ptoken = TokenArrayNext(tokens, next);
	return 1;
catch:
	fprintf(stderr, "Expected: _<emphasis>_; %s.\n", pos(begin));
	return 0;
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
	fprintf(stderr, "Expected: `[description](url)`; %s.\n", pos(t));
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



static void tokens_print(const struct TokenArray *const tokens) {
	const struct Token *token = TokenArrayNext(tokens, 0);
	OutFn sym_out;
	if(!token) return;
	while(token) {
		sym_out = symbol_outs[token->symbol];
		if(!sym_out) {
			printf("[undefined:%s]", symbols[token->symbol]);
			token = TokenArrayNext(tokens, token);
			continue;
		}
		state_sep_if_needed(token->symbol);
		if(!sym_out(tokens, &token)) { errno = EILSEQ; return /* fixme */; }
	}
}

/** @implements <Attribute>Action */
/*static void print_att_contents(struct Attribute *const att) {
	tokens_print(&att->contents);
}*/

/** @implements <Attribute>Action */
/*static void print_att_header(struct Attribute *const att) {
	tokens_print(&att->header);
}*/

/** @implements <Attribute>Action */
/*static void print_att_header_contents(struct Attribute *const att) {
	printf("<att:%s # ", symbols[att->symbol]);
	print_att_header(att);
	printf(" #\n");
	print_att_contents(att);
	printf(">\n");
}*/

/* @implements <Attribute>Predicate */
#define ATT_IS(lc, uc) static int att_is_ ## lc \
(const struct Attribute *const att) { return att->symbol == uc; }
/*ATT_IS(title, ATT_TITLE)
ATT_IS(param, ATT_PARAM)
ATT_IS(author, ATT_AUTHOR)
ATT_IS(std, ATT_STD)
ATT_IS(depend, ATT_DEPEND)
ATT_IS(version, ATT_VERSION)
 ATT_IS(since, ATT_SINCE)
 ATT_IS(fixme, ATT_FIXME)
 ATT_IS(depricated, ATT_DEPRICATED)
 ATT_IS(return, ATT_RETURN)
 ATT_IS(throws, ATT_THROWS)
 ATT_IS(implements, ATT_IMPLEMENTS)
 ATT_IS(order, ATT_ORDER)
 ATT_IS(allow, ATT_ALLOW)*/

/** @implements <Segment>Action */
/*static void segment_print_doc(struct Segment *const segment) {
	tokens_print(&segment->doc);
}*/

/** @implements <Segment>Action */
/*static void segment_print_code(struct Segment *const segment) {
	tokens_print(&segment->code);
	printf("\n");
}*/

/** @implements <Segment>Action */
/*static void segment_print_all(struct Segment *const segment) {
	segment_print_code(segment);
	segment_print_doc(segment);
	AttributeArrayIfEach(&segment->attributes, &att_is_author, &print_att_contents);
	AttributeArrayIfEach(&segment->attributes, &att_is_std, &print_att_contents);
	AttributeArrayIfEach(&segment->attributes, &att_is_depend, &print_att_contents);
	AttributeArrayIfEach(&segment->attributes, &att_is_param, &print_att_header_contents);
	printf("\n\n***\n\n");
}*/

/** @implements <Segment>Predictate */
/*static int segment_is_declaration(const struct Segment *const segment) {
	return segment->division == DIV_TAG || segment->division == DIV_TYPEDEF;
}*/

/** @implements <Segment>Predictate */
/*static int segment_is_function(const struct Segment *const segment) {
	return segment->division == DIV_FUNCTION;
}*/

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
			printf("%s{%s} %s.\n", symbols[att->symbol],
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
static int attribute_exists(const struct Segment *const segment,
	const enum Symbol symbol) {
	struct Attribute *attribute = 0;
	assert(segment);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
		if(attribute->symbol == symbol) return 1;
	return 0;
}

/** Seaches if attribute `symbol` exists within all preamble segments.
 @order O(`segments` * `attributes`) */
static int preamble_attribute_exists(const enum Symbol symbol) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != DIV_PREAMBLE) continue;
		if(attribute_exists(segment, symbol)) return 1;
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
		if(attribute->symbol != symbol) continue;
		tokens_print(&attribute->contents);
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

/** Prints all a `segment`. */
static void print_content(const struct Segment *const segment) {
	state_reset("{", "}", ", ");
	printf("<general>");
	tokens_print(&segment->code);
	state_reset("<p>", "</p>", "\n\n");
	printf("\n\n");
	tokens_print(&segment->doc);
	state_to_default();
	printf("\n\n");
	if(attribute_exists(segment, ATT_RETURN)) {
		state_reset("{", "}", ", ");
		attribute_print(segment, ATT_RETURN);
		state_to_default();
	}
	printf("</general>\n");
}

/** Prints preable segment's doc. */
static void preamble_print_all_content(void) {
	struct Segment *segment = 0;
	state_reset("<p>", "</p>", "\n\n");
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != DIV_PREAMBLE) continue;
		tokens_print(&segment->doc);
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
		division_act(DIV_TYPEDEF, &print_content);
	}
	/* Print tags. */
	if(division_exists(DIV_TAG)) {
		printf("## Tags ##\n\n");
		division_act(DIV_TAG, &print_content);
	}
	/* Print general declarations. */
	if(division_exists(DIV_DATA)) {
		printf("## Data Declarations ##\n\n");
		division_act(DIV_DATA, &print_content);
	}
	/* Print functions. */
	if(division_exists(DIV_FUNCTION)) {
		printf("## Functions ##\n\n");
		printf("## Function Detail ##\n\n");
	}
	fputc('\n', stdout);
/*
	SegmentArrayIfEach(&report, &segment_is_function, &segment_print_code);
	SegmentArrayIfEach(&report, &segment_is_function, &segment_print_all);*/
}
