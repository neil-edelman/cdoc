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
	plain_see_license = { "see", "(See license details ", ", ", ".)", 0 },
	plain_csv = { "csv", "", ", ", "", 0 },
	plain_ssv = { "ssv", "", "; ", "", 0 },
	html_div = { "div", "<div>", "", "</div>\n\n", 1},
	html_p   = { "para", "<p>", " ", "</p>\n\n", 1 },
	md_p     = { "para", "", " ", "\n\n", 1 },
	html_ul  = { "ul",   "<ul>\n", "", "</ul>\n\n", 1 },
	html_li  = { "li",   "\t<li>", " ", "</li>\n", 0 },
	md_ul    = { "ul",   "",       "",  "\n", 1 },
	md_li    = { "li",   " * ",    " ", "\n", 0 },
	html_code= { "code", "<code>", /*"&nbsp;"*/" ", "</code>", 0},
	html_pre = { "pre",  "<pre>\n", "", "</pre>\n\n", 1 },
	html_pre_line = { "line", "", "\n", "\n", 0 },
	md_pre = { "pre",  "", "", "\n", 1 }, /* fixme: not sure. */
	md_pre_line = { "line", "", "\n    ", "\n", 0 },
	html_title = { "title", "<title>", "", "</title>\n", 1 },
	html_h1  = { "h1",   "<h1>", "", "</h1>\n\n", 1 },
	html_h3  = { "h3",   "<h3>", "", "</h3>\n\n", 1 },
	html_dl  = { "dl",   "<dl>\n", "", "</dl>\n\n", 1 },
	html_dt  = { "dt",   "\t<dt>", "", "</dt>\n", 0 },
	html_dd  = { "dd",   "\t<dd>", "", "</dd>\n", 0 },
	html_desc = { "desc", title, "", "</dd>\n", 0 },
	html_em = { "em", "<em>", "", "</em>", 0 };

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
	/*printf("<!-- push %s -->", text->name);*/
	push->text = text;
	push->lazy = BEGIN;
}

static void style_pop(void) {
	struct Style *const pop = StyleArrayPop(&mode.styles),
		*const top = StyleArrayPeek(&mode.styles);
	assert(pop);
	/*printf("<!-- pop %s -->", pop->text->name);*/
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

/** Encode a bunch of arbitrary text `from` to `length` as whatever the options
 were.
 @param[a] If specified, prints it to the string. */
static void encode_s(int length, const char *from, char (*const a)[256]) {
	char *build = *a;
	assert(length >= 0 && from && ((a && *a) || !a));

	switch(CdocOptionsOutput()) {
	case OUT_HTML:
		if(a) goto html_encode_string;
		else goto html_encode_print;
	case OUT_MD:
		if(a) goto md_encode_string;
		else goto md_encode_print;
	default: assert(0);
	}

html_encode_string:
	while(length) {
		switch(*from) {
		case '<': if((size_t)(build - *a) >= sizeof *a - 5) goto terminate_html;
			strcpy(build, "&lt;"), build += 4; break;
		case '>': if((size_t)(build - *a) >= sizeof *a - 5) goto terminate_html;
			strcpy(build, "&gt;"), build += 4; break;
		case '&': if((size_t)(build - *a) >= sizeof *a - 6) goto terminate_html;
			strcpy(build, "&amp;"), build += 5; break;
		case '\0': goto terminate_html;
		default: if((size_t)(build - *a) >= sizeof *a - 1) goto terminate_html;
			*build++ = *from; break;
		}
		from++, length--;
	}
terminate_html:
	*build = '\0';
	return;

md_encode_string:
	while(length) {
		switch(*from) {
		case '\\': case '`': case '*': case '_': case '{': case '}': case '[':
		case ']': case '(': case ')': case '#': case '+': case '-': case '.':
		case '!':
			if((size_t)(build - *a) >= sizeof *a - 2) goto terminate_md;
			*build++ = '\\'; *build++ = *from; break;
		case '\0':
			goto terminate_md;
		default: if((size_t)(build - *a) >= sizeof *a - 1) goto terminate_md;
			*build++ = *from; break;
		}
		from++, length--;
	}
terminate_md:
	*build = '\0';
	return;	

html_encode_print:
	while(length) {
		switch(*from) {
		case '<': fputs("&lt;", stdout); break;
		case '>': fputs("&gt;", stdout); break; 
		case '&': fputs("&amp;", stdout); break;
		case '\0': fprintf(stderr, "Encoded null with %d left.\n", length);
			return;
		default: fputc(*from, stdout); break;
		}
		from++, length--;
	}
	return;

md_encode_print:
	while(length) {
		switch(*from) {
		case '\\': case '`': case '*': case '_': case '{': case '}': case '[':
		case ']': case '(': case ')': case '#': case '+': case '-': case '.':
		case '!':
			printf("\\%c", *from); break;
		case '\0': fprintf(stderr, "Encoded null with %d left.\n", length);
			return;
		default: fputc(*from, stdout); break;
		}
		from++, length--;
	}
	return;
}

static void encode(int length, const char *from) {
	encode_s(length, from, 0);
}

/* Some `OutFn` need this. */
static const struct Token *print_token(const struct TokenArray *const tokens,
	const struct Token *token);

/** Selects `token` out of `tokens` and prints it and returns the next token. */
typedef int (*OutFn)(const struct TokenArray *const tokens,
	const struct Token **const ptoken, char (*const a)[256]);
/* @param[ptoken] Is an [in/out] variable, it should get updated unless the
 return value is false.
 @return Success.
 @implements <Attribute>Predicate */
#define OUT(name) static int name(const struct TokenArray *const tokens, \
	const struct Token **ptoken, char (*const a)[256])

OUT(ws) {
	const struct Token *const space = *ptoken;
	assert(tokens && space && space->symbol == SPACE && !a);
	style_separate();
	*ptoken = TokenArrayNext(tokens, space);
	return 1;
}
OUT(par) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NEWLINE && !a);
	style_pop_level();
	CdocOptionsOutput() == OUT_HTML ? style_push(&html_p) : style_push(&md_p);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(lit) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->length > 0 && t->from);
	if(a) {
		encode_s(t->length, t->from, a);
	} else {
		style_prepare_output(t->symbol);
		encode(t->length, t->from);
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
	const enum Output o = CdocOptionsOutput();
	const char *const format =
		o == OUT_HTML ? "&lt;%.*s&gt;%.*s" : "<%.*s>%.*s";
	assert(tokens && t && t->symbol == ID_ONE_GENERIC);
	if(!lparen || lparen->symbol != LPAREN || !param || !rparen
	   || rparen->symbol != RPAREN) goto catch;
	type = t->from;
	if(!(b = strchr(type, '_'))) goto catch;
	type_size = (int)(b - type);
	assert(t->length == b + 1 - t->from);
	if(a) {
		assert(*a);
		if(t->length - 1 + param->length + strlen(o == OUT_HTML ? "&lt;&gt;"
			: "<>") + 1 >= sizeof *a) return fprintf(stderr, "%s: too long.\n",
			pos(t)), 0;
		sprintf(*a, format, t->length - 1, t->from, param->length,
			param->from);
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
	const enum Output o = CdocOptionsOutput();
	const char *const format = o == OUT_HTML ?
		"&lt;%.*s&gt;%.*s&lt;%.*s&gt;%.*s" : "<%.*s>%.*s<%.*s>%.*s";
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
	if(a) {
		assert(*a);
		if(type1_size + param1->length + type2_size + param2->length
			+ strlen(o == OUT_HTML ? "&lt;&gt;&lt;&gt;" : "<><>") + 1
			>= sizeof *a) return fprintf(stderr, "%s: too long.\n", pos(t)), 0;
		sprintf(*a, format, type1_size, type1, param1->length, param1->from,
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
	const enum Output o = CdocOptionsOutput();
	const char *const format = o == OUT_HTML ?
		"&lt;%.*s&gt;%.*s&lt;%.*s&gt;%.*s&lt;%.*s&gt;%.*s"
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
	if(a) {
		assert(*a);
		if(type1_size + param1->length + type2_size + param2->length
			+ type3_size + param3->length + strlen(o == OUT_HTML
			? "&lt;&gt;&lt;&gt;&lt;&gt;" : "<><><>") + 1 >= sizeof *a)
			return fprintf(stderr, "%s: too long.\n", pos(t)), 0;
		sprintf(*a, format, type1_size, type1, param1->length, param1->from,
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
	assert(tokens && t && t->symbol == ESCAPE && t->length == 2 && !a);
	style_prepare_output(t->symbol);
	encode(1, t->from + 1);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(url) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == URL && !a);
	style_prepare_output(t->symbol);
	if(CdocOptionsOutput() == OUT_HTML) {
		printf("<a href = \"%.*s\">", t->length, t->from);
		encode(t->length, t->from);
		printf("</a>");
	} else {
		printf("[");
		encode(t->length, t->from);
		/* fixme: What if it contains ()? */
		printf("](%.*s)", t->length, t->from);
	}
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(cite) {
	const struct Token *const t = *ptoken;
	const char *const url_encoded = UrlEncode(t->from, t->length);
	assert(tokens && t && t->symbol == CITE && !a);
	if(!url_encoded) goto catch;
	style_prepare_output(t->symbol);
	if(CdocOptionsOutput() == OUT_HTML) {
		printf("<a href = \"https://scholar.google.ca/scholar?q=%s\">",
			url_encoded);
		encode(t->length, t->from);
		printf("</a>");
	} else {
		printf("[");
		encode(t->length, t->from);
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
	assert(tokens && fn && fn->symbol == SEE_FN && !a);
	style_prepare_output(fn->symbol);
	if(CdocOptionsOutput() == OUT_HTML) {
		printf("<a href = \"#%s:", division_strings[DIV_FUNCTION]);
		encode(fn->length, fn->from);
		printf("\">");
		encode(fn->length, fn->from);
		printf("</a>");
	} else {
		printf("[");
		encode(fn->length, fn->from);
		printf("](%s:", division_strings[DIV_FUNCTION]);
		encode(fn->length, fn->from);
		printf(")");
	}
	*ptoken = TokenArrayNext(tokens, fn);
	return 1;
}
OUT(see_tag) {
	const struct Token *const tag = *ptoken;
	assert(tokens && tag && tag->symbol == SEE_TAG && !a);
	style_prepare_output(tag->symbol);
	if(CdocOptionsOutput() == OUT_HTML) {
		printf("<a href = \"#%s:", division_strings[DIV_TAG]);
		encode(tag->length, tag->from);
		printf("\">");
		encode(tag->length, tag->from);
		printf("</a>");
	} else {
		printf("[");
		encode(tag->length, tag->from);
		printf("](%s:", division_strings[DIV_TAG]);
		encode(tag->length, tag->from);
		printf(")");
	}
	*ptoken = TokenArrayNext(tokens, tag);
	return 1;
}
OUT(see_typedef) {
	const struct Token *const def = *ptoken;
	assert(tokens && def && def->symbol == SEE_TYPEDEF && !a);
	style_prepare_output(def->symbol);
	if(CdocOptionsOutput() == OUT_HTML) {
		printf("<a href = \"#%s:", division_strings[DIV_TYPEDEF]);
		encode(def->length, def->from);
		printf("\">");
		encode(def->length, def->from);
		printf("</a>");
	} else {
		printf("[");
		encode(def->length, def->from);
		printf("](%s:", division_strings[DIV_TYPEDEF]);
		encode(def->length, def->from);
		printf(")");
	}
	*ptoken = TokenArrayNext(tokens, def);
	return 1;
}
OUT(see_data) {
	const struct Token *const data = *ptoken;
	assert(tokens && data && data->symbol == SEE_DATA && !a);
	style_prepare_output(data->symbol);
	if(CdocOptionsOutput() == OUT_HTML) {
		printf("<a href = \"#%s:", division_strings[DIV_DATA]);
		encode(data->length, data->from);
		printf("\">");
		encode(data->length, data->from);
		printf("</a>");
	} else {
		printf("[");
		encode(data->length, data->from);
		printf("](%s:", division_strings[DIV_DATA]);
		encode(data->length, data->from);
		printf(")");
	}
	*ptoken = TokenArrayNext(tokens, data);
	return 1;
}
OUT(math_begin) { /* Math and code. */
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_BEGIN && !a);
	style_prepare_output(t->symbol);
	printf(CdocOptionsOutput() == OUT_HTML ? "<code>" : "`");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(math_end) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATH_END && !a);
	style_prepare_output(t->symbol);
	printf(CdocOptionsOutput() == OUT_HTML ? "</code>" : "`");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_begin) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_BEGIN && !a);
	style_prepare_output(t->symbol);
	printf(CdocOptionsOutput() == OUT_HTML ? "<em>" : "_");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(em_end) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == EM_END && !a);
	style_prepare_output(t->symbol);
	printf(CdocOptionsOutput() == OUT_HTML ? "</em>" : "_");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}


/* fixme: complete all the following. */
OUT(link) {
	const struct Token *const t = *ptoken, *text, *turl;
	const enum Output o = CdocOptionsOutput();
	assert(tokens && t && t->symbol == LINK_START && !a);
	style_prepare_output(t->symbol);
	for(turl = TokenArrayNext(tokens, t);;turl = TokenArrayNext(tokens, turl)) {
		if(!turl) goto catch;
		if(turl->symbol == URL) break;
	}
	if(o == OUT_HTML) printf("<a href = \"%.*s\">", turl->length, turl->from);
	else printf("[");
	for(text = TokenArrayNext(tokens, t); text->symbol != URL; )
		if(!(text = print_token(tokens, text))) goto catch;
	if(o == OUT_HTML) printf("</a>");
	else printf("](%.*s)", turl->length, turl->from);
	*ptoken = TokenArrayNext(tokens, turl);
	return 1;
catch:
	fprintf(stderr, "%s: expected `[description](url)`.\n", pos(t));
	return 0;
}
OUT(image) {
	const struct Token *const t = *ptoken, *text, *turl;
	const enum Output o = CdocOptionsOutput();
	assert(tokens && t && t->symbol == IMAGE_START && !a);
	style_prepare_output(t->symbol);
	for(turl = TokenArrayNext(tokens, t);;turl = TokenArrayNext(tokens, turl)) {
		if(!turl) goto catch;
		if(turl->symbol == URL) break;
	}
	if(o == OUT_HTML)
		printf("<img src = \"%.*s\" alt = \"", turl->length, turl->from);
	else
		printf("![");
	for(text = TokenArrayNext(tokens, t); text->symbol != URL; )
		if(!(text = print_token(tokens, text))) goto catch;
	if(o == OUT_HTML) {
		unsigned width, height;
		char fn[256];
		printf("\"");
		if((size_t)turl->length >= sizeof fn) { fprintf(stderr,
			"%s: path is too big %d to find dimensions.\n", pos(t),
			turl->length); goto dimensionless; }
		strncpy(fn, turl->from, turl->length);
		fn[turl->length] = '\0';
		/* Detailed exceptions inside; it's really a warning, skip it. */
		if(!ImageDimension(fn, &width, &height))
			{ errno = 0; goto dimensionless; }
		printf(" width = %u height = %u", width, height);
dimensionless:
		printf(">");
	} else {
		printf("](%.*s)", turl->length, turl->from);
	}
	*ptoken = TokenArrayNext(tokens, turl);
	return 1;
catch:
	fprintf(stderr, "%s: expected `[description](url)`.\n", pos(t));
	return 0;
}
OUT(nbsp) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NBSP && !a);
	style_prepare_output(t->symbol);
	printf("&nbsp;");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(nbthinsp) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NBTHINSP && !a);
	style_prepare_output(t->symbol);
	printf("&#8239;" /* "&thinsp;" <- breaking? */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(mathcalo) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == MATHCALO && !a);
	style_prepare_output(t->symbol);
	/* Omicron. It looks like a stylised "O"? The actual is "&#120030;" but
	 good luck finding a font that supports that. If one was using JavaScript
	 and had a constant connection, we could use MathJax. */
	printf("&#927" /* "O" */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(ctheta) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == CTHETA && !a);
	style_prepare_output(t->symbol);
	printf("&#920;" /* "&Theta;" This is supported on more browsers. */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(comega) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == COMEGA && !a);
	style_prepare_output(t->symbol);
	printf("&#937;" /* "&Omega;" */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(times) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == TIMES && !a);
	style_prepare_output(t->symbol);
	printf("&#215;");
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(cdot) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == CDOT && !a);
	style_prepare_output(t->symbol);
	printf("&#183;" /* &middot; */);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(list) {
	const struct StyleText *const peek = style_text_peek();
	const struct Token *const t = *ptoken;
	const enum Output o = CdocOptionsOutput();
	const struct StyleText *const li = o == OUT_HTML ? &html_li : &md_li,
		*const ul = o == OUT_HTML ? &html_ul : &md_ul;
	assert(tokens && t && t->symbol == LIST_ITEM && peek && !a);
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
	const enum Output o = CdocOptionsOutput();
	const struct StyleText *const line = o == OUT_HTML ? &html_pre_line
		: &md_pre_line, *const pref = o == OUT_HTML ? &html_pre : &md_pre;
	assert(tokens && t && t->symbol == PREFORMATTED && peek && !a);
	if(peek != line) {
		style_pop_level();
		style_push(pref), style_push(line);
	}
	style_prepare_output(t->symbol);
	encode(t->length, t->from);
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
 @return The next token. */
static const struct Token *print_token_s(const struct TokenArray *const tokens,
	const struct Token *token, char (*const a)[256]) {
	const OutFn sym_out = symbol_outs[token->symbol];
	assert(tokens && token);
	if(!sym_out) return fprintf(stderr, "%s: symbol output undefined.\n",
		pos(token)), TokenArrayNext(tokens, token);
	if(!sym_out(tokens, &token, a)) { errno = EILSEQ; return 0; }
	return token;
}

static const struct Token *print_token(const struct TokenArray *const tokens,
	const struct Token *token) {
	return print_token_s(tokens, token, 0);
}

/** @param[highlights] Must be sorted if not null, creates an emphasis on those
 words.
 @throws[EILSEQ] Sequence error. Must detect with `errno`. */
static void highlight_tokens(const struct TokenArray *const tokens,
	const struct IndexArray *const highlights) {
	const struct Token *const first = TokenArrayNext(tokens, 0), *token = first;
	size_t *highlight = IndexArrayNext(highlights, 0);
	int is_highlight, is_first_highlight = 1;
	assert(tokens);
	if(!token) return;
	while(token) {
		if(highlight && *highlight == (size_t)(token - first)) {
			is_highlight = 1;
			highlight = IndexArrayNext(highlights, highlight);
		} else {
			is_highlight = 0;
		}
		if(is_highlight) printf(is_first_highlight ? "<strong>" : "<em>");
		token = print_token(tokens, token);
		if(is_highlight) printf(is_first_highlight ? "</strong>" : "</em>"),
			is_first_highlight = 0;
	}
}

static void print_tokens(const struct TokenArray *const tokens) {
	highlight_tokens(tokens, 0);
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
	assert(segment && attribute && style);
	if((match && !segment_attribute_match_exists(segment, attribute, match))
		|| (!match && !segment_attribute_exists(segment, attribute))) return;
	style_push(&html_dt), style_push(&plain_text);
	style_prepare_output(END);
	printf("%s:", symbol_attribute_titles[attribute]);
	if(match) style_separate(), style_push(&html_em),
		print_token(&segment->code, match), style_pop();
	style_pop(), style_pop();
	style_push(&html_dd), style_push(style), style_push(&plain_text);
	segment_att_print_all(segment, attribute, match, SHOW_TEXT);
	style_pop(), style_pop(), style_pop();
}

/** This is used in preamble for attributes inside a `dl`.
 @param[is_recursive]  */
static void dl_preamble_att(const enum Symbol attribute,
	const enum AttShow show) {
	sprintf(title, "\t<dt>%.128s:</dt>\n"
		"\t<dd>", symbol_attribute_titles[attribute]);
	style_push(&html_desc), style_push(&plain_csv), style_push(&plain_text);
	div_att_print(&is_div_preamble, attribute, SHOW_TEXT);
	style_pop(), style_push(&plain_parenthetic), style_push(&plain_csv),
	style_push(&plain_text);
	div_att_print(&is_not_div_preamble, attribute, show);
	style_pop(), style_pop(), style_pop(), style_pop(), style_pop();
}

static void dl_segment_specific_att(const struct Attribute *const attribute) {
	assert(attribute);
	style_push(&html_dt), style_push(&plain_text);
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
	style_push(&html_dd), style_push(&plain_text);
	print_tokens(&attribute->contents);
	style_pop(), style_pop();
}


/** Prints all a `segment`.
 @implements division_act */
static void segment_print_all(const struct Segment *const segment) {
	const struct Token *param;
	assert(segment && segment->division != DIV_PREAMBLE);
	style_push(&html_div);

	/* The title is generally the first param. Only single-words. */
	if((param = param_no(segment, 0))) {
		style_prepare_output(END);
		printf("<a name = \"%s:", division_strings[segment->division]);
		print_token(&segment->code, param);
		printf("\"><!-- --></a>\n");
		style_push(&html_h3);
		print_token(&segment->code, param);
		style_pop_level();
		style_push(&html_p), style_push(&html_code);
		highlight_tokens(&segment->code, &segment->code_params);
		style_pop_level();
	} else {
		style_push(&html_h3);
		style_prepare_output(END);
		printf("Unknown");
		style_pop_level();
	}

	/* Now text. */
	style_push(&html_p);
	print_tokens(&segment->doc);
	style_pop_level();

	/* Attrubutes. */
	style_push(&html_dl);
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
			style_push(&html_em);
			style_prepare_output(END);
			printf("function");
			style_pop();
			return;
		}
		code = print_token(&segment->code, code);
	}
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
	div_att_print(&is_div_preamble, ATT_TITLE, SHOW_TEXT);
	style_pop_level();
	assert(!StyleArraySize(&mode.styles));
	printf("</head>\n\n"
		"<body>\n\n");

	/* Title. */
	style_push(&html_h1), style_push(&plain_ssv), style_push(&plain_text);
	div_att_print(&is_div_preamble, ATT_TITLE, SHOW_TEXT);
	style_pop_level();
	assert(!StyleArraySize(&mode.styles));

	/* TOC. */
	style_push(&html_ul), style_push(&html_li);
	if(is_preamble) {
		style_prepare_output(END);
		printf("<a href = \"#%s:\">Preamble</a>",
			division_strings[DIV_PREAMBLE]);
		style_pop_push();
	}
	if(is_typedef) {
		style_prepare_output(END);
		printf("<a href = \"#%s:\">Typedef Aliases</a>: ",
			division_strings[DIV_TYPEDEF]);
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
		style_prepare_output(END);
		printf("<a href = \"#%s:\">Struct, Union, and Enum Definitions</a>: ",
			division_strings[DIV_TAG]);
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
			/* fixme: tag type? */
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
		style_prepare_output(END);
		printf("<a href = \"#%s:\">General Declarations</a>: ",
			division_strings[DIV_DATA]);
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
		style_prepare_output(END);
		printf("<a href = \"#summary:\">Function Summary</a>");
		style_pop_push();
		style_prepare_output(END);
		printf("<a href = \"#%s:\">Function Definitions</a>",
			division_strings[DIV_FUNCTION]);
		style_pop_push();
	}
	if(is_license) style_prepare_output(END),
		printf("<a href = \"#license:\">License</a>"), style_pop_push();
	style_pop_level();
	assert(!StyleArraySize(&mode.styles));

	/* Preamble contents. */
	if(is_preamble) {
		style_push(&html_div);
		style_prepare_output(END);
		printf("<a name = \"%s:\"><!-- --></a>\n"
			   "<h2>Preamble</h2>\n\n",
			   division_strings[DIV_PREAMBLE]);
		while((segment = SegmentArrayNext(&report, segment))) {
			if(segment->division != DIV_PREAMBLE) continue;
			style_push(&html_p);
			print_tokens(&segment->doc);
			style_pop_level();
		}
		style_push(&html_dl);
		/* `ATT_TITLE` is above. */
		while((segment = SegmentArrayNext(&report, segment))) {
			const struct Attribute *att = 0;
			if(segment->division != DIV_PREAMBLE) continue;
			while((att = AttributeArrayNext(&segment->attributes, att))) {
				if(att->token.symbol != ATT_PARAM) continue;
				dl_segment_specific_att(att);
			}
		}
		dl_preamble_att(ATT_AUTHOR, SHOW_ALL);
		dl_preamble_att(ATT_STD, SHOW_ALL);
		dl_preamble_att(ATT_DEPEND, SHOW_ALL);
		dl_preamble_att(ATT_FIXME, SHOW_WHERE);
		/* `ATT_RETURN`, `ATT_THROWS`, `ATT_IMPLEMENTS`, `ATT_ORDER`,
		 `ATT_ALLOW` have warnings. `ATT_LICENSE` is below. */
		style_pop_level();
		style_pop_level();
	}
	assert(!StyleArraySize(&mode.styles));

	/* Print typedefs. */
	if(is_typedef) {
		style_push(&html_div);
		style_prepare_output(END);
		printf("<a name = \"%s:\"><!-- --></a>"
			"<h2>Typedef Aliases</h2>\n\n", division_strings[DIV_TYPEDEF]);
		division_act(DIV_TYPEDEF, &segment_print_all);
		style_pop_level();
	}
	/* Print tags. */
	if(is_tag) {
		style_push(&html_div);
		style_prepare_output(END);
		printf("<a name = \"%s:\"><!-- --></a>"
			"<h2>Struct, Union, and Enum Definitions</h2>\n\n",
			division_strings[DIV_TAG]);
		division_act(DIV_TAG, &segment_print_all);
		style_pop_level();
	}
	/* Print general declarations. */
	if(is_data) {
		style_push(&html_div);
		style_prepare_output(END);
		printf("<a name = \"%s:\"><!-- --></a>"
			"<h2>General Definitions</h2>\n\n", division_strings[DIV_DATA]);
		division_act(DIV_DATA, &segment_print_all);
		style_pop_level();
	}
	/* Print functions. */
	if(is_function) {
		/* Function table. */
		style_push(&html_div);
		style_prepare_output(END);
		printf("<a name = \"summary:\"><!-- --></a>"
			"<h2>Function Summary</h2>\n\n");
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
		style_push(&html_div);
		style_prepare_output(END);
		printf("<a name = \"%s:\"><!-- --></a>"
			"<h2>Function Definitions</h2>\n\n",
			division_strings[DIV_FUNCTION]);
		division_act(DIV_FUNCTION, &segment_print_all);
		style_pop_level();
	}
	/* License. */
	if(is_license) {
		style_push(&html_div);
		style_prepare_output(END);
		printf("<a name = \"license:\"><!-- --></a>\n"
			"<h2>License</h2>\n\n");
		style_push(&html_p);
		div_att_print(&is_div_preamble, ATT_LICENSE, SHOW_TEXT);
		style_pop_push();
		style_push(&plain_see_license), style_push(&plain_text);
		/* fixme: if a segment has multiple licenses, they will show multiple
		 times. */
		div_att_print(&is_not_div_preamble, ATT_LICENSE, SHOW_WHERE);
		style_pop_level();
		style_pop_level();
	}

	printf("</body>\n\n"
		"</html>\n");
	style_clear();
	return errno ? 0 : 1;
}
