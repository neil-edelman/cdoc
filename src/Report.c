/** 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 Organises tokens into sections, each section can have some documentation,
 code, and maybe attributes. */

#include <string.h> /* size_t strncpy sprintf */
#include <limits.h> /* INT_MAX */
#include "Division.h"
#include "Symbol.h"
#include "Report.h"

/** `Token` has a `Symbol` and is associated with an area of the text. */
struct Token {
	enum Symbol symbol;
	const char *from;
	int length;
	size_t line;
};
static void token_to_string(const struct Token *s, char (*const a)[12]) {
	/*int len = s->length >= 5 ? 5 : s->length;
	 sprintf(*a, "%.4s<%.*s>", symbols[s->symbol], len, s->from);*/
	strncpy(*a, symbols[s->symbol], sizeof *a - 1);
	(*a)[sizeof *a - 1] = '\0';
}
#define ARRAY_NAME Token
#define ARRAY_TYPE struct Token
#define ARRAY_TO_STRING &token_to_string
#include "Array.h"

/** `Attribute` is a specific structure of array of `Token` representing
 each-tags. */
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
 documentation including tags and code. */
struct Segment {
	enum Division division;
	struct TokenArray doc, code;
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

/** Destructor for the static document. */
void Report_(void) {
	struct Segment *segment;
	while((segment = SegmentArrayPop(&report)))
		TokenArray_(&segment->doc), TokenArray_(&segment->code),
		attributes_(&segment->attributes);
	SegmentArray_(&report);
}

/** @return A new empty segment, defaults to the preamble, or null on error. */
struct Segment *ReportNewSegment(void) {
	struct Segment *segment;
	if(!(segment = SegmentArrayNew(&report))) return 0;
	segment->division = DIV_PREAMBLE; /* Default. */
	TokenArray(&segment->doc);
	TokenArray(&segment->code);
	AttributeArray(&segment->attributes);
	return segment;
}

/** @return A new `Attribute` on `segment` with `symbol`, (should be a tag
 symbol.) */
struct Attribute *ReportSegmentNewAttribute(struct Segment *const
	segment, const enum Symbol symbol) {
	struct Attribute *att;
	assert(segment);
	if(!(att = AttributeArrayNew(&segment->attributes))) return 0;
	att->symbol = symbol;
	TokenArray(&att->header);
	TokenArray(&att->contents);
	return att;
}

/** @return Whether there is code in this `segment`. */
int ReportSegmentIsCode(const struct Segment *const segment) {
	if(!segment) return 0;
	return !!TokenArraySize(&segment->code);
}

/** @return Whether there is documentation in this `segment`. */
int ReportSegmentIsDoc(const struct Segment *const segment) {
	if(!segment) return 0;
	return !!TokenArraySize(&segment->doc);
}

/** Creates a new token of `symbol` from `segment`.
 @return Success. */
int ReportSegmentNewToken(struct Segment *const segment,
	const enum ReportSegmentWhere where, const enum Symbol symbol,
	const char *const from, const char *const to, const size_t line) {
	struct Attribute *recent;
	struct TokenArray *tokens = 0;
	struct Token *token;
	if(!segment) return 0;
	if(!from || from > to || from + INT_MAX < to) return errno = EILSEQ, 0;
	/* Choose `where`. */
	recent = AttributeArrayPeek(&segment->attributes);
	switch(where) {
	case WHERE_DOC: tokens = &segment->doc; break;
	case WHERE_CODE: tokens = &segment->code; break;
	case WHERE_ATT_HEADER:
		if(!recent) return errno = EILSEQ, 0;
		tokens = &recent->header; break;
	case WHERE_ATT_CONTENTS:
		if(!recent) return errno = EILSEQ, 0;
		tokens = &recent->contents; break;
	}
	assert(tokens);
	if(!(token = TokenArrayNew(tokens))) return 0;
	token->symbol = symbol;
	token->from = from;
	token->length = (int)(to - from);
	token->line = line;
	return 1;
}
