/* This should really go in it's own C file. */

/* `SYMBOL` is declared in `Scanner.h`. */
static const int symbol_before_sep[] = { SYMBOL(PARAM6D) };
static const int symbol_after_sep[]  = { SYMBOL(PARAM6E) };

#define HTML_AMP "&amp;"
#define HTML_GT  "&gt;"
#define HTML_LT  "&lt;"

/* Hack `sprintf` titles. */
static char style_title[256];

/* Every `StyleText` can have a beginning, a separator, and an end, which will
 be printed around literals. Block and can appear alone elements have
 `is_next_level`. */
static const struct StyleText {
	const char *name, *begin, *sep, *end;
	int is_next_level, is_to;
	const enum Format to_format;
} no_style = { "nosty", "", "", "", 0, 0, 0 },
	plain_text = { "plain", "", " ", "", 0, 0, 0 },
	plain_parenthetic = { "paren", "(", " ", ")", 0, 0, 0 },
	plain_see_license = { "license", "(See license details ", ", ", ".)",
	0, 0, 0 },
	plain_csv = { "csv", "", ", ", "", 0, 0, 0 },
	plain_ssv = { "ssv", "", "; ", "", 0, 0, 0 },
	to_raw = { "to_raw", "", "", "", 0, 1, OUT_RAW },
	to_html = { "to_html", "", "", "", 0, 1, OUT_HTML },
	html_title = { "title", "<title>", "", "</title>\n", 1, 0, 0 },
	styles[][3] = {
	{ { "raw_div", "", "", "", 1, 0, 0 },
		{ "html_div", "<div>", "", "</div>\n\n", 1, 0, 0 },
		{ "md_div", "", "", "\n\n", 1, 0, 0 } },
	{ { "raw_p", "", " ", "", 1, 0, 0 },
		{ "html_p", "<p>", " ", "</p>\n\n", 1, 0, 0 },
		{ "md_p", "", " ", "\n\n", 1, 0, 0 } },
	{ { "raw_ul", "", "", "", 1, 0, 0 },
		{ "html_ul", "<ul>\n", "", "</ul>\n\n", 1, 0, 0 },
		{ "md_ul", "", "",  "\n", 1, 0, 0 } },
	{ { "raw_li", "", " ", "", 0, 0, 0 },
		{ "html_li", "\t<li>", " ", "</li>\n", 0, 0, 0 },
		{ "md_li", " * ", " ", "\n", 0, 0, 0 } },
	{ { "raw_code", "", " ", "", 0, 0, 0 },
		{ "html_code", "<code>", /*"&nbsp;"*/" ", "</code>", 0, 0, 0 },
		{ "md_code", "`", " ", "`", 0, 1, OUT_RAW } },
	{ { "raw_pre", "", "", "", 1, 0, 0 },
		{ "html_pre", "<pre>\n", "", "</pre>\n\n", 1, 0, 0 },
		{ "md_pre", "", "", "\n", 1, 1, OUT_HTML } },
	{ { "raw_pretext", "", "\n", "", 0, 0, 0 },
		{ "html_pretext", "", "\n", "\n", 0, 0, 0 },
		{ "md_pretext", "", "\n    ", "\n", 0, 0, 0 } },
	{ { "raw_h1", "", "", "", 1, 0, 0 },
		{ "html_h1", "<h1>", "", "</h1>\n\n", 1, 0, 0 },
		{ "md_h1", " # ", "", " #\n\n", 1, 0, 0 } },
	{ { "raw_h2", "", "", "", 1, 0, 0 },
		{ "html_h2", "<h2>", "", "</h2>\n\n", 1, 0, 0 },
		{ "md_h2", " ## ", "", " ##\n\n", 1, 0, 0 } },
	{ { "raw_h3", "", "", "", 1, 0, 0 },
		{ "html_h3", "<h3>", "", "</h3>\n\n", 1, 0, 0 },
		{ "md_h3", " ### ", "", " ###\n\n", 1, 0, 0 } },
	{ { "raw_dl", "", "", "", 1, 0, 0 },
		{ "html_dl", "<dl>\n", "", "</dl>\n\n", 1, 0, 0 },
		{ "md_dl", "", "", "\n\n", 1, 0, 0 } },
	{ { "raw_dt", "", "", "", 0, 0, 0 },
		{ "html_dt", "\t<dt>", "", "</dt>\n", 0, 0, 0 },
		{ "md_dt", " - ", "", "  \n", 0, 0, 0 } },
	{ { "raw_dd", "", "", "", 0, 0, 0 },
		{ "html_dd", "\t<dd>", "", "</dd>\n", 0, 0, 0 },
		{ "md_dd", "   ", "", "\n", 0, 0, 0 } },
	/* This is a mess. */
	{ { "raw_ddtitle", style_title, "", "", 0, 0, 0 },
		{ "html_ddtitle", style_title, "", "</dd>\n", 0, 0, 0 },
		{ "md_ddtitle", style_title, "", "\n", 0, 0, 0 } },
	{ { "raw_em", "", "", "", 0, 0, 0 },
		{ "html_em", "<em>", "", "</em>", 0, 0, 0 },
		{ "md_em", "_", "", "_", 0, 0, 0 } },
	{ { "raw_strong", "", "", "", 0, 0, 0 },
		{ "html_strong", "<strong>", "", "</strong>", 0, 0, 0 },
		{ "md_strong", "*", "", "*", 0, 0, 0 } },
	{ { "raw_html_em", "", "", "", 0, 0, 0 },
		{ "html_html_em", "<em>", "", "</em>", 0, 0, 0 },
		{ "md_html_em", "<em>", "", "</em>", 0, 0, 0 } },
	{ { "raw_html_strong", "", "", "", 0, 0, 0 },
		{ "html_html_strong", "<strong>", "", "</strong>", 0, 0, 0 },
		{ "md_html_strong", "<strong>", "", "</strong>", 0, 0, 0 } }
	};

/* This is a hack. Don't change the styles without changing this. */
enum { ST_DIV, ST_P, ST_UL, ST_LI, ST_CODE, ST_PRE, ST_PRELINE,
	ST_H1, ST_H2, ST_H3, ST_DL, ST_DT, ST_DD, ST_DESC, ST_EM, ST_STRONG,
	ST_EM_HTML, ST_STRONG_HTML };

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

static struct {
	const struct StyleText *style;
	int on;
} style_highlight;

static void unrecoverable(void) { perror("Unrecoverable"), exit(EXIT_FAILURE); }

static void style_clear(void) {
	assert(!StyleArraySize(&mode.styles) && !style_highlight.on);
	StyleArray_(&mode.styles);
	mode.is_before_sep = 0;
}

static void style_push(const struct StyleText *const text) {
	struct Style *const push = StyleArrayNew(&mode.styles);
	assert(text);
	/* There's so many void functions that rely on this function and it's such
	 a small amount of memory, that it's useless to recover. The OS will have
	 to clean up our mess. */
	if(!push) { unrecoverable(); return; }
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
	/* Now do the highlight. */
	if(style_highlight.style && !style_highlight.on)
		fputs(style_highlight.style->begin, stdout), style_highlight.on = 1;
}

/** If we want to print a string. */
static void style_string_output(void) {
	style_prepare_output(END);
}

static void style_highlight_on(const struct StyleText *const style) {
	assert(!style_highlight.style && !style_highlight.on);
	style_highlight.style = style;
}

static void style_highlight_off(void) {
	assert(style_highlight.style);
	if(style_highlight.on)
		fputs(style_highlight.style->end, stdout), style_highlight.on = 0;
	style_highlight.style = 0;
}

/** Differed space. */
static void style_separate(void) {
	struct Style *const top = StyleArrayPeek(&mode.styles);
	assert(top);
	if(top->lazy == ITEM) top->lazy = SEPARATE;
}

/** With Markdown, the effective style is sometimes HTML. */
static int effective_format_pop(const int will_be_popped) {
	struct Style *style = 0;
	const enum Format f = CdocGetFormat();
	/* If the style will be popped, don't include it. */
	if(will_be_popped) style = StyleArrayBack(&mode.styles, style);
	while((style = StyleArrayBack(&mode.styles, style)))
		if(style->text->is_to) return /*fprintf(stderr, "%s suppresses escapes.\n", StyleArrayToString(&mode.styles)),*/ style->text->to_format;
	return f;
}

static int effective_format(void) { return effective_format_pop(0); }

static int effective_format_will_be_popped(void) {
	return effective_format_pop(1);
}

/** Encode a bunch of arbitrary text `from` to `length` as whatever the options
 were.
 @param[is_buffer] Appends to the buffer chosen in `Buffer.c`. */
static void encode_len_choose(int length, const char *from,
	const enum Format f, const int is_buffer) {
	int ahead = 0;
	char *b;
	const char *str;
	size_t str_len;
	assert(length >= 0 && from);

	switch(f) {
	case OUT_HTML:
		if(is_buffer) goto html_encode_buffer;
		else goto html_encode_print;
	case OUT_MD:
		if(is_buffer) goto md_encode_buffer;
		else goto md_encode_print;
	case OUT_RAW:
		if(!is_buffer) { fprintf(stderr,
			"%.*s: doesn't make sense to output is raw.\n", length, from);
			return; }
		goto raw_encode_buffer;
	}

raw_encode_buffer:
	if(!(b = BufferPrepare(length))) { unrecoverable(); return; }
	memcpy(b, from, length);
	return;

html_encode_buffer:
	while(length - ahead) {
		switch(from[ahead]) {
		case '<': str = HTML_LT, str_len = strlen(str); break;
		case '>': str = HTML_GT, str_len = strlen(str); break;
		case '&': str = HTML_AMP, str_len = strlen(str); break;
		case '\0': goto terminate_html;
		default: ahead++; continue;
		}
		if(ahead) {
			if(!(b = BufferPrepare(ahead))) { unrecoverable(); return; }
			memcpy(b, from, ahead);
			length -= ahead;
			from += ahead;
			ahead = 0;
		}
		if(!(b = BufferPrepare(str_len))) { unrecoverable(); return; }
		memcpy(b, str, str_len);
		from++, length--;
	}
terminate_html:
	if(ahead) {
		if(!(b = BufferPrepare(ahead))) { unrecoverable(); return; }
		memcpy(b, from, ahead);
	}
	return;

md_encode_buffer:
	while(length - ahead) {
		const char *escape = 0;
		switch(from[ahead]) {
		case '\0': goto terminate_md;
		case '\\': case '`': case '*': case '_': case '{': case '}': case '[':
		case ']': case '(': case ')': case '#': case '+': case '-': case '.':
		case '!': break;
		default: ahead++; continue;
		}
		if(ahead) {
			if(!(b = BufferPrepare(ahead))) { unrecoverable(); return; }
			memcpy(b, from, ahead);
			length -= ahead;
			from += ahead;
			ahead = 0;
		}
		if(escape) {
			const size_t e_len = strlen(escape);
			if(!(b = BufferPrepare(e_len))) { unrecoverable(); return; }
			memcpy(b, escape, e_len);
		} else {
			if(!(b = BufferPrepare(2))) { unrecoverable(); return; }
			b[0] = '\\', b[1] = *from;
		}
		from++, length--;
	}
terminate_md:
	if(ahead) {
		if(!(b = BufferPrepare(ahead))) { unrecoverable(); return; }
		memcpy(b, from, ahead);
	}
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
	while(length) {
		switch(*from) {
		case '\0': fprintf(stderr, "Encoded null with %d left.\n", length);
			return;
		case '\\': case '`': case '*': case '_': case '{': case '}': case '[':
		case ']': case '(': case ')': case '#': case '+': case '-': case '.':
		case '!': printf("\\%c", *from); break;
		default: fputc(*from, stdout); break;
		}
		from++, length--;
	}
	return;
}

static const char *encode_len_s_raw(const int length, const char *const from) {
	BufferClear();
	encode_len_choose(length, from, OUT_RAW, 1);
	return BufferGet();
}

static void encode_cat_len_s(const int length, const char *const from) {
	encode_len_choose(length, from, effective_format(), 1);
}

static void encode_len(const int length, const char *const from) {
	encode_len_choose(length, from, effective_format(), 0);
}

static void encode(const char *const str) {
	size_t length = strlen(str);
	if(length > INT_MAX) { fprintf(stderr,
		"Encode: \"%.10s...\" length is greater then the maximum integer; "
		"clipped at %d.\n", str, INT_MAX);
		length = INT_MAX; }
	encode_len((int)length, str);
}
