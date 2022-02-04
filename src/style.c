/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).
 
 Outputs between tokens based on a hierarchical lazy state; one should call
 this before every token group thing. It also encodes things. Is is really
 strict. */

#include "cdoc.h"
#include "buffer.h"
#include "style.h" /** \include */
#include <string.h> /* strlen memcpy */
#include <stdio.h>  /* fprintf */
#include <limits.h> /* INT_MAX */

/* `SYMBOL` is declared in `symbol.h`. */
#define X(a, b, c, d, e, f) d
static const int symbol_before_sep[] = { SYMBOL };
#undef X
#define X(a, b, c, d, e, f) e
static const int symbol_after_sep[]  = { SYMBOL };
#undef X

/* Can have a beginning, a separator, and an end, which will be printed around
 literals. Must match the <tag:style_punctuate>. */
static const struct punctuate {
	const char *name, *begin, *sep, *end;
	int is_strong, is_to;
	const enum format to_format;
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
		{ "html_div", "<div>\n\n", "", "</div>\n\n", 1, 0, 0 },
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

static void punctuate_to_string(const struct punctuate *p,
	char (*const a)[12]) {
	const char *name = p->name;
	size_t name_len = strlen(name);
	if(name_len > sizeof *a - 1) name_len = sizeof *a - 1;
	memcpy(*a, p->name, name_len);
	(*a)[name_len] = '\0';
}

/** This does a delayed lazy unencoded surrounding text. */
struct style {
	const struct punctuate *punctuate;
	enum { BEGIN, ITEM, SEPARATE } lazy;
};

static void style_to_string(const struct style *s, char (*const a)[12]) {
	punctuate_to_string(s->punctuate, a);
}
#define ARRAY_NAME style
#define ARRAY_TYPE struct style
#define ARRAY_EXPECT_TRAIT
#include "array.h"
#define ARRAY_TO_STRING &style_to_string
#include "array.h"

/** Singleton style stack (array) with temporary values. */
static struct {
	struct style_array styles;
	int is_before_sep;
	struct { const struct punctuate *punctuate; int on; } highlight;
} style;

/** This takes a bounded and small amount of memory, so unless one has an
 infinite style-push loop or one's memory is very small, this is not going to
 happen. (We think.) */
static void unrecoverable(void)
	{ perror("Unrecoverable"), fprintf(stderr, "style stack: %s.\n",
	style_array_to_string(&style.styles)), assert(0), exit(EXIT_FAILURE); }

/** Expects the stack to be bounded. If `will_be_popped`, starts searching one
 spot below the top. */
static enum format effective_format_search(const int will_be_popped) {
	size_t i = style.styles.size;
	const enum format f = cdoc_get_format();
	/* If the style will be popped, don't include it. */
	if(i && will_be_popped) i--;
	while(i) {
		const struct style *const s = style.styles.data + --i;
		if(s->punctuate->is_to) return s->punctuate->to_format;
	}
	return f;
}
static enum format effective_format(void) { return effective_format_search(0); }
static enum format effective_format_will_be_popped(void)
	{ return effective_format_search(1); }

/** @return Unlike `cdoc_get_format` which always returns the same thing, this
 is the style format, which could change. */
enum format style_format(void) { return effective_format(); }

/** Destructor for styles. */
void style_(void) {
	assert(!style.styles.size && !style.highlight.on);
	style_array_(&style.styles);
	style.is_before_sep = 0;
}

static void push(const struct punctuate *const p) {
	struct style *const s = style_array_new(&style.styles);
	/* There's so many void functions that rely on this function and it's such
	 a small amount of memory, that it's useless to recover. The OS will have
	 to clean up our mess. Hack. */
	if(!s) { unrecoverable(); return; }
	/*printf("<!-- push %s -->", text->name);*/
	s->punctuate = p;
	s->lazy = BEGIN;
	if(cdoc_get_debug() & DBG_STYLE) fprintf(stderr, "Push style, now %s.\n",
		style_array_to_string(&style.styles));
}

/** Push the style `e`. @fixme Failing inexplicably? */
void style_push(const enum style_punctuate p) {
	push(&punctuates[p][effective_format()]);
}

static void pop(void) {
	struct style *const s = style_array_pop(&style.styles);
	if(!s) unrecoverable();
	/*printf("<!-- pop %s -->", pop->text->name);*/
	if(s->lazy == BEGIN) return;
	fputs(s->punctuate->end, stdout);
	if(cdoc_get_debug() & DBG_STYLE) fprintf(stderr, "Pop style, now %s.\n",
		style_array_to_string(&style.styles));
}

/** Pop the style. One must have pushed. */
void style_pop(void) { pop(); }

/** Pops until the the element that is popped is strong or the style list is
 empty. Is strong means it can appear on it's own; _eg_, a list item is not
 strong because it can not appear without a list, but a list is. This allows
 simplified lists. */
void style_pop_strong(void) {
	struct style *top;
	while((top = style_array_peek(&style.styles)))
		{ pop(); if(top->punctuate->is_strong) break; }
}

/** Pops, separates, and then pushes the same element. */
void style_pop_push(void) {
	struct style *const peek = style_array_peek(&style.styles), *top;
	assert(peek);
	pop();
	if((top = style_array_peek(&style.styles))) top->lazy = SEPARATE;
	push(peek->punctuate);
}

/** @return Is the top style `p`? */
int style_Is_top(const enum style_punctuate p) {
	struct style *const peek = style_array_peek(&style.styles);
	return peek && (peek->punctuate == &punctuates[p][OUT_RAW]
		|| peek->punctuate == &punctuates[p][OUT_HTML]
		|| peek->punctuate == &punctuates[p][OUT_MD]);
}

/** @return Is the style stack empty? */
int style_is_empty(void) { return !style.styles.size; }

/** Differed space; must have a style. */
void style_separate(void) {
	struct style *const top = style_array_peek(&style.styles);
	if(!top) unrecoverable();
	if(top->lazy == ITEM) top->lazy = SEPARATE;
}

/** Crashes the programme if it doesn't find `p`. */
void style_expect(const enum style_punctuate p) {
	const struct style *const s = style_array_peek(&style.styles);
	if((!p && !s) || (s && s->punctuate
		== &punctuates[p][effective_format_will_be_popped()])) return;
	{
		char a[12];
		const enum format f = effective_format();
		punctuate_to_string(&punctuates[p][f], &a);
		fprintf(stderr, "Expected %s.\n", a);
	}
	unrecoverable();
}

/** Right before we commit to print, call this to prepare extra characters,
 _eg_, space.
 @param[symbol] What output symbol we are going to print. */
void style_flush_symbol(const enum symbol symbol) {
	size_t i = 0;
	struct style *const top = style_array_peek(&style.styles);
	assert(top);
	/* Make sure all the stack is on `ITEM`. */
	while(i < style.styles.size) {
		struct style *const s = style.styles.data + i;
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
void style_flush(void) { style_flush_symbol(END); }

void style_highlight_on(const enum style_punctuate p) {
	assert(!style.highlight.punctuate && !style.highlight.on);
	style.highlight.punctuate = &punctuates[p][effective_format()];
}

void style_highlight_off(void) {
	assert(style.highlight.punctuate);
	if(style.highlight.on)
		fputs(style.highlight.punctuate->end, stdout), style.highlight.on = 0;
	style.highlight.punctuate = 0;
}

/** Encode a bunch of arbitrary text `from` to `length` as whatever the options
 were.
 @param[is_buffer] Appends to the buffer chosen in `buffer.c`. */
static void encode_len_choose(size_t length, const char *from,
	const enum format f, const int is_buffer) {
	size_t ahead = 0;
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
	if(!(b = buffer_prepare(length))) { unrecoverable(); return; }
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
			if(!(b = buffer_prepare(ahead))) { unrecoverable(); return; }
			memcpy(b, from, ahead);
			length -= ahead;
			from += ahead;
			ahead = 0;
		}
		if(!(b = buffer_prepare(str_len))) { unrecoverable(); return; }
		memcpy(b, str, str_len);
		from++, length--;
	}
terminate_html:
	if(ahead) {
		if(!(b = buffer_prepare(ahead))) { unrecoverable(); return; }
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
			if(!(b = buffer_prepare(ahead))) { unrecoverable(); return; }
			memcpy(b, from, ahead);
			length -= ahead;
			from += ahead;
			ahead = 0;
		}
		if(escape) {
			const size_t e_len = strlen(escape);
			if(!(b = buffer_prepare(e_len))) { unrecoverable(); return; }
			memcpy(b, escape, e_len);
		} else {
			if(!(b = buffer_prepare(2))) { unrecoverable(); return; }
			b[0] = '\\', b[1] = *from;
		}
		from++, length--;
	}
terminate_md:
	if(ahead) {
		if(!(b = buffer_prepare(ahead))) { unrecoverable(); return; }
		memcpy(b, from, ahead);
	}
	return;
	
raw_encode_print:
	if(length > INT_MAX) unrecoverable();
	printf("%.*s", (int)length, from);
	return;
	
html_encode_print:
	while(length) {
		switch(*from) {
			case '<': fputs(HTML_LT, stdout); break;
			case '>': fputs(HTML_GT, stdout); break;
			case '&': fputs(HTML_AMP, stdout); break;
			case '\0':
				fprintf(stderr, "Encoded null with %d left.\n", (int)length);
				return;
			default: fputc(*from, stdout); break;
		}
		from++, length--;
	}
	return;
	
md_encode_print:
	while(length) {
		switch(*from) {
			case '\0':
				fprintf(stderr, "Encoded null with %d left.\n", (int)length);
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

static void encode_len(const size_t length, const char *const from) {
	assert(length > 0);
	encode_len_choose(length, from, effective_format(), 0);
}

/** Encodes `from` with the `length` in the style chosen to `stdout`. */
void style_encode_length(const size_t length, const char *const from) {
	if(!from || length <= 0) return;
	encode_len(length, from);
}

/** Encodes `string` in the style chosen to `stdout`. */
void style_encode(const char *const string) {
	size_t length;
	if(!string) return;
	length = strlen(string);
	if(length > INT_MAX) { fprintf(stderr,
		"style_encode \"%.10s...\" length clipped at %d.\n", string, INT_MAX);
		length = INT_MAX; }
	encode_len(length, string);
}

/** Encodes `from` to `length` in the style chosen and returns the buffer.
 @return The buffer or null if . */
const char *style_encode_length_cat_to_buffer(const size_t length,
	const char *const from) {
	if(length <= 0 || !from) return buffer_get();
	encode_len_choose(length, from, effective_format(), 1);
	return buffer_get();
}
	   
/** @return The buffer or null. */
const char *style_encode_length_raw_to_buffer(const size_t length,
	const char *const from) {
	buffer_clear();
	if(length > 0 && from) encode_len_choose(length, from, OUT_RAW, 1);
	return buffer_get();
}
