/** 2019 Neil Edelman, distributed under the terms of the MIT License,
 \url{ https://opensource.org/licenses/MIT }.

 {Doxygen} should be used if possible. However, maybe one is too hip to use it?
 (or it uses libraries that are too new for one's computer.) The documentation
 is a perversion and simplification of {Doxygen}.

 Sorts and stores the parsed file into sections.
 
 Parses and extracts the documentation commands in a {.c} file. A documentation
 command begins with {/\**}.

 Any documentation comments up to two lines before the statement
 begins, but not before the the previous statement ended, and before
 the statement comes to a close, belong to that statement.

 Any documentation begin greater then 2 lines above the next thing to
 comment on is in the header.

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
#include "Sorter.h"
#include "Marker.h"

/* `Token` is in `Scanner.h` to be used by `ScannerToken`. */

static void token_to_string(const struct Token *s, char (*const a)[12]) {
	int len = s->length >= 5 ? 5 : s->length;
	sprintf(*a, "%.4s<%.*s>", symbols[s->symbol], len, s->from);
	(*a)[sizeof *a - 1] = '\0';
}

#define ARRAY_NAME Token
#define ARRAY_TYPE struct Token
#define ARRAY_TO_STRING &token_to_string
#include "../src/Array.h"

/** `TokenArraySize` is used in `Marker`. */
size_t TokensSize(const struct TokenArray *const ta) {
	return TokenArraySize(ta);
}

/** `TokenArrayNext` is used in `Marker`. */
struct Token *TokensNext(const struct TokenArray *const a,
	struct Token *const here) {
	return TokenArrayNext(a, here);
}

/* `static` constants that require memory so are initialised on start and
 deleted on end. */
static struct TokenArray paragraph;



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
	strncpy(*a, symbols[t->token.symbol], sizeof *a - 1);
	(*a)[sizeof *a - 1] = '\0';
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



/** A {SegmentArray} is the top level parser. `Segment` is classified to a
 section of the document and can have documentation including tags and code.
 Ie, `section = FUNCTION, doc = { I, , don't, , know, , what, , this, , fn, ,
 does }`, `code = { int foo(int foo) {} }`, `tags = { { TAG_PARAM, { does, ,
 nothing } }, { TAG_ALLOW, { } } }`. */
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
	strncpy(*a, sections[seg->section], sizeof *a - 1);
	(*a)[sizeof *a - 1] = '\0';
}

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

#include "Transform.h"
#include "Out.h"

static struct Sorter {
	int is_indent, is_struct, is_differed_cut;
	struct Token token;
	struct TokenInfo info;
	struct Segment *segment;
	struct Tag *tag;
	struct TokenArray *tokens;
} sorter;

static void sorter_end_segment(void) {
	char a[12];
	if(sorter.segment) segment_to_string(sorter.segment, &a);
	else strcpy(a, "null");
	printf("<Ending %s segment.>\n", a);
	sorter.is_differed_cut = 0, sorter.is_struct = 0, sorter.tag = 0;
	sorter.segment = 0;
	sorter.tag = 0;
	sorter.tokens = 0;
}

static void sorter_err(void) {
	fprintf(stderr, "At %lu%c indent level %d; %s \"%.*s\".\n",
		(unsigned long)sorter.token.line, sorter.info.is_doc ? '~' : ':',
		sorter.info.indent_level,
		symbols[sorter.token.symbol], sorter.token.length, sorter.token.from);
}

int main(int argc, char **argv) {
	struct SegmentArray segments = ARRAY_ZERO;
	int is_done = 0;

	/* https://stackoverflow.com/questions/10293387/piping-into-application-run-under-xcode/13658537 */
	if (argc == 2 && strcmp(argv[1], "debug") == 0 ) {
		const char *test_file_path = "/Users/neil/Movies/Cdoc/c.txt";
		printf("== [RUNNING IN DEBUG MODE with %s]==\n\n", test_file_path);
		freopen(test_file_path, "r", stdin);
	}

	{ /* Constant dynamically allocated things. */
		struct Token *t;
		if(!(t = TokenArrayNew(&paragraph))) goto catch;
		t->symbol = NEWLINE;
		t->from = 0, t->length = 0, t->line = 0;
	}

	/* Lex. */
	if(!Scanner()) goto catch;
	while((ScannerNext())) {
		ScannerToken(&sorter.token);
		ScannerTokenInfo(&sorter.info);

		switch(sorter.token.symbol) {

		case BEGIN_DOC:
			/* If there's no `code`, end the segment.
			 eg, `/ ** Header * / / ** Function * / int main(void)`. */
			if(sorter.segment && !TokenArraySize(&sorter.segment->code))
				sorter_end_segment();
			continue;

		case SEMI:
			/* A semicolon always ends the section; we should see what section
			 it's supposed to be in. */
			if(!sorter.segment) { fprintf(stderr,
				"Stray semicolon on line %lu?\n",
				(unsigned long)sorter.token.line); continue; }
			sorter.is_differed_cut = 1;
			sorter.segment->section = Marker(&sorter.segment->code);
			break;

		case END_BLOCK:
			/* It's a function; behaves like a semicolon; already classified. */
			if(!sorter.segment)
				{ fprintf(stderr, "Stray code-block ending on line %lu?\n",
				(unsigned long)sorter.token.line); continue; }
			sorter.is_differed_cut = 1;
			break;

		case LBRACE:
			/* Starting a code block. If it's global, then it could be a
			 function, in which case we ignore all code in it. But also
			 definition of `struct`, `union`, or `enum`. */
			if(sorter.info.indent_level != 1) break;
			if(!sorter.segment) {
				fprintf(stderr, "Stray code-block beginning on line %lu?\n",
					(unsigned long)sorter.token.line);
				ScannerIgnoreBlock();
				continue;
			}
			sorter.segment->section = Marker(&sorter.segment->code);
			if(sorter.segment->section == FUNCTION) ScannerIgnoreBlock();
			continue;

		default: break;
		}
		/* If it's the first line of code that is greater than some reasonable
		 distance to the documentation, split it up. */
		/* if(code && !already_code && lines_since_doc) cut; */
		if(sorter.segment && !sorter.info.is_doc
			&& !TokenArraySize(&sorter.segment->code)
			&& sorter.info.is_doc_far) printf("<cut>\n"), sorter_end_segment();

		/* Create new segment if need be. */
		if(!sorter.segment) {
			printf("<New segment>\n");
			if(!(sorter.segment = SegmentArrayNew(&segments)))
				{ sorter_err(); goto catch; }
			segment_init(sorter.segment);
		}
		/* Choose the token array. */
		if(sorter.info.is_doc) {
			if(symbol_mark[sorter.token.symbol] == '@') {
				printf("@tag %s\n", symbols[sorter.token.symbol]);
				if(!(sorter.tag = TagArrayNew(&sorter.segment->tags)))
					{ sorter_err(); goto catch; }
				tag_init(sorter.tag);
				sorter.tokens = &sorter.tag->contents;
				continue;
			} else if(!sorter.tokens
				|| sorter.tokens == &sorter.segment->code) {
				/* fixme: new doc, new paragraph. */
				sorter.tokens = &sorter.segment->doc;
			}
		} else { /* !is_doc */
			sorter.tokens = &sorter.segment->code;
		}
		{ /* Push symbol. */
			struct Token *token;
			if(!(token = TokenArrayNew(sorter.tokens)))
				{ sorter_err(); goto catch; }
			ScannerToken(token);
		}
		/* Create another segment next time. */
		if(sorter.is_differed_cut) sorter_end_segment();
	}

	/* Finished the compilation unit, the indent level should be zero. */
	if(ScannerTokenInfo(&sorter.info), sorter.info.indent_level) {
		fprintf(stderr, "EOF indent level %d.\n", sorter.info.indent_level);
		errno = EILSEQ;
		goto catch;
	}

	/* Cull. Rid uncommented blocks. Whitespace clean-up, (after!) */
	{
		struct Segment *segment;
		fputs("\n\n -- Edit: --\n", stdout);
		printf("segments size %lu.\n", SegmentArraySize(&segments));
		segment = 0;
		while((segment = SegmentArrayNext(&segments, segment)))
			printf("segment: %s\n", sections[segment->section]);

		/* Removes the stuff that we don't care about. */
		SegmentArrayKeepIf(&segments, &keep_segment);

		/* Cleans out the whitespace. */
		segment = 0;
		while((segment = SegmentArrayNext(&segments, segment))) {
			struct Tag *tag = 0;
			clean_whitespace(&segment->doc);
			while((tag = TagArrayNext(&segment->tags, tag))) {
				clean_whitespace(&tag->header);
				clean_whitespace(&tag->contents);
			}
		}

		fputs("\n -- Print out: --\n", stdout);
		printf("segments size %lu.\n", SegmentArraySize(&segments));
		segment = 0;
		while((segment = SegmentArrayNext(&segments, segment)))
			printf("segment: %p %s\n", (void *)segment,
			sections[segment->section]);
		segment = 0;
		while((segment = SegmentArrayNext(&segments, segment))) {
			struct Tag *tag;
			printf("Segment(%s):\n\tdoc: %s.\n\tcode: %s.\n",
				sections[segment->section], TokenArrayToString(&segment->doc),
				TokenArrayToString(&segment->code));
			tag = 0;
			while((tag = TagArrayNext(&segment->tags, tag))) {
				printf("\t%s{%s} %s.\n", symbols[tag->token.symbol],
					TokenArrayToString(&tag->header),
					TokenArrayToString(&tag->contents));
			}
		}
		fputc('\n', stdout);
	}

	printf("  -- Output: --\n\n");
	/* Now output. */
	out(&segments);

	is_done = 1;
	goto finally;

catch:
	perror("Cdoc");

finally:
	Marker(0);
	segment_array_remove(&segments);
	Scanner_();
	TokenArray_(&paragraph);

	return is_done ? EXIT_SUCCESS : EXIT_FAILURE;
}
