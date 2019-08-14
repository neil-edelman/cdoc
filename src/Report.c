/** 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 Organises tokens into sections, each section can have some documentation,
 code, and maybe attributes. */

#include <string.h> /* size_t strncpy sprintf */
#include <limits.h> /* INT_MAX */
#include <stdio.h>  /* sprintf */
#include "Division.h"
#include "Scanner.h"
#include "Parser.h"
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
#define ARRAY_NAME Token
#define ARRAY_TYPE struct Token
#define ARRAY_TO_STRING &token_to_string
#include "Array.h"

static void token_copy(struct Token *const dst, const struct Token *const src) {
	assert(dst && src);
	dst->symbol = src->symbol;
	dst->from   = src->from;
	dst->length = src->length;
	dst->line   = src->line;
}

/** Accessor for symbol required for `Parser.y`. */
enum Symbol TokenSymbol(const struct Token *const token) {
	if(!token) return END;
	return token->symbol;
}

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

/** Where is the current segment. */
static struct {
	enum Division division;
	int division_set;
	struct TokenArray params;
} current;

/** @return Whether this is a new division, reset with
 <fn:ReportCurrentReset>. */
int ReportCurrentDivision(const enum Division division) {
	int division_set = current.division_set;
	if(division_set) fprintf(stderr,
		"ReportCurrentDivision: division %s changed to %s.\n",
		divisions[current.division], divisions[division]);
	current.division = division;
	current.division_set = 1;
	return division_set;
}

/** @param Which token to add.
 @return Success. */
int ReportCurrentParam(const struct Token *const token) {
	struct Token *copy;
	if(!token) return 0;
	if(!(copy = TokenArrayNew(&current.params))) return 0;
	token_copy(copy, token);
	return 1;
}

/** Resets the current. */
static void current_reset(void) {
	current.division = DIV_PREAMBLE;
	current.division_set = 0;
	TokenArrayClear(&current.params);
}

/** Destructor for the static document. Also destucts the current. */
void Report_(void) {
	struct Segment *segment;
	/* Destroy the report. */
	while((segment = SegmentArrayPop(&report)))
		TokenArray_(&segment->doc), TokenArray_(&segment->code),
		attributes_(&segment->attributes);
	SegmentArray_(&report);
	/* Destroy the rest. */
	ReportCurrentReset();
	TokenArray_(&current.params);
}

/** @return A new empty segment, defaults to the preamble, or null on error. */
static struct Segment *new_segment(void) {
	struct Segment *segment;
	if(!(segment = SegmentArrayNew(&report))) return 0;
	segment->division = DIV_PREAMBLE; /* Default. */
	TokenArray(&segment->doc);
	TokenArray(&segment->code);
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

/** This calls the parser with `tokens`. */
static void parse(const struct TokenArray *const tokens) {
	struct Token *token = 0;
	assert(tokens);
	printf("Parsing: ");
	current_reset();
	while((token = TokenArrayNext(tokens, token)))
		printf("%s, ", symbols[token->symbol]);
	printf("END.\n");
	while((token = TokenArrayNext(tokens, token))) ParserToken(token);
	ParserToken(0);
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
		int is_attribute_header, is_ignored_code;
	} sorter = { 0, 0, 0, 0, 0, 0 }; /* This holds the sorting state. */

	/* These symbols require special consideration. */
	switch(symbol) {
	case DOC_BEGIN: /* Multiple doc comments. */
		sorter.attribute = 0;
		if(!sorter.segment) return 1;
		if(!TokenArraySize(&sorter.segment->code)) sorter.segment = 0;
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
		return 1; /* Doesn't actually do something. */
	case SPACE:   sorter.space++; return 1;
	case NEWLINE: sorter.newline++; return 1;
	case SEMI:
		if(indent_level != 0) break;
		parse(&sorter.segment->code);
		sorter.segment->division = current.division;
		sorter.is_ignored_code = 1;
		is_differed_cut = 1;
		break;
	case LBRACE:
		if(indent_level != 1) break;
		parse(&sorter.segment->code);
		sorter.segment->division = current.division;
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
		break;
	default:
		if(sorter.is_ignored_code) goto differed;
		if(!new_token(&sorter.segment->code, symbol)) return 0;
		break;
	}
	
differed:
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

/** Keeps only the stuff we care about.
 @implements{Predicate<Segment>} */
static int keep_segment(const struct Segment *const s) {
	/* fixme: and not static or containing @allow att. */
	if(TokenArraySize(&s->doc) || s->division == DIV_FUNCTION) return 1;
	return 0;
}

/** Selects `token` out of `ta` and prints it and returns the next token. */
typedef const struct Token *(*OutFn)(const struct TokenArray *const ta,
	const struct Token *const token);

/* @implements <Attribute>Predicate */
#define OUT(name) static const struct Token *name(const struct TokenArray \
*const ta, const struct Token *const token)
OUT(lit) {
	printf("%.*s~", token->length, token->from);
	return TokenArrayNext(ta, token);
}
OUT(gen1) {
	struct Token *const lparen = TokenArrayNext(ta, token),
	*const param = TokenArrayNext(ta, lparen),
	*const rparen = TokenArrayNext(ta, param);
	const char *a, *type;
	int type_size;
	if(!lparen || lparen->symbol != LPAREN || !param || !rparen
	   || rparen->symbol != RPAREN) goto catch;
	type = token->from;
	if(!(a = strchr(type, '_'))) goto catch;
	type_size = (int)(a - type);
	assert(token->length == a + 1 - token->from);
	printf("<%.*s>%.*s~",
		   token->length - 1, token->from, param->length, param->from);
	return TokenArrayNext(ta, rparen);
catch:
	fprintf(stderr, "Expected: generic(id);\n%s.\n", pos(token));
	return 0;
}
OUT(gen2) {
	struct Token *const lparen = TokenArrayNext(ta, token),
	*const param1 = TokenArrayNext(ta, lparen),
	*const comma = TokenArrayNext(ta, param1),
	*const param2 = TokenArrayNext(ta, comma),
	*const rparen = TokenArrayNext(ta, param2);
	const char *a, *type1, *type2;
	int type1_size, type2_size;
	if(!lparen || lparen->symbol != LPAREN || !param1 || !comma
	   || comma->symbol != COMMA || !param2 || !rparen
	   || rparen->symbol != RPAREN) goto catch;
	type1 = token->from;
	if(!(a = strchr(type1, '_'))) goto catch;
	type1_size = (int)(a - type1);
	type2 = a + 1;
	if(!(a = strchr(type2, '_'))) goto catch;
	type2_size = (int)(a - type2);
	assert(token->length == a + 1 - token->from);
	printf("<%.*s>%.*s<%.*s>%.*s~", type1_size, type1, param1->length,
		   param1->from, type2_size, type2, param2->length, param2->from);
	return TokenArrayNext(ta, rparen);
catch:
	fprintf(stderr, "Expected: generic(id,id);\n%s.\n", pos(token));
	return 0;
}
OUT(gen3) {
	struct Token *const lparen = TokenArrayNext(ta, token),
	*const param1 = TokenArrayNext(ta, lparen),
	*const comma1 = TokenArrayNext(ta, param1),
	*const param2 = TokenArrayNext(ta, comma1),
	*const comma2 = TokenArrayNext(ta, param2),
	*const param3 = TokenArrayNext(ta, comma2),
	*const rparen = TokenArrayNext(ta, param3);
	const char *a, *type1, *type2, *type3;
	int type1_size, type2_size, type3_size;
	if(!lparen || lparen->symbol != LPAREN || !param1 || !comma1
	   || comma1->symbol != COMMA || !param2 || !comma2 ||
	   comma2->symbol != COMMA || !param3 || !rparen
	   || rparen->symbol != RPAREN) goto catch;
	type1 = token->from;
	if(!(a = strchr(type1, '_'))) goto catch;
	type1_size = (int)(a - type1);
	type2 = a + 1;
	if(!(a = strchr(type2, '_'))) goto catch;
	type2_size = (int)(a - type2);
	type3 = a + 1;
	if(!(a = strchr(type3, '_'))) goto catch;
	type3_size = (int)(a - type3);
	assert(token->length == a + 1 - token->from);
	printf("<%.*s>%.*s<%.*s>%.*s<%.*s>%.*s~", type1_size, type1,
		   param1->length, param1->from, type2_size, type2, param2->length,
		   param2->from, type3_size, type3, param3->length, param3->from);
	return TokenArrayNext(ta, rparen);
catch:
	fprintf(stderr, "Expected: generic(id,id,id);\n%s.\n", pos(token));
	return 0;
}
OUT(esc_bs) {
	printf("\\~");
	return TokenArrayNext(ta, token);
}
OUT(url) {
	struct Token *const lbr = TokenArrayNext(ta, token),
	*next = TokenArrayNext(ta, lbr); /* Variable no. */
	if(!lbr || lbr->symbol != LBRACE || !next) goto catch;
	printf("(");
	while(next->symbol != RBRACE) {
		/* We don't care about the symbol's meaning in the url. */
		printf("%.*s", next->length, next->from);
		if(!(next = TokenArrayNext(ta, next))) goto catch;
	}
	printf(")~");
	return TokenArrayNext(ta, next);
catch:
	fprintf(stderr, "Expected: \\url{<cat url>};\n%s.\n", pos(token));
	return 0;
}
OUT(cite) {
	struct Token *const lbr = TokenArrayNext(ta, token),
	*next = TokenArrayNext(ta, lbr); /* Variable no. */
	if(!lbr || lbr->symbol != LBRACE || !next) goto catch;
	printf("(");
	while(next->symbol != RBRACE) {
		printf("%.*s~", next->length, next->from);
		if(!(next = TokenArrayNext(ta, next))) goto catch;
	}
	printf(")[https://scholar.google.ca/scholar?q=");
	next = TokenArrayNext(ta, lbr);
	while(next->symbol != RBRACE) {
		/* fixme: escape url! */
		printf("%.*s_", next->length, next->from);
		if(!(next = TokenArrayNext(ta, next))) goto catch;
	}
	printf("]~");
	return TokenArrayNext(ta, next);
catch:
	fprintf(stderr, "Expected: \\cite{<source>};\n%s.\n", pos(token));
	return 0;
}
OUT(see) { /* fixme: Have a new field in segment. */
	printf("(fixme)\\see");
	return TokenArrayNext(ta, token);
}
OUT(math) { /* Math and code. */
	struct Token *next = TokenArrayNext(ta, token);
	printf("{code:`");
	while(next->symbol != MATH_END) {
		printf("%.*s", next->length, next->from);
		if(!(next = TokenArrayNext(ta, next))) goto catch;
	}
	printf("`:code}~");
	return TokenArrayNext(ta, next);
catch:
	fprintf(stderr, "Expected: `<math/code>`;\n%s.\n", pos(token));
	return 0;
}
OUT(em) {
	struct Token *next = TokenArrayNext(ta, token);
	printf("{em:`");
	while(next->symbol != EM_END) {
		printf("%.*s~", next->length, next->from);
		if(!(next = TokenArrayNext(ta, next))) goto catch;
	}
	printf("`:em}~");
	return TokenArrayNext(ta, next);
catch:
	fprintf(stderr, "Expected: _<emphasis>_;\n%s.\n", pos(token));
	return 0;
}
OUT(par) {
	printf("^\n^\n");
	return TokenArrayNext(ta, token);
}

/* `SYMBOL` is declared in `Scanner.h` and `PARAM3_C` is one of the preceding
 functions. */
static const OutFn symbol_out[] = { SYMBOL(PARAM3_C) };

static void tokens_print(const struct TokenArray *const ta) {
	const struct Token *token = TokenArrayNext(ta, 0);
	OutFn sym_out;
	if(!token) return;
	while((sym_out = symbol_out[token->symbol])
		  && (token = sym_out(ta, token)));
	fputc('\n', stdout);
}

/** @implements <Attribute>Action */
static void print_att_contents(struct Attribute *const att) {
	tokens_print(&att->contents);
}

/** @implements <Attribute>Action */
static void print_att_header(struct Attribute *const att) {
	tokens_print(&att->header);
}

/** @implements <Attribute>Action */
static void print_att_header_contents(struct Attribute *const att) {
	printf("<att:%s # ", symbols[att->symbol]);
	print_att_header(att);
	printf(" #\n");
	print_att_contents(att);
	printf(">\n");
}

/* @implements <Attribute>Predicate */
#define ATT_IS(lc, uc) static int att_is_ ## lc \
(const struct Attribute *const att) { return att->symbol == uc; }
ATT_IS(title, ATT_TITLE)
ATT_IS(param, ATT_PARAM)
ATT_IS(author, ATT_AUTHOR)
ATT_IS(std, ATT_STD)
ATT_IS(depend, ATT_DEPEND)
/*ATT_IS(version, ATT_VERSION)
 ATT_IS(since, ATT_SINCE)
 ATT_IS(fixme, ATT_FIXME)
 ATT_IS(depricated, ATT_DEPRICATED)
 ATT_IS(return, ATT_RETURN)
 ATT_IS(throws, ATT_THROWS)
 ATT_IS(implements, ATT_IMPLEMENTS)
 ATT_IS(order, ATT_ORDER)
 ATT_IS(allow, ATT_ALLOW)*/

/** @implements <Segment>Action */
static void segment_print_doc(struct Segment *const segment) {
	tokens_print(&segment->doc);
}

/** @implements <Segment>Action */
static void segment_print_code(struct Segment *const segment) {
	tokens_print(&segment->code);
	printf("\n");
}

/** @implements <Segment>Action */
static void segment_print_all(struct Segment *const segment) {
	segment_print_code(segment);
	segment_print_doc(segment);
	AttributeArrayIfEach(&segment->attributes, &att_is_author, &print_att_contents);
	AttributeArrayIfEach(&segment->attributes, &att_is_std, &print_att_contents);
	AttributeArrayIfEach(&segment->attributes, &att_is_depend, &print_att_contents);
	AttributeArrayIfEach(&segment->attributes, &att_is_param, &print_att_header_contents);
	printf("\n\n***\n\n");
}

/** @implements <Segment>Action */
static void segment_print_all_title(struct Segment *const segment) {
	AttributeArrayIfEach(&segment->attributes, &att_is_title, &print_att_contents);
}

/** @implements <Segment>Predictate */
static int segment_is_header(const struct Segment *const segment) {
	return segment->division == DIV_PREAMBLE;
}

/** @implements <Segment>Predictate */
static int segment_is_declaration(const struct Segment *const segment) {
	return segment->division == DIV_TAG || segment->division == DIV_TYPEDEF;
}

/** @implements <Segment>Predictate */
static int segment_is_function(const struct Segment *const segment) {
	return segment->division == DIV_FUNCTION;
}

void ReportDebug(void) {
	struct Segment *segment = 0;
	struct Attribute *att = 0;
	while((segment = SegmentArrayNext(&report, segment))) {
		printf("Segment(%s):\n\tdoc: %s.\n\tcode: %s.\n",
			divisions[segment->division],
			TokenArrayToString(&segment->doc),
			TokenArrayToString(&segment->code));
		while((att = AttributeArrayNext(&segment->attributes, att)))
			printf("\t%s{%s} %s.\n", symbols[att->symbol],
			TokenArrayToString(&att->header),
			TokenArrayToString(&att->contents));
		fputc('\n', stdout);
	}
}

/** Outputs a file when given a `SegmentArray`. */
void ReportOut(void) {
	/* Print header. */
	printf("<header:title>\n # ");
	SegmentArrayIfEach(&report, &segment_is_header, &segment_print_all_title);
	printf(" #\n\n");
	printf("<header:doc>\n");
	SegmentArrayIfEach(&report, &segment_is_header, &segment_print_doc);
	/* Print typedefs. */
	printf("\n\n## Typedefs ##\n");
	/* Print tags. */
	printf("\n\n## Tags ##\n");
	/* Print general declarations. */
	printf("\n\n## Declarations ##\n\n");
	SegmentArrayIfEach(&report, &segment_is_declaration, &segment_print_all);
	printf("\n\n## Functions ##\n\n");
	SegmentArrayIfEach(&report, &segment_is_function, &segment_print_code);
	printf("\n\n## Function Detail ##\n\n");
	SegmentArrayIfEach(&report, &segment_is_function, &segment_print_all);
}
