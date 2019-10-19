#include "UrlEncode.h"

/* `SYMBOL` is declared in `Scanner.h`. */
static const int symbol_before_sep[] = { SYMBOL(PARAM6D) };
static const int symbol_after_sep[]  = { SYMBOL(PARAM6E) };
static const char *symbol_attribute_titles[] = { SYMBOL(PARAM6F) };

/* `sprintf` titles. */
static char title[256];

/* Every `StyleText` can have a beginning, a separator, and an end, which will
 be printed around literals. Block and can appear alone elements have
 `is_next_level`. */
static const struct StyleText {
	const char *name, *begin, *sep, *end;
	int is_next_level;
} no_style = { "no style", "", "", "", 0 },
	plain_text = { "text",  "", " ", "", 0 },
	plain_parenthetic = { "parenthetic", "(", " ", ")", 0 },
	plain_csv = { "csv", "", ", ", "", 0 },
	plain_ssv = { "ssv", "", "; ", "", 0 },
	html_p   = { "para", "<p>", " ", "</p>\n\n", 1 },
	html_ul  = { "ul",   "<ul>\n", "", "</ul>\n\n", 1 },
	html_li  = { "li",   "\t<li>", " ", "</li>\n", 0 },
	/*html_dt  = { "dt",   "\t<dt>", " ", "</dt>\n", 0 },
	html_dd  = { "dd",   "\t<dd>", " ", "</dd>\n", 0 },*/
	html_code= { "code", "<code>", "&nbsp;", "</code>", 0},
	html_pre = { "pre",  "<pre>\n", "", "</pre>\n\n", 1 },
	html_pre_line = { "line", "", "\n", "\n", 0 },
	html_title = { "title", "<title>", "", "</title>\n", 1 },
	html_h1  = { "h1",   "<h1>", "", "</h1>\n\n", 1 },
	html_h3  = { "h3",   "<h3>", "", "</h3>\n\n", 1 },
	html_dl  = { "dl",   "<dl>\n", "", "</dl>\n\n", 1 },
	html_desc = { "desc", title, "", "</dd>\n", 0 };

/* block: address article(5) aside(5) blockquote canvas(5) dd div dl dt
 fieldset? figcaption(5) figure(5) footer(5) form h1-6 header(5) hr li main(5)
 nav(5) noscript ol p pre section(5) tfoot ul video(5) */

/* inline: a abbr acronym b bdo big br button cite code dfn em i img input kbd
 label map object output q (quote) samp script select small span strong sub sup
 textarea time(5) tt(4) var */
/* em strong code samp kbd var */

/* This does a delayed lazy unencoded surrounding text. Popping a `Style` and
 pushing another one, then printing, will cause a end-separator-start-print.
 Alternately, by explicitly `SPACE` or having `symbol_before_sep` and
 `symbol_after_sep` both true. Eg, say the stack is: { h1, p -> } and we have
 not printed anything, it will be empty. Then we print "foo", it will output
 "<h1><p>foo", then "bar", "<h1><p>foobar", pop "<h1><p>foobar</p>\n\n", pop
 "<h1><p>foobar</p>\n\n</h1>\n\n".  */
struct Style {
	const struct StyleText *text;
	enum { BEGIN, ITEM, SEPARATE } lazy;
};

static void style_to_string(const struct Style *s, char (*const a)[12]) {
	sprintf(*a, "%.11s", s->text->name);
}

#define ARRAY_NAME Style
#define ARRAY_TYPE struct Style
#define ARRAY_TO_STRING &style_to_string
#define ARRAY_STACK
#include "Array.h"

/** `mode` is the global style stack with a flag that controls auto-spacing. */
static struct {
	struct StyleArray styles;
	int is_before_sep;
} mode;

static void style_clear(void) {
	assert(!StyleArraySize(&mode.styles));
	StyleArray_(&mode.styles);
	mode.is_before_sep = 0;
}

static void style_push(const struct StyleText *const text) {
	struct Style *const push = StyleArrayNew(&mode.styles);
	assert(text);
	/* There's so many void functions that rely on this function and it's such
	 a small amount of memory, that it's useless to recover. The OS will have
	 to clean up our mess. */
	if(!push) { perror("Unrecoverable"), exit(EXIT_FAILURE); return; }
	printf("<!-- push %s -->", text->name);
	push->text = text;
	push->lazy = BEGIN;
}

static void style_pop(void) {
	struct Style *const pop = StyleArrayPop(&mode.styles),
		*const top = StyleArrayPeek(&mode.styles);
	assert(pop);
	printf("<!-- pop %s -->", pop->text->name);
	if(pop->lazy == BEGIN) return;
	/* Was used. */
	fputs(pop->text->end, stdout);
	if(top) assert(top->lazy != BEGIN), top->lazy = SEPARATE;
}

/** Pops until the the element that is popped is a block element and can appear
 outside all elements. */
static void style_pop_level(void) {
	struct Style *top;
	while((top = StyleArrayPeek(&mode.styles))) {
		style_pop();
		if(top->text->is_next_level) break;
	}
}

/** Pops and then pushes the same element. */
static void style_pop_push(void) {
	struct Style *const peek = StyleArrayPeek(&mode.styles);
	assert(peek);
	style_pop();
	style_push(peek->text);
}

/** Some of the elements require looking at the style on top. */
static const struct StyleText *style_text_peek(void) {
	const struct Style *const s = StyleArrayPeek(&mode.styles);
	return s ? s->text : 0;
}

/** Right before we print. If one doesn't have a `symbol`, just pass
 `END == 0`. */
static void style_prepare_output(const enum Symbol symbol) {
	struct Style *const top = StyleArrayPeek(&mode.styles), *style = 0;
	assert(top);
	/* Make sure all the stack is on `ITEM`. */
	while((style = StyleArrayNext(&mode.styles, style))) {
		switch(style->lazy) {
		case ITEM: continue;
		case SEPARATE: fputs(style->text->sep, stdout); break;
		case BEGIN: fputs(style->text->begin, stdout); break;
		}
		style->lazy = ITEM;
		mode.is_before_sep = 0;
	}
	assert(top->lazy == ITEM);
	/* If there was no separation, is there an implied separation? */
	if(mode.is_before_sep && symbol_before_sep[symbol])
		fputs(top->text->sep, stdout);
	mode.is_before_sep = symbol_after_sep[symbol];
}

/** Differed space. */
static void style_separate(void) {
	struct Style *const top = StyleArrayPeek(&mode.styles);
	assert(top);
	if(top->lazy == ITEM) top->lazy = SEPARATE;
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
	style_separate();
	*ptoken = TokenArrayNext(tokens, space);
	return 1;
}
OUT(par) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NEWLINE);
	style_pop_level();
	style_push(&html_p);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(lit) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->length > 0 && t->from);
	style_prepare_output(t->symbol);
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
	style_prepare_output(t->symbol);
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
	style_prepare_output(t->symbol);
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
	style_prepare_output(t->symbol);
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
	style_prepare_output(t->symbol);
	html_encode(1, t->from + 1);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(url) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == URL);
	style_prepare_output(t->symbol);
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
	style_prepare_output(t->symbol);
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
	style_prepare_output(fn->symbol);
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_FUNCTION],
		fn->length, fn->from, fn->length, fn->from);
	*ptoken = TokenArrayNext(tokens, fn);
	return 1;
}
OUT(see_tag) {
	const struct Token *const tag = *ptoken;
	assert(tokens && tag && tag->symbol == SEE_TAG);
	style_prepare_output(tag->symbol);
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_TAG],
		tag->length, tag->from, tag->length, tag->from);
	*ptoken = TokenArrayNext(tokens, tag);
	return 1;
}
OUT(see_typedef) {
	const struct Token *const def = *ptoken;
	assert(tokens && def && def->symbol == SEE_TYPEDEF);
	style_prepare_output(def->symbol);
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_TYPEDEF],
		def->length, def->from, def->length, def->from);
	*ptoken = TokenArrayNext(tokens, def);
	return 1;
}
OUT(see_data) {
	const struct Token *const data = *ptoken;
	assert(tokens && data && data->symbol == SEE_DATA);
	style_prepare_output(data->symbol);
	printf("<a href = \"#%s:%.*s\">%.*s</a>", division_strings[DIV_DATA],
		data->length, data->from, data->length, data->from);
	*ptoken = TokenArrayNext(tokens, data);
	return 1;
}
OUT(math_begin) { /* Math and code. */
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_BEGIN);
	style_prepare_output(t->symbol);
	printf("<code>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(math_end) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_END);
	style_prepare_output(t->symbol);
	printf("</code>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_begin) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_BEGIN);
	style_prepare_output(t->symbol);
	printf("<em>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_end) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_END);
	style_prepare_output(t->symbol);
	printf("</em>");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(link) {
	const struct Token *const t = *ptoken, *text, *turl;
	assert(tokens && t && t->symbol == LINK_START);
	style_prepare_output(t->symbol);
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
	style_prepare_output(t->symbol);
	printf("&nbsp;");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(nbthinsp) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NBTHINSP);
	style_prepare_output(t->symbol);
	printf("&#8239;" /* "&thinsp;" <- breaking? */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(list) {
	const struct StyleText *const peek = style_text_peek();
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == LIST_ITEM && peek);
	if(peek == &html_li) {
		style_pop_push();
	} else {
		style_pop_level();
		style_push(&html_ul), style_push(&html_li);
	}
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
/* @fixme This is broken. Want it to `<pre>` all the way to the end, instead,
 `<pre>` is happening each time. It is very messy, but approximates what we
 want. Styles must be more complicated. */
OUT(pre) {
	const struct StyleText *const peek = style_text_peek();
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == PREFORMATTED && peek);
	if(peek != &html_pre_line) {
		style_pop_level();
		style_push(&html_pre), style_push(&html_pre_line);
	}
	style_prepare_output(t->symbol);
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
		if(is_highlight) printf("<em>");
		token = print_token(tokens, token);
		if(is_highlight) printf("</em>");
	}
}

static void print_tokens(const struct TokenArray *const tokens) {
	highlight_tokens(tokens, 0);
}

void ReportDebug(void) {
#ifdef DEBUG_SEGMENTS
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

typedef int (*DivisionPredicate)(const enum Division);

/** @implements DivisionPredicate */
static int is_div_preamble(const enum Division d) {
	return d == DIV_PREAMBLE;
}

/** @implements DivisionPredicate */
static int is_not_div_preamble(const enum Division d) {
	return d != DIV_PREAMBLE;
}



/** For `segment`, print all attributes that match `symbol`.
 @param[is_details] Prints where is is first.
 @order O(`attributes`) */
static void segment_att_print(const struct Segment *const segment,
	const enum Symbol symbol, const int is_details) {
	struct Attribute *attribute = 0;
	assert(segment);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute))) {
		size_t *pindex;
		if(attribute->token.symbol != symbol) continue;
		if(is_details) {
			style_prepare_output(END);
			printf("%s", division_strings[segment->division]);
			if((pindex = SizeArrayNext(&segment->code_params, 0))
			   && *pindex < TokenArraySize(&segment->code)) {
				const struct Token *token
					= TokenArrayGet(&segment->code) + *pindex;
				fputc(' ', stdout);
				print_token(&segment->code, token);
			}
			fputs(": ", stdout);
		}
		print_tokens(&attribute->contents);
		style_pop_push();
	}
}
/** For each `division` segment, print all attributes that match `symbol`.
 @param[is_details] Prints where is is first.
 @order O(`segments` * `attributes`) */
static void div_att_print(const DivisionPredicate div_pred,
	const enum Symbol symbol, const int is_details) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment)))
		if(!div_pred || div_pred(segment->division))
		segment_att_print(segment, symbol, is_details);
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

/** @return Is `division` in the report?
 <needed> */
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

/** @return Is `attribute_symbol` under `division` in the report?
 <needed> */
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
		/*style_end_item();*/
	}
}

static void print_attribute_maybe(const struct Segment *const segment,
	const enum Symbol symbol) {
	assert(segment && symbol);
	printf("<!-- print attribute maybe??? -->");
	if(!segment_attribute_exists(segment, symbol)) return;
	printf("\n\n");
	style_push(&html_dl);
	segment_att_print(segment, symbol, 0);
	/*style_end_item()*/; /* state_to_default(); */
}

static void print_attribute_header_maybe(const struct Segment *const segment,
	const enum Symbol symbol, const struct Token *header) {
	assert(segment && symbol);
	printf("<!-- print attribute header maybe??? -->");
	if(!segment_attribute_header_exists(segment, symbol, header)) return;
	printf("\n\n");
	style_push(&html_dl);
	attribute_header_print(segment, symbol, header);
	/*style_end_item()*/; /* state_to_default(); */
}

/** Prints all a `segment`.
 @implements division_act */
static void segment_print_all(const struct Segment *const segment) {
	const struct Token *param;
	size_t no;
	assert(segment);
	/* The title is generally the first param. Only single-words. */
	if((param = param_no(segment, 0))) {
		style_push(&html_p);
		printf("<a name = \"%s:", division_strings[segment->division]);
		print_token(&segment->code, param);
		printf("\"><!-- --></a>\n");
		style_push(&html_h3);
		print_token(&segment->code, param);
		/*style_end_item()*/; /* state_to_default(); */
		printf("\n\n");
		style_push(&html_code);
		highlight_tokens(&segment->code, &segment->code_params);
		style_pop();
	}
	style_push(&html_p);
	print_tokens(&segment->doc);
	/*style_end_item()*/; /* state_to_default(); */
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
	/* Set `errno` here so that we don't have to test output each time. */
	errno = 0;
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
	style_push(&html_title), style_push(&plain_ssv), style_push(&plain_text);
	div_att_print(&is_div_preamble, ATT_TITLE, 0);
	style_pop_level();
	assert(!StyleArraySize(&mode.styles));
	printf("</head>\n\n"
		"<body>\n\n");

	/* Title. */
	style_push(&html_h1), style_push(&plain_ssv), style_push(&plain_text);
	div_att_print(&is_div_preamble, ATT_TITLE, 0);
	style_pop_level();
	assert(!StyleArraySize(&mode.styles));

	/* Search for any author. */
	style_push(&html_p), style_push(&plain_csv), style_push(&plain_text);
	div_att_print(&is_div_preamble, ATT_AUTHOR, 0);
	style_pop(), style_push(&plain_parenthetic),
	style_push(&plain_csv), style_push(&plain_text);
	div_att_print(&is_not_div_preamble, ATT_AUTHOR, 1);
	style_pop_level();
	assert(!StyleArraySize(&mode.styles));

	/* TOC. */
	style_push(&html_ul), style_push(&html_li);
	if(is_typedef || is_tag || is_data || is_function)
		style_prepare_output(END),
		printf("<a href = \"#summary:\">Summary</a>"), style_pop_push();
	if(is_preamble) style_prepare_output(END), printf("<a href = \"#%s:\">Preamble</a>",
		division_strings[DIV_PREAMBLE]), style_pop_push();
	if(is_typedef) style_prepare_output(END),
		printf("<a href = \"#%s:\">Typedef Aliases</a>",
		division_strings[DIV_TYPEDEF]), style_pop_push();
	if(is_tag) style_prepare_output(END),
		printf("<a href = \"#%s:\">Struct, Union, and Enum Definitions</a>",
		division_strings[DIV_TAG]), style_pop_push();
	if(is_data) style_prepare_output(END),
		printf("<a href = \"#%s:\">Data Definitions</a>",
		division_strings[DIV_DATA]), style_pop_push();
	if(is_function) style_prepare_output(END),
		printf("<a href = \"#%s:\">Function Definitions</a>",
		division_strings[DIV_FUNCTION]), style_pop_push();
	if(is_license) style_prepare_output(END),
		printf("<a href = \"#license:\">License</a>"), style_pop_push();
	style_pop(), /* li */ style_pop(); /* ul */
	assert(!StyleArraySize(&mode.styles));

	/* Print summary. fixme: this could be way shorter. */
	style_push(&no_style);
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
			style_push(&html_code);
			print_tokens(&segment->code);
			style_pop();
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
	style_pop(); /* no_style */
	assert(!StyleArraySize(&mode.styles));

	/* Preamble contents. */
	if(is_preamble) {
		struct Segment *segment = 0;
		printf("<a name = \"%s:\"><!-- --></a>\n"
			"<h2>Preamble</h2>\n\n",
			division_strings[DIV_PREAMBLE]);
		while((segment = SegmentArrayNext(&report, segment))) {
			if(segment->division != DIV_PREAMBLE) continue;
			style_push(&html_p);
			print_tokens(&segment->doc);
			style_pop_level();
		}

		printf("\n\n[Also print attributes.]\n\n");

		style_push(&html_dl);

		/* Search for any author. */
		sprintf(title, "\t<dt>%s</dt>\n"
			"\t<dd>", symbol_attribute_titles[ATT_AUTHOR]);
		style_push(&html_desc);
		style_push(&plain_csv), style_push(&plain_text);
		div_att_print(&is_div_preamble, ATT_AUTHOR, 0);
		style_pop(), style_push(&plain_parenthetic), style_push(&plain_csv),
			style_push(&plain_text);
		div_att_print(&is_not_div_preamble, ATT_AUTHOR, 1);
		style_pop(), style_pop(), style_pop(), style_pop(), style_pop();
		assert(StyleArraySize(&mode.styles) == 1);

		style_pop_level();

		/* fixme: also print fn attributes for @since @std @depend, _etc_. */
	}

#if 0
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
		style_push(&html_p);
		division_attribute_act(DIV_PREAMBLE, ATT_LICENSE, &print_tokens);
		/*style_end_item()*/; /* state_to_default(); */
		printf("\n\n");
	}
#endif

	printf("</body>\n"
		"</html>\n");
	style_clear();
	return errno ? 0 : 1;
}
