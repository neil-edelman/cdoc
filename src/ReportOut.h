#include "UrlEncode.h"

/* `SYMBOL` is declared in `Scanner.h`. */
static const int symbol_before_sep[] = { SYMBOL(PARAM6D) };
static const int symbol_after_sep[]  = { SYMBOL(PARAM6E) };
static const char *symbol_attribute_titles[] = { SYMBOL(PARAM6F) };

/* fixme: hmmm, have a fn to change only lists? */
static const struct ModeText {
	const char *name, *begin_list, *begin_item, *sep_word, *end_item, *sep_item,
		*end_list;
} mode_simple = { "simple", "", "", " ", "", "", "" },
	mode_title = { "title", "<title>", "", " ", "", ", ", "</title>\n" },
	mode_h1  = { "h1",      "<h1>",     "", " ", "", ", ", "</h1>\n\n" },
	mode_h3  = { "h3",      "<h3>",     "", " ", "", ", ", "</h3>\n\n" },
	mode_p   = { "para", "<div>", "<p>", " ", "</p>\n\n", "", "</div>\n\n" },
	mode_ul  = { "ul",   "<ul>\n", "\t<li>", " ", "</li>\n", "", "</ul>\n\n" },
	mode_dl  = { "dl",   "<dl>\n", "\t<dd>", " ", "</dd>\n", "", "</dl>\n\n" },
	mode_code= { "code",    "<code>",   "", "&nbsp;", "", ", ", "</code>"}, /* this is scetchy. */
	mode_pre = { "pre",     "<pre>", "", "", "", "\n", "</pre>" };

static struct {
	const struct ModeText *text;
	enum { IN_CLEAR, IN_LIST, IN_LIST_LAZY, IN_ITEM, IN_ITEM_LAZY } in;
	int is_before_sep;
} mode = { &mode_simple, IN_CLEAR, 0 };

/** Change the mode to clear. */
static void flush_mode(void) {
	printf("<!-- flush %s -->", mode.text->name);
	switch(mode.in) {
		case IN_CLEAR: break;
		case IN_ITEM:
		case IN_ITEM_LAZY: fputs(mode.text->end_item, stdout);
		case IN_LIST:
		case IN_LIST_LAZY: fputs(mode.text->end_list, stdout);
	}
	mode.in = IN_CLEAR;
	mode.is_before_sep = 0;
}

/** Change to another output `text_state`. */
static void change_mode(const struct ModeText *const text) {
	assert(text);
	if(mode.text != text) {
		const char *const prev_text_name = mode.text->name;
		flush_mode();
		mode.text = text;
		printf("<!-- %s becomes %s -->", prev_text_name, mode.text->name);
	} else if(mode.in == IN_ITEM || mode.in == IN_ITEM_LAZY) {
		fputs(mode.text->end_list, stdout);
		mode.in = IN_LIST_LAZY;
		mode.is_before_sep = 0;
	}
	return;
}

/** Right before we print. */
static void mode_ready(const enum Symbol symbol) {
	switch(mode.in) {
	case IN_CLEAR: fputs(mode.text->begin_list, stdout);
	case IN_LIST: fputs(mode.text->begin_item, stdout); break;
	case IN_LIST_LAZY:
		printf("%s%s", mode.text->sep_item, mode.text->begin_item); break;
	case IN_ITEM:
		if(mode.is_before_sep && symbol_before_sep[symbol])
			fputs(mode.text->sep_word, stdout);
		mode.is_before_sep = symbol_after_sep[symbol];
		break;
	case IN_ITEM_LAZY: fputs(mode.text->sep_word, stdout); break;
	}
	mode.in = IN_ITEM;
}

/** Differed space. */
static void mode_lazy(void) {
	if(mode.in == IN_ITEM) mode.in = IN_ITEM_LAZY;
}

/** Paragraph/li/whatever end. */
static void mode_end_item(void) {
	switch(mode.in) {
	case IN_CLEAR:
	case IN_LIST:
	case IN_LIST_LAZY: return;
	case IN_ITEM:
	case IN_ITEM_LAZY: fputs(mode.text->end_item, stdout);
	}
	mode.in = IN_LIST_LAZY;
	mode.is_before_sep = 0;
}

/** Encode a bunch of arbitrary text as html. */
static void html_encode(int length, const char *from) {
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
	mode_lazy();
	*ptoken = TokenArrayNext(tokens, space);
	return 1;
}
OUT(par) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NEWLINE);
	mode_end_item();
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(lit) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->length > 0 && t->from);
	mode_ready(t->symbol);
	html_encode(t->length, t->from);
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
	mode_ready(t->symbol);
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
	mode_ready(t->symbol);
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
	mode_ready(t->symbol);
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
	mode_ready(t->symbol);
	html_encode(1, t->from + 1);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(url) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == URL);
	mode_ready(t->symbol);
	printf("<a href = \"%.*s\">", t->length, t->from);
	html_encode(t->length, t->from);
	printf("</a>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(cite) {
	const struct Token *const t = *ptoken;
	const char *const url_encoded = UrlEncode(t->from, t->length);
	assert(tokens && t && t->symbol == CITE);
	if(!url_encoded) goto catch;
	mode_ready(t->symbol);
	printf("<a href = \"https://scholar.google.ca/scholar?q=%s\">",
		url_encoded);
	html_encode(t->length, t->from);
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
	mode_ready(fn->symbol);
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_FUNCTION],
		fn->length, fn->from, fn->length, fn->from);
	*ptoken = TokenArrayNext(tokens, fn);
	return 1;
}
OUT(see_tag) {
	const struct Token *const tag = *ptoken;
	assert(tokens && tag && tag->symbol == SEE_TAG);
	mode_ready(tag->symbol);
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_TAG],
		tag->length, tag->from, tag->length, tag->from);
	*ptoken = TokenArrayNext(tokens, tag);
	return 1;
}
OUT(see_typedef) {
	const struct Token *const def = *ptoken;
	assert(tokens && def && def->symbol == SEE_TYPEDEF);
	mode_ready(def->symbol);
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_TYPEDEF],
		def->length, def->from, def->length, def->from);
	*ptoken = TokenArrayNext(tokens, def);
	return 1;
}
OUT(see_data) {
	const struct Token *const data = *ptoken;
	assert(tokens && data && data->symbol == SEE_DATA);
	mode_ready(data->symbol);
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_DATA],
		data->length, data->from, data->length, data->from);
	*ptoken = TokenArrayNext(tokens, data);
	return 1;
}
OUT(math_begin) { /* Math and code. */
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_BEGIN);
	mode_ready(t->symbol);
	printf("<code>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(math_end) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_END);
	mode_ready(t->symbol);
	printf("</code>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_begin) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_BEGIN);
	mode_ready(t->symbol);
	printf("<em>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_end) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_END);
	mode_ready(t->symbol);
	printf("</em>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(link) {
	const struct Token *const t = *ptoken, *text, *turl;
	assert(tokens && t && t->symbol == LINK_START);
	mode_ready(t->symbol);
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
	mode_ready(t->symbol);
	printf("&nbsp;");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(nbthinsp) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NBTHINSP);
	mode_ready(t->symbol);
	printf("&#8239;" /* "&thinsp;" <- breaking? */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(list) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == LIST_ITEM);
	change_mode(&mode_ul);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(pre) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == PREFORMATTED);
	change_mode(&mode_pre);
	html_encode(t->length, t->from);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}



/* `SYMBOL` is declared in `Scanner.h`. */
static const OutFn symbol_outs[] = { SYMBOL(PARAM6C) };



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
		/*state_sep_if_needed(token->symbol);*/
		if(is_highlight) printf("<em>");
		token = print_token(tokens, token);
		if(is_highlight) printf("</em>");
	}
}

static void print_tokens(const struct TokenArray *const tokens) {
	highlight_tokens(tokens, 0);
}

void ReportDebug(void) {
#ifdef DEBUG
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
#endif
}

/* Lambdas would be nice! fixme: this needs rewriting and simplifying in light
 of modes. It should be simpler. */

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
		mode_end_item();
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
		mode_end_item();
	}
}

/** For each `division` segment, print all attributes that match `symbol`.
 @order O(`segments` * `attributes`) */
static void division_attribute_print(const enum Division division,
	const enum Symbol symbol) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != division) continue;
		attribute_print(segment, symbol);
		mode_end_item();
	}
}

static void print_attribute_maybe(const struct Segment *const segment,
	const enum Symbol symbol) {
	assert(segment && symbol);
	printf("<!-- print attribute maybe??? -->");
	if(!segment_attribute_exists(segment, symbol)) return;
	printf("\n\n");
	change_mode(&mode_dl);/*reset_state("{", "}", ", ");*/
	attribute_print(segment, symbol);
	mode_end_item(); /* state_to_default(); */
}

static void print_attribute_header_maybe(const struct Segment *const segment,
	const enum Symbol symbol, const struct Token *header) {
	assert(segment && symbol);
	printf("<!-- print attribute header maybe??? -->");
	if(!segment_attribute_header_exists(segment, symbol, header)) return;
	printf("\n\n");
	change_mode(&mode_dl);/*reset_state("{", "}", ", ");*/
	attribute_header_print(segment, symbol, header);
	mode_end_item(); /* state_to_default(); */
}

/** Prints all a `segment`.
 @implements division_act */
static void segment_print_all(const struct Segment *const segment) {
	const struct Token *param;
	size_t no;
	assert(segment);
	/* The title is generally the first param. Only single-words. */
	if((param = param_no(segment, 0))) {
		change_mode(&mode_p);/*reset_state("", "", "???");*/
		printf("<a name = \"%s:", division_strings[segment->division]);
		print_token(&segment->code, param);
		printf("\"><!-- --></a>\n");
		change_mode(&mode_h3);/*reset_state("<h3>", "</h3>", "???");*/
		print_token(&segment->code, param);
		mode_end_item(); /* state_to_default(); */
		printf("\n\n");
		change_mode(&mode_code);
		highlight_tokens(&segment->code, &segment->code_params);
		flush_mode();
	}
	change_mode(&mode_p);
	print_tokens(&segment->doc);
	mode_end_item(); /* state_to_default(); */
	if(segment->division == DIV_PREAMBLE) {
		/*fixme: print_attributes_header(segment, ATT_PARAM);*/
	} else if(segment->division == DIV_FUNCTION) {
		for(no = 1; (param = param_no(segment, no)); no++)
			print_attribute_header_maybe(segment, ATT_PARAM, param);
		print_attribute_maybe(segment, ATT_RETURN);
		print_attribute_maybe(segment, ATT_IMPLEMENTS);
		print_attribute_maybe(segment, ATT_THROWS);
		print_attribute_maybe(segment, ATT_ORDER);
	} else if(segment->division == DIV_TAG) {
		/*fixme: print_attributes_header(segment, ATT_PARAM);*/
	}
	print_attribute_maybe(segment, ATT_AUTHOR);
	print_attribute_maybe(segment, ATT_STD);
	print_attribute_maybe(segment, ATT_DEPEND);
	print_attribute_maybe(segment, ATT_FIXME);
	print_attribute_maybe(segment, ATT_LICENSE);
}

/** Prints preable segment's doc. */
static void preamble_print_all_content(void) {
	struct Segment *segment = 0;
	change_mode(&mode_p);
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != DIV_PREAMBLE) continue;
		print_tokens(&segment->doc);
		mode_end_item(); /* state_to_default(); */
	}
	printf("\n\nAlso print attributes.\n\n");
	/* fixme: also print fn attributes for @since @std @depend, _etc_. */
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
		is_license = division_attribute_exists(DIV_PREAMBLE, ATT_LICENSE);
	/* We set `errno` here so that we don't have to test output each time. */
	errno = 0;
	/* fixme: how to set utf-8? */
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
	change_mode(&mode_title);
	division_attribute_print(DIV_PREAMBLE, ATT_TITLE);
	flush_mode();
	printf("</head>\n\n"
		"<body>\n\n");

	change_mode(&mode_h1),
	division_attribute_print(DIV_PREAMBLE, ATT_TITLE),
	flush_mode();

	change_mode(&mode_p);
	division_attribute_print(DIV_PREAMBLE, ATT_AUTHOR);
	flush_mode();

	change_mode(&mode_ul);
	if(is_typedef || is_tag || is_data || is_function) mode_ready(END),
		printf("<a href = \"#summary:\">Summary</a>"), mode_end_item();
	if(is_preamble) mode_ready(END), printf("<a href = \"#%s:\">Preamble</a>",
		division_strings[DIV_PREAMBLE]), mode_end_item();
	if(is_typedef) mode_ready(END),
		printf("<a href = \"#%s:\">Typedef Aliases</a>",
		division_strings[DIV_TYPEDEF]), mode_end_item();
	if(is_tag) mode_ready(END),
		printf("<a href = \"#%s:\">Struct, Union, and Enum Definitions</a>",
		division_strings[DIV_TAG]), mode_end_item();
	if(is_data) mode_ready(END),
		printf("<a href = \"#%s:\">Data Definitions</a>",
		division_strings[DIV_DATA]), mode_end_item();
	if(is_function) mode_ready(END),
		printf("<a href = \"#%s:\">Function Definitions</a>",
		division_strings[DIV_FUNCTION]), mode_end_item();
	if(is_license) mode_ready(END),
		printf("<a href = \"#license:\">License</a>"), mode_end_item();
	flush_mode();

	/* Print summary. */
	change_mode(&mode_simple);
	if(is_typedef || is_tag || is_data || is_function)
		printf("<a name = \"summary:\"><!-- --></a>\n"
		"<h2>Summary</h2>\n\n");
	if(is_typedef) {
		const struct Segment *segment = 0;
		printf("<table>\n\n"
			"<tr><th>Typedef Alias</th><th>Full</th></tr>\n\n");
		while((segment = SegmentArrayNext(&report, segment))) {
			size_t *idxs;
			struct Token *params;
			if(segment->division != DIV_TYPEDEF
				|| !SizeArraySize(&segment->code_params)) continue;
			idxs = SizeArrayGet(&segment->code_params);
			params = TokenArrayGet(&segment->code);
			assert(idxs[0] < TokenArraySize(&segment->code));
			printf("<tr><td><a href = \"#%s:", division_strings[DIV_TYPEDEF]);
			print_token(&segment->code, params + idxs[0]);
			printf("\">");
			print_token(&segment->code, params + idxs[0]);
			printf("</a></td><td>");
			/* fixme: More advanced in Semantic: we know "typedef ". */
			change_mode(&mode_code);
			print_tokens(&segment->code);
			change_mode(&mode_simple);
			printf("</td></tr>\n\n");
		}
		printf("</table>\n\n");
	}
	if(is_tag) {
		const struct Segment *segment = 0;
		printf("<table>\n\n"
			"<tr><th>Struct, Union, or Enum</th></tr>\n\n");
		while((segment = SegmentArrayNext(&report, segment))) {
			size_t *idxs;
			struct Token *params;
			if(segment->division != DIV_TAG
				|| !SizeArraySize(&segment->code_params)) continue;
			idxs = SizeArrayGet(&segment->code_params);
			params = TokenArrayGet(&segment->code);
			assert(idxs[0] < TokenArraySize(&segment->code));
			/* fixme: tag type? */
			printf("<tr><td><a href = \"#%s:", division_strings[DIV_TAG]);
			print_token(&segment->code, params + idxs[0]);
			printf("\">");
			print_token(&segment->code, params + idxs[0]);
			printf("</a></td></tr>\n\n");
		}
		printf("</table>\n\n");
	}
	if(is_data) {
		const struct Segment *segment = 0;
		printf("<table>\n\n"
			"<tr><th>General Declarations</th></tr>\n\n");
		while((segment = SegmentArrayNext(&report, segment))) {
			size_t *idxs;
			struct Token *params;
			if(segment->division != DIV_DATA
			   || !SizeArraySize(&segment->code_params)) continue;
			idxs = SizeArrayGet(&segment->code_params);
			params = TokenArrayGet(&segment->code);
			assert(idxs[0] < TokenArraySize(&segment->code));
			/* fixme: data type? */
			printf("<tr><td><a href = \"#%s:", division_strings[DIV_DATA]);
			print_token(&segment->code, params + idxs[0]);
			printf("\">");
			print_token(&segment->code, params + idxs[0]);
			printf("</a></td></tr>\n\n");
		}
		printf("</table>\n\n");
	}
	if(is_function) {
		const struct Segment *segment = 0;
		printf("<table>\n\n"
			"<tr><th>Return Type</th><th>Function Name</th>"
			"<th>Argument List</th></tr>\n\n");
		while((segment = SegmentArrayNext(&report, segment))) {
			size_t *idxs, idxn, idx, paramn;
			struct Token *params;
			if(segment->division != DIV_FUNCTION
				|| !(idxn = SizeArraySize(&segment->code_params))) continue;
			idxs = SizeArrayGet(&segment->code_params);
			params = TokenArrayGet(&segment->code);
			paramn = TokenArraySize(&segment->code);
			assert(idxs[0] < paramn);
			/* fixme: hard! */
			printf("<tr><td>donno</td><td><a href = \"#%s:",
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
	}
	flush_mode(); /* Does nothing; just for symmetry. */

	/* Preamble contents. */
	if(is_preamble) {
		printf("<a name = \"%s:\"><!-- --></a>\n"
			"<h2>Preamble</h2>\n\n",
			division_strings[DIV_PREAMBLE]);
		change_mode(&mode_p);
		preamble_print_all_content();
		flush_mode();
	}

	/* Print typedefs. */
	if(is_typedef) {
		printf("<a name = \"%s:\"><!-- --></a>"
			"<h2>Typedef Aliases</h2>\n\n", division_strings[DIV_TYPEDEF]);
		division_act(DIV_TYPEDEF, &segment_print_all);
	}
	/* Print tags. */
	if(is_tag) {
		printf("<a name = \"%s:\"><!-- --></a>"
			"<h2>Struct, Union, and Enum Definitions</h2>\n\n",
			division_strings[DIV_TAG]);
		division_act(DIV_TAG, &segment_print_all);
	}
	/* Print general declarations. */
	if(is_data) {
		printf("<a name = \"%s:\"><!-- --></a>"
			"<h2>General Definitions</h2>\n\n", division_strings[DIV_DATA]);
		division_act(DIV_DATA, &segment_print_all);
	}
	/* Print functions. */
	if(is_function) {
		printf("<a name = \"%s:\"><!-- --></a>"
			"<h2>Function Definitions</h2>\n\n",
			division_strings[DIV_FUNCTION]);
		division_act(DIV_FUNCTION, &segment_print_all);
	}
	/* License. */
	if(is_license) {
		printf("<a name = \"license:\"><!-- --></a>\n"
			"<h2>License</h2>\n\n");
		change_mode(&mode_p);/*reset_state("<p>", "</p>", "\n\n");*/
		division_attribute_act(DIV_PREAMBLE, ATT_LICENSE, &print_tokens);
		mode_end_item(); /* state_to_default(); */
		printf("\n\n");
	}

	flush_mode();
	printf("</body>\n"
		"</html>\n");
	return errno ? 0 : 1;
}
