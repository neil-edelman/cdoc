/** 2019 Neil Edelman, distributed under the terms of the MIT License,
 \url{ https://opensource.org/licenses/MIT }.

 {Doxygen} should be used if possible. However, maybe one is too hip to use it?
 (or it uses libraries that are too new for one's computer.) The documentation
 is a perversion and simplification of {Doxygen}.

 Parses and extracts the documentation commands in a {.c} file. A documentation
 command begins with {/\**}.

 @title Cdoc.c
 @author Neil
 @version 2019-05 Re-done in {re2c}.
 @since 2017-03 Initial version.
 @std C89
 @depend re2c (\url{http://re2c.org/})
 @fixme Different doc comments need new paragraphs.
 @fixme Lists in comments, etc.
 @fixme {void A_BI_(Create, Thing)(void)} -> {<A>Create<BI>Thing(void)}.
 @fixme Trigraph support. */

#include <stdlib.h> /* EXIT malloc free */
#include <stdio.h>  /* FILE printf fputc perror */
#include <string.h> /* memmove memset */
#include <errno.h>  /* EDOM ERANGE */
/* #define NDEBUG */
#include <assert.h> /* assert */

#include "Scanner.h"

/* Private symbol infomation. From `Scanner.h`. */
static const int symbol_flags[] = { SYMBOL(PARAM_B) };

/* Define the sections of output. */
#define SECTION(X) X(UNDECIDED), X(HEADER), X(DECLARATION), X(FUNCTION)
enum Section { SECTION(PARAM) };
static const char *const sections[] = { SECTION(STRINGISE) };

/*
static struct Token global_token;

static int accept(const enum Symbol symbol) {
	static Token *token;
	if(symbol != global_token.symbol) return 0;
	
	getsym();
	return 1;
}

static int expect(const enum Symbol symbol) {
	if(accept(symbol)) return 1;
	errno = EILSEQ;
	return 0;
}
*/






/* `Token` is in `Scanner.h` to be used by `ScannerToken`. */

static void token_to_string(const struct Token *s, char (*const a)[12]) {
	int len = s->length >= 5 ? 5 : s->length;
	sprintf(*a, "%.4s<%.*s>", symbols[s->symbol], len, s->from);
	/*strncpy(*a, symbols[s->symbol], sizeof *a - 1);*/
	*a[sizeof *a - 1] = '\0';/*???*/
}

#define ARRAY_NAME Token
#define ARRAY_TYPE struct Token
#define ARRAY_TO_STRING &token_to_string
#include "../src/Array.h"

/* `static` constants that require memory so are initialised on start and
 deleted on end. */
static struct TokenArray whitespace, paragraph;



/** Tags are documentation symbols and everything that comes after.
 Ie, `tag = TAG_TITLE, header = {}, contents = { all, , your, , base }` or
 `tag = TAG_PARAM, header = { ID"x" }, contents = { the, , dependant, ,
 varible }`. */
struct Tag {
	struct Token token;
	struct TokenArray header;
	struct TokenArray contents;
};

static void tag_to_string(const struct Tag *t, char (*const a)[12]) {
	strncpy(*a, symbols[t->token.symbol], sizeof *a - 1),
		*a[sizeof *a - 1] = '\0';
}

static void tag_init(struct Tag *const tag) {
	assert(tag);
	TokenArray(&tag->header);
	TokenArray(&tag->contents);
	ScannerToken(&tag->token);
}

#define ARRAY_NAME Tag
#define ARRAY_TYPE struct Tag
#define ARRAY_TO_STRING &tag_to_string
#include "Array.h"

static void tag_array_remove(struct TagArray *const ta) {
	struct Tag *t;
	if(!ta) return;
	while((t = TagArrayPop(ta)))
		TokenArray_(&t->header), TokenArray_(&t->contents);
	TagArray_(ta);
}



/** Segment is classified to a section of the document and can have
 documentation including tags and code. Ie, section = FUNCTION, doc = { I, ,
 don't, , know, , what, , this, , fn, , does }, code = { int foo(int foo) {} },
 tags = { { TAG_PARAM, { does, , nothing } }, { TAG_ALLOW, { } } } */
struct Segment {
	enum Section section;
	struct TokenArray doc, code;
	struct TagArray tags;
};

static void segment_init(struct Segment *const segment) {
	assert(segment);
	segment->section = HEADER; /* Default. */
	TokenArray(&segment->doc);
	TokenArray(&segment->code);
	TagArray(&segment->tags);
}

static void segment_to_string(const struct Segment *seg, char (*const a)[12]) {
	strncpy(*a, sections[seg->section], sizeof *a - 1),
		*a[sizeof *a - 1] = '\0';
}

/* A {SegmentArray} is the top level parser. */
#define ARRAY_NAME Segment
#define ARRAY_TYPE struct Segment
#define ARRAY_TO_STRING &segment_to_string
#include "../src/Array.h"

static void segment_array_remove(struct SegmentArray *const sa) {
	struct Segment *s;
	if(!sa) return;
	while((s = SegmentArrayPop(sa)))
		TokenArray_(&s->doc), TokenArray_(&s->code),
		tag_array_remove(&s->tags);
	SegmentArray_(sa);
}



/** Cleans the whitespace so it's just in between words and adds paragraphs
 where needed. */
static void clean_whitespace(struct TokenArray *const sa) {
	const struct TokenArray *replace;
	struct Token *x = 0, *x_start = 0;
	size_t count_nl = 0;
	int is_content = 0;
	assert(sa);
	while((x = TokenArrayNext(sa, x))) {
		switch(x->symbol) {
		case NEWLINE: count_nl++; /* Fall-through. */
		case WHITESPACE: if(!x_start) x_start = x; break;
		default: /* Non white-space, viz,
			[ 0, ... x_start (whitespace), ..., x (first non-white), ... ] */
			if(x_start) {
				replace = is_content ? count_nl > 1 ? &paragraph : &whitespace : 0;
				count_nl = 0;
				TokenArrayReplace(sa, x_start, (long)(x - x_start), replace);
				x = x_start + TokenArraySize(replace);
				x_start = 0;
			}
			is_content = 1;
		}
	}
	/* Whitespace at end of section. */
	if(x_start) TokenArrayReplace(sa, x_start, -1, 0);
	printf("Parser:Clean: %s.\n", TokenArrayToString(sa));
}

/** @implements Predicate<Segment> */
static int keep_segment(const struct Segment *const s) {
	if(TokenArraySize(&s->doc) || s->section == FUNCTION) return 1;
	return 0;
}

int main(int argc, char **argv) {
	struct SegmentArray text;
	int is_done = 0, is_matching = 1;

	/* https://stackoverflow.com/questions/10293387/piping-into-application-run-under-xcode/13658537 */
	if (argc == 2 && strcmp(argv[1], "debug") == 0 ) {
		const char *test_file_path = "/Users/neil/Movies/Cdoc/c.txt";
		printf("== [RUNNING IN DEBUG MODE with %s]==\n\n", test_file_path);
		freopen(test_file_path, "r", stdin);
	}

	/* Initialise. */
	SegmentArray(&text);
	do { /* Try. */
		int is_indent = 0, is_struct = 0, is_line = 0;
		struct Segment *segment = 0;

		{ /* Constant things. */
			struct Token *s;
			if(!(s = TokenArrayNew(&whitespace))) break;
			s->symbol = WHITESPACE;
			s->from = 0, s->length = 0, s->line = 0;
			if(!(s = TokenArrayNew(&paragraph))) break;
			s->symbol = NEWLINE;
			s->from = 0, s->length = 0, s->line = 0;
		}

		/* Lex. */

		if(!Scanner()) break; /* First. */
		while((ScannerNext())) {
			struct Token token, *pushed_token;
			struct TokenInfo info;
			ScannerToken(&token);
			ScannerTokenInfo(&info);
			is_matching = !info.indent_level;
			{ /* Print debug. */
				int indent;
				printf("%lu%c\t", (unsigned long)token.line,
					info.is_doc ? '~' : ':');
				for(indent = 0; indent < info.indent_level; indent++)
					fputc('\t', stdout);
				printf("%s %s \"%.*s\"\n", ScannerStates(),
				   symbols[token.symbol], token.length,
				   token.from);
			}
			if(!is_indent) {
				if(info.indent_level) { /* Entering a code block. */
					struct Token *fn;
					assert(info.indent_level == 1 && !info.is_doc
						&& token.symbol == LBRACE);
					is_indent = 1;
					/******* Maybe it segfaults depending on what compiler and what version you compile it on. vvvv ********/
					printf("HERE segment %s is crashing segment = %s.\n", SegmentArrayToString(&text), sections[segment->section]);
#if 1
					/* Determine if this is function. */
					if(segment) {
						/* ???? what??? it hasn't been read yet? */
					}
					if((fn = TokenArrayPop(&segment->code))
						&& fn->symbol != RPAREN) {
						if(segment) segment->section = DECLARATION;
						is_struct = 1;
					} else if(segment) segment->section = FUNCTION;
#endif
				} else if(token.symbol == SEMI) { /* Semicolons on indent level 0. */
					/* General declaration? */
					if(segment && segment->section == HEADER)
						segment->section = DECLARATION;
					is_line = 1;
				} else if(segment && TokenArraySize(&segment->doc)
					&& !TokenArraySize(&segment->code)
					&& (token.symbol == BEGIN_DOC
					|| (!info.is_doc && info.is_doc_far))) {
					/* Hasn't scanned any code and is on the top level, cut
					 multiple docs and the doc has to be within a reasonable
					 distance. */
					printf("<cut>\n"), segment = 0;
				}
			} else { /* {is_indent}. */
				if(!info.indent_level) { /* Exiting to indent level 0. */
					is_indent = 0, assert(token.symbol == RBRACE);
					if(!is_struct) is_line = 1; /* Functions (fixme: sure?) */
				} else if(!is_struct && !info.is_doc) {
					continue; /* Code in functions: don't care. */
				}
			}
			/* This is a dummy symbol that is for splitting up multiple doc
			 comments on a single line -- ignore in practice. */
			if(token.symbol == BEGIN_DOC) continue;
			/* Create new segment if need be. */
			if(!segment) {
				printf("<new segment>\n");
				if(!(segment = SegmentArrayNew(&text))) break;
				segment_init(segment);
			}
			/* Push symbol. */
			if(!(pushed_token = TokenArrayNew(info.is_doc
				? &segment->doc : &segment->code))) break;
			ScannerToken(pushed_token);
			/* Create another segment next time. */
			if(is_line) is_line = 0, is_struct = 0, segment = 0;
		}
		if(!is_matching) { errno = EILSEQ; break; }

		/* Cull. Rid uncommented blocks. Whitespace clean-up, (after!) */
		SegmentArrayKeepIf(&text, &keep_segment);
		segment = 0;
		/*while((segment = SegmentArrayNext(&text, segment)))
			split_sections(&segment->doc);*/
		while((segment = SegmentArrayNext(&text, segment)))
			clean_whitespace(&segment->doc);

		is_done = 1;
	} while(0); if(!is_done) {
		perror("Cdoc");
		/*fprintf(stderr, "At %lu%c indent level %d; state stack %s; %s \"%.*s\"."
			"\n", (unsigned long)ScannerGetLine(), is_doc ? '~' : ':',
			indent_level, ScannerGetStates(),
			symbols[symbol], ScannerGetLength(), ScannerGetFrom()); fixme*/
	} else {
		struct Segment *segment = 0;
		fputs("\n\n*****\n\n", stdout);
		while((segment = SegmentArrayNext(&text, segment))) {
			printf("Segment(%s):\n\tdoc: %s.\n\tcode: %s.\n\n",
				sections[segment->section],
				TokenArrayToString(&segment->doc),
				TokenArrayToString(&segment->code));
		}
		fputc('\n', stdout);
	} {
		segment_array_remove(&text);
		Scanner_();
		TokenArray_(&paragraph);
		TokenArray_(&whitespace);
	}
	return is_done ? EXIT_SUCCESS : EXIT_FAILURE;
}
