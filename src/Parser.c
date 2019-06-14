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

/* Private token infomation. From `Scanner.h`. */
static const char *const tokens[] = { TOKEN(STRINGISE_A) };
static const int token_flags[] = { TOKEN(PARAM_B) };

/* Define the sections of output. */
#define SECTION(X) X(HEADER), X(DECLARATION), X(FUNCTION)
enum Section { SECTION(PARAM) };
static const char *const sections[] = { SECTION(STRINGISE) };

/* Parser. */

/** Parser is only valid when Scanner is active and in steady-state. */
struct Symbol {
	enum Token token;
	const char *from;
	int length;
};

static void symbol_to_string(const struct Symbol *s, char (*const a)[12]) {
	int len = s->length >= 5 ? 5 : s->length;
	sprintf(*a, "%.4s<%.*s>", tokens[s->token], len, s->from);
	/*strncpy(*a, tokens[s->token], sizeof *a - 1);*/
	*a[sizeof *a - 1] = '\0';
}

#define ARRAY_NAME Symbol
#define ARRAY_TYPE struct Symbol
#define ARRAY_TO_STRING &symbol_to_string
#include "../src/Array.h"

struct Segment {
	struct SymbolArray doc, code;
	enum Section section;
};

/* A {SegmentArray} is the top level parser. */
#define ARRAY_NAME Segment
#define ARRAY_TYPE struct Segment
#include "../src/Array.h"

static void DeleteAllSegments(struct SegmentArray *const sa) {
	struct Segment *s;
	if(!sa) return;
	while((s = SegmentArrayPop(sa)))
		SymbolArray_(&s->doc), SymbolArray_(&s->code);
}

static struct Segment *NewSegment(struct SegmentArray *const sa) {
	struct Segment *s;
	assert(sa);
	if(!(s = SegmentArrayNew(sa))) return 0;
	SymbolArray(&s->doc);
	SymbolArray(&s->code);
	s->section = HEADER; /* Default. */
	return s;
}

/* Create a new symbol for this segment. */
static int PushSymbol(struct Segment *const s, const int is_doc,
	const enum Token token) {
	struct Symbol *symbol;
	assert(s);
	/* @fixme: This line is not initialised? */
	if(!(symbol = SymbolArrayNew(is_doc ? &s->doc : &s->code))) return 0;
	symbol->token = token;
	symbol->from = ScannerGetFrom();
	symbol->length = ScannerGetLength();
	return 1;
}



/** Cleans the whitespace so it's just in between words and adds paragraphs
 where needed. */
static void clean_whitespace(struct SymbolArray *const sa,
	const struct SymbolArray *const white,
	const struct SymbolArray *const paragraph) {
	const struct SymbolArray *replace;
	struct Symbol *x = 0, *x_start = 0;
	size_t count_nl = 0;
	int is_content = 0;
	assert(sa);
	while((x = SymbolArrayNext(sa, x))) {
		switch(x->token) {
		case NEWLINE: count_nl++; /* Fall-through. */
		case WHITESPACE: if(!x_start) x_start = x; break;
		default: /* Non white-space, viz,
			[ 0, ... x_start (whitespace), ..., x (first non-white), ... ] */
			if(x_start) {
				replace = is_content ? count_nl > 1 ? paragraph : white : 0;
				count_nl = 0;
				SymbolArrayReplace(sa, x_start, (long)(x - x_start), replace);
				x = x_start + SymbolArraySize(replace);
				x_start = 0;
			}
			is_content = 1;
		}
	}
	/* Whitespace at end of section. */
	if(x_start) SymbolArrayReplace(sa, x_start, -1, 0);
	printf("Parser:Clean: %s.\n", SymbolArrayToString(sa));
}

/** @implements Predicate<Segment> */
static int keep_segment(const struct Segment *const s) {
	if(SymbolArraySize(&s->doc) || s->section == FUNCTION) return 1;
	return 0;
}

int main(int argc, char **argv) {
	enum Token t = END;
	struct SegmentArray text;
	int is_done = 0, is_doc = 0;
	int indent_level = 0;
	struct SymbolArray white, paragraph;

	/* https://stackoverflow.com/questions/10293387/piping-into-application-run-under-xcode/13658537 */
	if (argc == 2 && strcmp(argv[1], "debug") == 0 ) {
		const char *test_file_path = "/Users/neil/Movies/Cdoc/c.txt";
		printf("== [RUNNING IN DEBUG MODE with %s]==\n\n", test_file_path);
		freopen(test_file_path, "r", stdin);
	}

	/* Initialise. */
	SegmentArray(&text);
	SymbolArray(&white);
	SymbolArray(&paragraph);

	do { /* Try. */
		int is_indent = 0, is_struct = 0, is_line = 0;
		struct Segment *segment = 0;

		/* Constant things. */
		{
			struct Symbol *s;
			if(!(s = SymbolArrayNew(&white))) break;
			s->token = WHITESPACE;
			s->from = 0;
			s->length = 0;
			if(!(s = SymbolArrayNew(&paragraph))) break;
			s->token = NEWLINE;
			s->from = 0;
			s->length = 0;
		}

		/* Lex. */

		if(!Scanner()) break; /* First. */
		while((t = ScannerScan())) {
			int indent; /* Debug. */
			is_doc = ScannerIsDoc();
			indent_level = ScannerGetIndentLevel();
			printf("%lu%c\t", (unsigned long)ScannerGetLine(),
				is_doc ? '~' : ':');
			for(indent = 0; indent < indent_level; indent++)
				fputc('\t', stdout);
			printf("%s %s \"%.*s\"\n", ScannerGetStates(),
				tokens[t], ScannerGetLength(), ScannerGetFrom());
			if(!is_indent) {
				if(indent_level) { /* Entering a code block. */
					struct Symbol *symbol;
					assert(indent_level == 1 && !is_doc && t == LBRACE);
					is_indent = 1;
					/* Determine if this is function. */
					if((symbol = SymbolArrayPop(&segment->code))
						&& symbol->token != RPAREN) {
						if(segment) segment->section = DECLARATION;
						is_struct = 1;
					} else if(segment) segment->section = FUNCTION;
				} else if(t == SEMI) { /* Semicolons on indent level 0. */
					/* General declaration? */
					if(segment && segment->section == HEADER)
						segment->section = DECLARATION;
					is_line = 1;
				} else if(segment && SymbolArraySize(&segment->doc)
					&& !SymbolArraySize(&segment->code)
					&& (t == BEGIN_DOC
					|| (!is_doc && ScannerIsDocFar()))) {
					/* Hasn't scanned any code and is on the top level, cut
					 multiple docs and the doc has to be within a reasonable
					 distance. */
					printf("<cut>\n"), segment = 0;
				}
			} else { /* {is_indent}. */
				if(!indent_level) { /* Exiting to indent level 0. */
					is_indent = 0, assert(t == RBRACE);
					if(!is_struct) is_line = 1; /* Functions (fixme: sure?) */
				} else if(!is_struct && !is_doc) {
					continue; /* Code in functions: don't care. */
				}
			}
			/* This is a dummy token that is for splitting up multiple doc
			 comments on a single line -- ignore in practice. */
			if(t == BEGIN_DOC) continue;
			/* Create new segment if need be. */
			if(!segment) {
				printf("<new segment>\n");
				if(!(segment = NewSegment(&text))) break;
			}
			if(!PushSymbol(segment, is_doc, t)) break;
			/* Create another segment next time. */
			if(is_line) is_line = 0, is_struct = 0, segment = 0;
		}
		if(t) break;
		if(indent_level) { errno = EILSEQ; break; }

		/* Cull. Rid uncommented blocks. Whitespace clean-up, (after!) */
		SegmentArrayKeepIf(&text, &keep_segment);
		segment = 0;
		/*while((segment = SegmentArrayNext(&text, segment)))
			split_sections(&segment->doc);*/
		while((segment = SegmentArrayNext(&text, segment)))
			clean_whitespace(&segment->doc, &white, &paragraph);

		is_done = 1;
	} while(0); if(!is_done) {
		perror("Cdoc");
		fprintf(stderr, "At %lu%c indent level %d; state stack %s; %s \"%.*s\"."
			"\n", (unsigned long)ScannerGetLine(), is_doc ? '~' : ':',
			indent_level, ScannerGetStates(),
			tokens[t], ScannerGetLength(), ScannerGetFrom());
	} else {
		struct Segment *segment = 0;
		fputs("\n\n*****\n\n", stdout);
		while((segment = SegmentArrayNext(&text, segment))) {
			printf("Segment(%s):\n\tdoc: %s.\n\tcode: %s.\n\n",
				sections[segment->section],
				SymbolArrayToString(&segment->doc),
				SymbolArrayToString(&segment->code));
		}
		fputc('\n', stdout);
	} {
		DeleteAllSegments(&text), SegmentArray_(&text);
		Scanner_();
	}
	return is_done ? EXIT_SUCCESS : EXIT_FAILURE;
}
