/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).
 
 Outputs between tokens based on a hierarchical lazy state; one should call
 this before every token group thing. It also encodes things. Is is really
 strict. */

#include <string.h> /* strlen memcpy */
#include <stdio.h>  /* fprintf */
#include <limits.h> /* INT_MAX */
#include "Cdoc.h"
#include "Symbol.h"
#include "Buffer.h"
#include "Style.h" /** \include */

/* `SYMBOL` is declared in `Symbol.h`. */
static const int symbol_before_sep[] = { SYMBOL(PARAM6D) };
static const int symbol_after_sep[]  = { SYMBOL(PARAM6E) };

/* Can have a beginning, a separator, and an end, which will be printed around
 literals. Must match the <tag:StylePunctuate>. */
static const struct Punctuate {
	const char *name, *begin, *sep, *end;
	int is_strong, is_to;
	const enum Format to_format;
} punctuates[][3] = {
	{
		{ "nosty", "", "", "", 0, 0, 0 },
		{ "nosty", "", "", "", 0, 0, 0 },
		{ "nosty", "", "", "", 0, 0, 0 }
	}, {
		{ "plain", "", " ", "", 0, 0, 0 },
		{ "plain", "", " ", "", 0, 0, 0 },
		{ "plain", "", " ", "", 0, 0, 0 }
	}, {
		{ "paren", "(", " ", ")", 0, 0, 0 },
		{ "paren", "(", " ", ")", 0, 0, 0 },
		{ "paren", "(", " ", ")", 0, 0, 0 }
	}, {
		{ "license", "(See license details ", ", ", ".)", 0, 0, 0 },
		{ "license", "(See license details ", ", ", ".)", 0, 0, 0 },
		{ "license", "(See license details ", ", ", ".)", 0, 0, 0 }
	}, {
		{ "csv", "", ", ", "", 0, 0, 0 },
		{ "csv", "", ", ", "", 0, 0, 0 },
		{ "csv", "", ", ", "", 0, 0, 0 }
	}, {
		{ "ssv", "", "; ", "", 0, 0, 0 },
		{ "ssv", "", "; ", "", 0, 0, 0 },
		{ "ssv", "", "; ", "", 0, 0, 0 }
	}, {
		{ "to_raw", "", "", "", 0, 1, OUT_RAW },
		{ "to_raw", "", "", "", 0, 1, OUT_RAW },
		{ "to_raw", "", "", "", 0, 1, OUT_RAW }
	}, {
		{ "to_html", "", "", "", 0, 1, OUT_HTML },
		{ "to_html", "", "", "", 0, 1, OUT_HTML },
		{ "to_html", "", "", "", 0, 1, OUT_HTML }
	}, {
		{ "title", "<title>", "", "</title>\n", 1, 0, 0 },
		{ "title", "<title>", "", "</title>\n", 1, 0, 0 },
		{ "title", "<title>", "", "</title>\n", 1, 0, 0 }
	}, {
		{ "raw_div", "", "", "", 1, 0, 0 },
		{ "html_div", "<div>", "", "</div>\n\n", 1, 0, 0 },
		{ "md_div", "", "", "\n\n", 1, 0, 0 }
	}, {
		{ "raw_p", "", " ", "", 1, 0, 0 },
		{ "html_p", "<p>", " ", "</p>\n\n", 1, 0, 0 },
		{ "md_p", "", " ", "\n\n", 1, 0, 0 }
	}, {
		{ "raw_ul", "", "", "", 1, 0, 0 },
		{ "html_ul", "<ul>\n", "", "</ul>\n\n", 1, 0, 0 },
		{ "md_ul", "", "",  "\n", 1, 0, 0 }
	}, {
		{ "raw_li", "", " ", "", 0, 0, 0 },
		{ "html_li", "\t<li>", " ", "</li>\n", 0, 0, 0 },
		{ "md_li", " * ", " ", "\n", 0, 0, 0 }
	}, {
		{ "raw_code", "", " ", "", 0, 0, 0 },
		{ "html_code", "<code>", " ", "</code>", 0, 0, 0 },
		{ "md_code", "`", " ", "`", 0, 1, OUT_RAW /* Really? */ }
	}, {
		{ "raw_pre", "", "", "", 1, 0, 0 },
		{ "html_pre", "<pre>\n", "", "</pre>\n\n", 1, 0, 0 },
		{ "md_pre", "", "", "\n", 1, 1, OUT_HTML }
	}, {
		{ "raw_preline", "", "\n", "", 0, 0, 0 },
		{ "html_preline", "", "\n", "\n", 0, 0, 0 },
		{ "md_preline", "", "\n    ", "\n", 0, 0, 0 }
	}, {
		{ "raw_h1", "", "", "", 1, 0, 0 },
		{ "html_h1", "<h1>", "", "</h1>\n\n", 1, 0, 0 },
		{ "md_h1", "# ", "", " #\n\n", 1, 0, 0 }
	}, {
		{ "raw_h2", "", "", "", 1, 0, 0 },
		{ "html_h2", "<h2>", "", "</h2>\n\n", 1, 0, 0 },
		{ "md_h2", "## ", "", " ##\n\n", 1, 0, 0 }
	}, {
		{ "raw_h3", "", "", "", 1, 0, 0 },
		{ "html_h3", "<h3>", "", "</h3>\n\n", 1, 0, 0 },
		{ "md_h3", "### ", "", " ###\n\n", 1, 0, 0 }
	}, {
		{ "raw_dl", "", "", "", 1, 0, 0 },
		{ "html_dl", "<dl>\n", "", "</dl>\n\n", 1, 0, 0 },
		{ "md_dl", "", "", "\n\n", 1, 0, 0 }
	}, {
		{ "raw_dt", "", "", "", 0, 0, 0 },
		{ "html_dt", "\t<dt>", "", "</dt>\n", 0, 0, 0 },
		{ "md_dt", " * ", "", "  \n", 0, 0, 0 }
	}, {
		{ "raw_dd", "", "", "", 0, 0, 0 },
		{ "html_dd", "\t<dd>", "", "</dd>\n", 0, 0, 0 },
		{ "md_dd", "   ", "", "\n", 0, 0, 0 }
	}, {
		{ "raw_em", "", "", "", 0, 0, 0 },
		{ "html_em", "<em>", "", "</em>", 0, 0, 0 },
		{ "md_em", "_", "", "_", 0, 0, 0 }
	}, {
		{ "raw_strong", "", "", "", 0, 0, 0 },
		{ "html_strong", "<strong>", "", "</strong>", 0, 0, 0 },
		{ "md_strong", "*", "", "*", 0, 0, 0 }
	}, {
		{ "raw_html_em", "", "", "", 0, 0, 0 },
		{ "html_html_em", "<em>", "", "</em>", 0, 0, 0 },
		{ "md_html_em", "<em>", "", "</em>", 0, 0, 0 }
	}, {
		{ "raw_html_strong", "", "", "", 0, 0, 0 },
		{ "html_html_strong", "<strong>", "", "</strong>", 0, 0, 0 },
		{ "md_html_strong", "<strong>", "", "</strong>", 0, 0, 0 }
	}
};

static void punctuate_to_string(const struct Punctuate *p,
	char (*const a)[12]) {
	const char *name = p->name;
	size_t name_len = strlen(name);
	if(name_len > sizeof *a - 1) name_len = sizeof *a - 1;
	memcpy(*a, p->name, name_len);
	(*a)[name_len] = '\0';
}

/** This does a delayed lazy unencoded surrounding text. */
struct Style {
	const struct Punctuate *punctuate;
	enum { BEGIN, ITEM, SEPARATE } lazy;
};

static void style_to_string(const struct Style *s, char (*const a)[12]) {
	punctuate_to_string(s->punctuate, a);
}
#define ARRAY_NAME Style
#define ARRAY_TYPE struct Style
#define ARRAY_TO_STRING &style_to_string
#define ARRAY_STACK
#include "Array.h"

/** Style stack with more. */
static struct {
	struct StyleArray styles;
	int is_before_sep;
	struct { const struct Punctuate *punctuate; int on; } highlight;
} style;

/** This takes a bounded and small amount of memory, so unless one has an
 infinite style-push loop or one's memory is very small and one just happened
 to knick this small piece, this is not going to happen. */
static void unrecoverable(void)
	{ perror("Unrecoverable"), fprintf(stderr, "Styles stack: %s.\n",
	StyleArrayToString(&style.styles)), assert(0), exit(EXIT_FAILURE); }

/** Expects the stack to be bounded. If `will_be_popped`, starts searching one
 spot below the top. */
static enum Format effective_format_search(const int will_be_popped) {
	struct Style *s = 0;
	const enum Format f = CdocGetFormat();
	/* If the style will be popped, don't include it. */
	if(will_be_popped) s = StyleArrayBack(&style.styles, s);
	while((s = StyleArrayBack(&style.styles, s)))
		if(s->punctuate->is_to) return s->punctuate->to_format;
	return f;
}
static enum Format effective_format(void) { return effective_format_search(0); }
static enum Format effective_format_will_be_popped(void)
	{ return effective_format_search(1); }

/** @return Unlike `CdocGetFormat` which always returns the same thing, this is
 the style format, which could change. */
enum Format StyleFormat(void) { return effective_format(); }

/** Destructor for styles. */
void Style_(void) {
	assert(!StyleArraySize(&style.styles) && !style.highlight.on);
	StyleArray_(&style.styles);
	style.is_before_sep = 0;
}

static void push(const struct Punctuate *const p) {
	struct Style *const s = StyleArrayNew(&style.styles);
	/* There's so many void functions that rely on this function and it's such
	 a small amount of memory, that it's useless to recover. The OS will have
	 to clean up our mess. Hack. */
	if(!s) { unrecoverable(); return; }
	/*printf("<!-- push %s -->", text->name);*/
	s->punctuate = p;
	s->lazy = BEGIN;
}

/** Push the style `e`. @fixme Failing inexplicably? */
void StylePush(const enum StylePunctuate p) {
	push(&punctuates[p][effective_format()]);
	if(CdocGetDebug() & DBG_STYLE) fprintf(stderr, "Push style %s.\n",
		StyleArrayToString(&style.styles));	
}

static void pop(void) {
	struct Style *const s = StyleArrayPop(&style.styles);
	if(!s) unrecoverable();
	/*printf("<!-- pop %s -->", pop->text->name);*/
	if(s->lazy == BEGIN) return;
	fputs(s->punctuate->end, stdout);
}

/** Pop the style. One must have pushed. */
void StylePop(void) { pop(); }

/** Pops until the the element that is popped is strong or the style list is
 empty. Is strong means it can appear on it's own; _eg_, a list item is not
 strong because it can not appear without a list, but a list is. This allows
 simplified lists. */
void StylePopStrong(void) {
	struct Style *top;
	while((top = StyleArrayPeek(&style.styles)))
		{ pop(); if(top->punctuate->is_strong) break; }
}

/** Pops, separates, and then pushes the same element. */
void StylePopPush(void) {
	struct Style *const peek = StyleArrayPeek(&style.styles), *top;
	assert(peek);
	pop();
	if((top = StyleArrayPeek(&style.styles))) top->lazy = SEPARATE;
	push(peek->punctuate);
}

/** @return Is the top style `p`? */
int StyleIsTop(const enum StylePunctuate p) {
	struct Style *const peek = StyleArrayPeek(&style.styles);
	return peek && (peek->punctuate == &punctuates[p][OUT_RAW]
		|| peek->punctuate == &punctuates[p][OUT_HTML]
		|| peek->punctuate == &punctuates[p][OUT_MD]);
}

/** @return Is the style stack empty? */
int StyleIsEmpty(void) {
	return !StyleArraySize(&style.styles);
}

/** Differed space; must have a style. */
void StyleSeparate(void) {
	struct Style *const top = StyleArrayPeek(&style.styles);
	if(!top) unrecoverable();
	if(top->lazy == ITEM) top->lazy = SEPARATE;
}

/** Crashes the programme if it doesn't find `e`. */
void StyleExpect(const enum StylePunctuate p) {
	const struct Style *const s = StyleArrayPeek(&style.styles);
	if((!p && !s) || (s && s->punctuate
		== &punctuates[p][effective_format_will_be_popped()])) return;
	{
		char a[12];
		const enum Format f = effective_format();
		punctuate_to_string(&punctuates[p][f], &a);
		fprintf(stderr, "Expected %s.\n", a);
	}
	unrecoverable();
}

/** Right before we commit to print, call this to prepare extra characters,
 _eg_, space.
 @param[symbol] What output symbol we are going to print. */
void StyleFlushSymbol(const enum Symbol symbol) {
	struct Style *const top = StyleArrayPeek(&style.styles), *s = 0;
	assert(top);
	/* Make sure all the stack is on `ITEM`. */
	while((s = StyleArrayNext(&style.styles, s))) {
		switch(s->lazy) {
			case ITEM: continue;
			case SEPARATE: fputs(s->punctuate->sep, stdout); break;
			case BEGIN: fputs(s->punctuate->begin, stdout); break;
		}
		s->lazy = ITEM;
		style.is_before_sep = 0;
	}
	assert(top->lazy == ITEM);
	/* If there was no separation, is there an implied separation? */
	if(style.is_before_sep && symbol_before_sep[symbol])
		fputs(top->punctuate->sep, stdout);
	style.is_before_sep = symbol_after_sep[symbol];
	/* Now do the highlight. */
	if(style.highlight.punctuate && !style.highlight.on)
		fputs(style.highlight.punctuate->begin, stdout), style.highlight.on = 1;
}

/** If we want to print a string. */
void StyleFlush(void) { StyleFlushSymbol(END); }

void StyleHighlightOn(const enum StylePunctuate p) {
	assert(!style.highlight.punctuate && !style.highlight.on);
	style.highlight.punctuate = &punctuates[p][effective_format()];
}

void StyleHighlightOff(void) {
	assert(style.highlight.punctuate);
	if(style.highlight.on)
		fputs(style.highlight.punctuate->end, stdout), style.highlight.on = 0;
	style.highlight.punctuate = 0;
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
		if(is_buffer) goto raw_encode_buffer;
		goto raw_encode_print; /* This is `things in math/code` only in MD. */
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
	
raw_encode_print:
	printf("%.*s", length, from);
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

static void encode_len(const int length, const char *const from) {
	assert(length > 0);
	encode_len_choose(length, from, effective_format(), 0);
}

/** Encodes `from` with the `length` in the style chosen to `stdout`. */
void StyleEncodeLength(const int length, const char *const from) {
	if(!from || length <= 0) return;
	encode_len(length, from);
}

/** Encodes `string` in the style chosen to `stdout`. */
void StyleEncode(const char *const string) {
	size_t length;
	if(!string) return;
	length = strlen(string);
	if(length > INT_MAX) { fprintf(stderr,
		"StyleEncode \"%.10s...\" length clipped at %d.\n", string, INT_MAX);
		length = INT_MAX; }
	encode_len((int)length, string);
}

/** Encodes `from` to `length` in the style chosen and returns the buffer.
 @return The buffer or null if . */
const char *StyleEncodeLengthCatToBuffer(const int length,
	const char *const from) {
	if(length <= 0 || !from) return BufferGet();
	encode_len_choose(length, from, effective_format(), 1);
	return BufferGet();
}
	   
/** @return The buffer or null. */
const char *StyleEncodeLengthRawToBuffer(const int length,
	const char *const from) {
	BufferClear();
	if(length > 0 && from) encode_len_choose(length, from, OUT_RAW, 1);
	return BufferGet();
}
