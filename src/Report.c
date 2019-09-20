/** 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 Organises tokens into sections, each section can have some documentation,
 code, and maybe attributes. */

#include <string.h> /* size_t strncpy sprintf */
#include <limits.h> /* INT_MAX */
#include <stdio.h>  /* sprintf */
#include "Division.h"
#include "Scanner.h"
#include "Semantic.h"
#include "Report.h"


/** `Token` has a `Symbol` and is associated with an area of the text. */
struct Token {
	enum Symbol symbol;
	const char *from;
	int length;
	size_t line;
};
static void token_to_string(const struct Token *t, char (*const a)[12]) {
	switch(t->symbol) {
	case WORD: { int len = t->length >= 9 ? 9 : t->length;
		sprintf(*a, "<%.*s>", len, t->from); break; }
	case ID: { int len = t->length >= 8 ? 8 : t->length;
		sprintf(*a, "ID:%.*s", len, t->from); break; }
	case SPACE: { (*a)[0] = '~', (*a)[1] = '\0'; break; }
	default:
		strncpy(*a, symbols[t->symbol], sizeof *a - 1);
		(*a)[sizeof *a - 1] = '\0';
	}
}
static int token_compare(const struct Token *const a,
	const struct Token *const b) {
	if(a->length != b->length) return *a->from - *b->from;
	return strncmp(a->from, b->from, a->length);
}
#define ARRAY_NAME Token
#define ARRAY_TYPE struct Token
#define ARRAY_TO_STRING &token_to_string
#include "Array.h"
/** This is used in `Semantic.c.re` to get the size of the string for
 `tokens`. */
size_t TokensMarkSize(const struct TokenArray *const tokens) {
	if(!tokens) return 0;
	return TokenArraySize(tokens) + 1; /* Fixme: overflow? */
}
/** @param[tokens] The `TokenArray` that converts to a string.
 @param[marks] Must be an at-least <fn:TokensMarkSize> buffer, or null.
 @return The size of the string, including null. */
void TokensMark(const struct TokenArray *const tokens, char *const marks) {
	struct Token *token = 0;
	char *mark = marks;
	if(!marks) return;
	while((token = TokenArrayNext(tokens, token)))
		*mark++ = symbol_marks[token->symbol];
	*mark = '\0';
	assert((size_t)(mark - marks) == TokenArraySize(tokens));
}


static void tokenref_to_string(struct Token *const*pt, char (*const a)[12]) {
	token_to_string(*pt, a);
}
#define ARRAY_NAME TokenRef
#define ARRAY_TYPE struct Token *
#define ARRAY_TO_STRING &tokenref_to_string
#include "Array.h"


/** `Attribute` is a specific structure of array of `Token` representing
 each-attributes, "@param ...". */
struct Attribute {
	enum Symbol symbol;
	struct TokenArray header;
	struct TokenArray contents;
};
static void attribute_to_string(const struct Attribute *t, char (*const a)[12])
{
	strncpy(*a, symbols[t->symbol], sizeof *a - 1);
	(*a)[sizeof *a - 1] = '\0';
}
#define ARRAY_NAME Attribute
#define ARRAY_TYPE struct Attribute
#define ARRAY_TO_STRING &attribute_to_string
#include "../src/Array.h"
static void attributes_(struct AttributeArray *const atts) {
	struct Attribute *a;
	if(!atts) return;
	while((a = AttributeArrayPop(atts)))
		TokenArray_(&a->header), TokenArray_(&a->contents);
	AttributeArray_(atts);
}

/** `Segment` is classified to a section of the document and can have
 documentation including attributes and code. */
struct Segment {
	char name[6];
	enum Division division;
	struct TokenArray doc, code;
	struct TokenRefArray params;
	struct AttributeArray attributes;
};
static void segment_to_string(const struct Segment *seg, char (*const a)[12]) {
	strncpy(*a, divisions[seg->division], sizeof *a - 1);
	(*a)[sizeof *a - 1] = '\0';
}
#define ARRAY_NAME Segment
#define ARRAY_TYPE struct Segment
#define ARRAY_TO_STRING &segment_to_string
#include "../src/Array.h"

/** Top-level static document. */
static struct SegmentArray report;



/** Destructor for the static document. Also destucts the string used for
 tokens. */
void Report_(void) {
	struct Segment *segment;
	/* Destroy the report. */
	while((segment = SegmentArrayPop(&report)))
		TokenArray_(&segment->doc), TokenArray_(&segment->code),
		TokenRefArray_(&segment->params), attributes_(&segment->attributes);
	SegmentArray_(&report);
	/* Destroy the semantic buffer. */
	Semantic(0);
}

/** @return A new empty segment, defaults to the preamble, or null on error. */
static struct Segment *new_segment(void) {
	struct Segment *segment;
	if(!(segment = SegmentArrayNew(&report))) return 0;
	{ /* Debug. */
		char *n;
		for(n = segment->name; n < segment->name + 6; n++)
			*n = 'a' + rand() / (RAND_MAX / ('z' - 'a' + 1) + 1);
		*n = '\0';
	}
	segment->division = DIV_PREAMBLE; /* Default. */
	TokenArray(&segment->doc);
	TokenArray(&segment->code);
	TokenRefArray(&segment->params);
	AttributeArray(&segment->attributes);
	return segment;
}

/** @return A new `Attribute` on `segment` with `symbol`, (should be a
 attribute symbol.) Null on error. */
static struct Attribute *new_attribute(struct Segment *const
	segment, const enum Symbol symbol) {
	struct Attribute *att;
	assert(segment);
	if(!(att = AttributeArrayNew(&segment->attributes))) return 0;
	att->symbol = symbol;
	TokenArray(&att->header);
	TokenArray(&att->contents);
	return att;
}

/** Creates a new token from `tokens` and fills it with `symbol` and the
 most recent scanner location.
 @return Success. */
static int new_token(struct TokenArray *const tokens, const enum Symbol symbol)
{
	struct Token *token;
	const char *const from = ScannerFrom(), *const to = ScannerTo();
	assert(tokens && from && from <= to);
	if(from + INT_MAX < to) return errno = EILSEQ, 0;
	if(!(token = TokenArrayNew(tokens))) return 0;
	token->symbol = symbol;
	token->from = from;
	token->length = (int)(to - from);
	token->line = ScannerLine();
	return 1;
}

/** Wrapper for `Sematic.h`; extracts semantic information from `segment`. */
static int semantic(struct Segment *const segment) {
	size_t no, i, entry;
	const size_t *array;
	struct Token *code;
	struct Token **ref;
	if(!segment) return 0;
	if(!Semantic(&segment->code)) return 0;
	segment->division = SemanticDivision();
	SemanticParams(&no, &array);
	code = TokenArrayGet(&segment->code);
	for(i = 0; i < no; i++) {
		entry = array[i];
		if(entry >= TokenArraySize(&segment->code)) {
			printf("Segment array index is out-of-range: %lu.\n",
				(unsigned long)i);
			continue;
		};
		if(!(ref = TokenRefArrayNew(&segment->params))) return 0;
		*ref = code + entry;
	}
	return 1;
}



/** This appends the current token based on the state it was last in.
 @return Success. */
int ReportPlace(void) {
	const enum Symbol symbol = ScannerSymbol();
	const int indent_level = ScannerIndentLevel();
	const char symbol_mark = symbol_marks[symbol];
	int is_differed_cut = 0;
	static struct {
		struct Segment *segment;
		struct Attribute *attribute;
		unsigned space, newline;
		int is_attribute_header, is_ignored_code, is_semantic_set;
	} sorter = { 0, 0, 0, 0, 0, 0, 0 }; /* This holds the sorting state. */
	/* These symbols require special consideration. */
	switch(symbol) {
	case DOC_BEGIN: /* Multiple doc comments. */
		sorter.attribute = 0;
		if(!sorter.segment) return 1;
		if(!TokenArraySize(&sorter.segment->code)) sorter.segment = 0,
			printf("----CUT in preamble----\n");
		return 1;
	case DOC_END: return 1; /* Doesn't actually do something. */
	case DOC_LEFT: /* Should only happen in @foo[]. */
		if(!sorter.segment || !sorter.attribute || sorter.is_attribute_header)
			return errno = EILSEQ, 0;
		sorter.is_attribute_header = 1;
		return 1;
	case DOC_RIGHT:
		if(!sorter.segment || !sorter.attribute || !sorter.is_attribute_header)
			return errno = EILSEQ, 0;
		sorter.is_attribute_header = 0;
		return 1;
	case DOC_COMMA:
		if(!sorter.segment || !sorter.attribute || !sorter.is_attribute_header)
			return errno = EILSEQ, 0;
		return 1; /* Doesn't actually do something. [,,] */
	case SPACE:   sorter.space++; return 1;
	case NEWLINE: sorter.newline++; return 1;
	case SEMI:
		if(indent_level != 0 || !sorter.segment) break;
		if(!sorter.is_semantic_set && !semantic(sorter.segment)) return 0;
		sorter.is_semantic_set = 1;
		is_differed_cut = 1;
		break;
	case LBRACE:
		if(indent_level != 1 || sorter.is_semantic_set) break;
		if(!semantic(sorter.segment)) return 0;
		sorter.is_semantic_set = 1;
		if(sorter.segment->division == DIV_FUNCTION)
			sorter.is_ignored_code = 1;
		break;
	case RBRACE: /* Functions don't have ';' to end them. */
		if(indent_level != 0) break;
		if(sorter.segment->division == DIV_FUNCTION) is_differed_cut = 1;
		break;
	default: break;
	}

	/* Make a new segment if needed. */
	if(!sorter.segment) {
		if(!(sorter.segment = new_segment())) return 0;
		sorter.attribute = 0;
		sorter.space = sorter.newline = 0;
		sorter.is_ignored_code = 0;
		assert(!sorter.is_attribute_header);
		sorter.is_semantic_set = 0;
	}

	/* Make a `token` where the context places us. */
	switch(symbol_mark) {
	case '~':
	{ /* This lazily places whitespace when other stuff is added. */
		struct TokenArray *selected = sorter.attribute
		? (sorter.is_attribute_header ? &sorter.attribute->header
		   : &sorter.attribute->contents) : &sorter.segment->doc;
		const int is_para = sorter.newline > 1,
		is_space = sorter.space || sorter.newline,
		is_doc_empty = !TokenArraySize(&sorter.segment->doc),
		is_selected_empty = !TokenArraySize(selected);
		sorter.space = sorter.newline = 0;
		if(is_para) {
			/* Switch out of attribute. */
			sorter.attribute = 0, sorter.is_attribute_header = 0;
			selected = &sorter.segment->doc;
			if(!is_doc_empty
			   && !new_token(&sorter.segment->doc, NEWLINE)) return 0;
		} else if(is_space) {
			if(!is_selected_empty
			   && !new_token(selected, SPACE)) return 0;
		}
		if(!new_token(selected, symbol)) return 0;
	}
		break;
	case '@':
		if(!(sorter.attribute = new_attribute(sorter.segment, symbol)))
			return 0;
		assert(!sorter.is_attribute_header);
		sorter.space = sorter.newline = 0; /* Also reset this for attributes. */
		break;
	default: /* Code. */
		if(sorter.is_ignored_code) {
			printf("Report::place: ignored.\n"); break; }
		if(!new_token(&sorter.segment->code, symbol)) return 0;
		break;
	}
	/* End the segment? */
	if(is_differed_cut) printf("----Differered CUT----\n"), sorter.segment = 0;
	return 1;
}

/* Output. */

/** Prints line info in a static buffer, (to be printed?) */
static const char *pos(const struct Token *const token) {
	static char p[128];
	const int max_size = 32,
		is_truncated = token->length > max_size ? 1 : 0,
		len = is_truncated ? max_size : token->length;
	assert(token);
	sprintf(p, "line %lu, %s: \"%.*s\"", (unsigned long)token->line,
		symbols[token->symbol], len, token->from);
	return p;
}

/** @implements{Predicate<Segment>} */
static int keep_segment(const struct Segment *const s) {
	struct Attribute *a = 0;
	printf("keep %s: ", TokenArrayToString(&s->code));
	if(TokenArraySize(&s->doc) || AttributeArraySize(&s->attributes)
		|| s->division == DIV_FUNCTION) {
		/* `static` and containing `@allow`. */
		if(TokenArraySize(&s->code)
			&& TokenArrayGet(&s->code)->symbol == STATIC) {
			while((a = AttributeArrayNext(&s->attributes, a))
				  && a->symbol != ATT_ALLOW);
			printf("static %s.\n", a ? "allowed" : "erased");
			return a ? 1 : 0;
		}
		printf("normal keep.\n");
		return 1;
	}
	printf("don't have doc, erased.\n");
	return 0;
}

/** Keeps only the stuff we care about; discards no docs except fn and `static`
 if not `@allow`. */
void ReportCull(void) {
	SegmentArrayKeepIf(&report, &keep_segment);
}

#include "ReportOut.h"
