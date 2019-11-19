/* `SYMBOL` is declared in `Scanner.h`. */
static const int symbol_before_sep[] = { SYMBOL(PARAM6D) };
static const int symbol_after_sep[]  = { SYMBOL(PARAM6E) };

#define HTML_AMP "&amp;"
#define HTML_GT  "&gt;"
#define HTML_LT  "&lt;"

/* Hack `sprintf` titles. */
static char title[256];

/* Every `StyleText` can have a beginning, a separator, and an end, which will
 be printed around literals. Block and can appear alone elements have
 `is_next_level`. */
static const struct StyleText {
	const char *name, *begin, *sep, *end;
	int is_next_level, is_suppress_escapes;
} no_style = { "nosty", "", "", "", 0, 0 },
plain_text = { "plain", "", " ", "", 0, 0 },
plain_parenthetic = { "paren", "(", " ", ")", 0, 0 },
plain_see_license = { "license", "(See license details ", ", ", ".)", 0, 0 },
plain_csv = { "csv", "", ", ", "", 0, 0 },
plain_ssv = { "ssv", "", "; ", "", 0, 0 },
html_title = { "title", "<title>", "", "</title>\n", 1, 0 },
styles[][3] = {
	{ { "", 0, 0, 0, 0, 0 },
		{ "div", "<div>", "", "</div>\n\n", 1, 0 },
		{ "div", "", "", "\n\n", 1, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "p", "<p>", " ", "</p>\n\n", 1, 0 },
		{ "p", "", " ", "\n\n", 1, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "ul", "<ul>\n", "", "</ul>\n\n", 1, 0 },
		{ "ul", "", "",  "\n", 1, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "li", "\t<li>", " ", "</li>\n", 0, 0 },
		{ "li", " * ", " ", "\n", 0, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "code", "<code>", /*"&nbsp;"*/" ", "</code>", 0, 0 },
		{ "code", "`", " ", "`", 0, 1 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "pre", "<pre>\n", "", "</pre>\n\n", 1, 0 },
		{ "pre", "", "", "\n", 1, 1 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "pretext", "", "\n", "\n", 0, 0 },
		{ "pretext", "", "\n    ", "\n", 0, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "h1", "<h1>", "", "</h1>\n\n", 1, 0 },
		{ "h1", " # ", "", " #\n\n", 1, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "h2", "<h2>", "", "</h2>\n\n", 1, 0 },
		{ "h2", " ## ", "", " ##\n\n", 1, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "h3", "<h3>", "", "</h3>\n\n", 1, 0 },
		{ "h3", " ### ", "", " ###\n\n", 1, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "dl", "<dl>\n", "", "</dl>\n\n", 1, 0 },
		{ "dl", "", "", "\n\n", 1, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "dt", "\t<dt>", "", "</dt>\n", 0, 0 },
		{ "dt", " - ", "", "  \n", 0, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "dd", "\t<dd>", "", "</dd>\n", 0, 0 },
		{ "dd", "   ", "", "\n", 0, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "ddtitle", title, "", "</dd>\n", 0, 0 },
		{ "ddtitle", title, "", "\n", 0, 0 } },
	{ { "", 0, 0, 0, 0, 0 },
		{ "em", "<em>", "", "</em>", 0, 0 },
		{ "em", "_", "", "_", 0, 0 } }
};

/* This is a hack. Don't change the styles without changing this. */
enum { ST_DIV, ST_P, ST_UL, ST_LI, ST_CODE, ST_PRE, ST_PRELINE,
	ST_H1, ST_H2, ST_H3, ST_DL, ST_DT, ST_DD, ST_DESC, ST_EM };

/* This does a delayed lazy unencoded surrounding text. Popping a `Style` and
 pushing another one, then printing, will cause a end-separator-start-print.
 Alternately, by explicitly `SPACE` or having `symbol_before_sep` and
 `symbol_after_sep` both true. Eg, say the stack is: { h1, p -> } and we have
 not printed anything, it will be empty. Then we print "foo", it will output
 "<h1><p>foo", then "bar", "<h1><p>foobar", pop "<h1><p>foobar</p>\n\n", pop
 "<h1><p>foobar</p>\n\n</h1>\n\n". */
struct Style {
	const struct StyleText *text;
	enum { BEGIN, ITEM, SEPARATE } lazy;
};

static void style_text_to_string(const struct StyleText *st,
	char (*const a)[12]) {
	strcpy(*a, st->name);
}

static void style_to_string(const struct Style *s, char (*const a)[12]) {
	style_text_to_string(s->text, a);
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

/** Only used with md. */
static int style_suppress_escapes(void) {
	struct Style *style = 0;
	/*fprintf(stderr, "%s.\n", StyleArrayToString(&mode.styles));*/
	while((style = StyleArrayNext(&mode.styles, style))) {
		if(style->text->is_suppress_escapes) return 1;
	}
	return 0;
}

/** Encode a bunch of arbitrary text `from` to `length` as whatever the options
 were.
 @param[a] If specified, prints it to the string. */
static void encode_s(int length, const char *from, char (*const a)[256]) {
	char *build = *a;
	int suppress;
	assert(length >= 0 && from && ((a && *a) || !a));

	switch(CdocGetFormat()) {
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
			strcpy(build, HTML_LT), build += strlen(HTML_LT); break;
		case '>': if((size_t)(build - *a) >= sizeof *a - 5) goto terminate_html;
			strcpy(build, HTML_GT), build += strlen(HTML_GT); break;
		case '&': if((size_t)(build - *a) >= sizeof *a - 6) goto terminate_html;
			strcpy(build, HTML_AMP), build += strlen(HTML_AMP); break;
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
	suppress = style_suppress_escapes();
	while(length) {
		switch(*from) {
		case '\0':
			goto terminate_md;
		case '\\': case '`': case '*': case '_': case '{': case '}': case '[':
		case ']': case '(': case ')': case '#': case '+': case '-': case '.':
		case '!':
			if(!suppress) {
				if((size_t)(build - *a) >= sizeof *a - 2) goto terminate_md;
				*build++ = '\\'; *build++ = *from; break;
			}
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
		case '<': fputs(HTML_LT, stdout); break;
		case '>': fputs(HTML_GT, stdout); break; 
		case '&': fputs(HTML_AMP, stdout); break;
		case '\0': fprintf(stderr, "Encoded null with %d left.\n", length);
			return;
		default: fputc(*from, stdout); break;
		}
		from++, length--;
	}
	return;
	
md_encode_print:
	suppress = style_suppress_escapes();
	while(length) {
		switch(*from) {
		case '\0': fprintf(stderr, "Encoded null with %d left.\n", length);
			return;
		case '\\': case '`': case '*': case '_': case '{': case '}': case '[':
		case ']': case '(': case ')': case '#': case '+': case '-': case '.':
		case '!':
			if(!suppress) { printf("\\%c", *from); break; }
		default: fputc(*from, stdout); break;
		}
		from++, length--;
	}
	return;
}

static void encode(int length, const char *from) {
	encode_s(length, from, 0);
}
