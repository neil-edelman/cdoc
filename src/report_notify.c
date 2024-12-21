/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 After one has done the parsing, this organizes the report for output to the
 user. */

#include "division.h"
#include "format.h"
#include "semantic.h"
#include "buffer.h"
#include "style.h"
#include "image_dimension.h"
#include "cdoc.h"
#include "report.h"
#include <string.h> /* size_t strncpy strncmp */
#include <limits.h> /* INT_MAX for printf */
#include <stdio.h>  /* printf */

#include <stdlib.h>
#include <assert.h>

#if defined __GNUC__ || defined __MINGW32__ || defined __clang__
__attribute__((noreturn))
#elif _MSC_VER
__declspec(noreturn)
#endif
/** Ludicrous. fixme: putting them in we check, this should be an assert. */
static void unrecoverable(void) {
	fprintf(stderr, "report: couldn't write file because it is too big.\n");
	assert(0), exit(EXIT_FAILURE);
}

/* So many parameters `is_buffer`. It's easier to just have a static. */
static int report_is_buffer;

/** This is used in `semantic.c.re` to get the first file:line for error. */
const char *tokens_first_label(const struct token_array *const tokens)
	{ return tokens->size ? tokens->data[0].label : "unlabelled"; }
size_t tokens_first_line(const struct token_array *const tokens)
	{ return tokens->size ? tokens->data[0].line : 0; }
/** This is used in `semantic.c.re` to get the size of the string for
 `tokens`. */
size_t tokens_mark_size(const struct token_array *const tokens) {
	if(!tokens) return 0;
	return tokens->size + 1;
}
/** @param[tokens] The `token_array` that converts to a string.
 @param[marks] Must be an at-least the size of `tokens`, or null.
 @return The size of the string, including null. */
void tokens_mark(const struct token_array *const tokens, char *mark) {
	size_t i;
	assert(mark);
	for(i = 0; i < tokens->size; i++)
		*mark++ = symbol_marks[tokens->data[i].symbol];
	*mark = '\0';
}


static void index_to_string(const size_t *const n, char (*const a)[12]) {
	sprintf(*a, "%lu", (unsigned long)*n % 1000000000lu);
}
/*#define ARRAY_NAME index
#define ARRAY_TYPE size_t
#define ARRAY_TO_STRING
#include "boxes/array.h"*/
#include "index_array.h"

/** `Attribute` is a specific structure of array of `Token` representing
 each-attributes, "\@param ...". */
struct attribute {
	struct token token;
	struct token_array header;
	struct token_array contents;
};
static void attribute_to_string(const struct attribute *t, char (*const a)[12])
{
	strncpy(*a, symbols[t->token.symbol], sizeof *a - 1);
	(*a)[sizeof *a - 1] = '\0';
}
#define ARRAY_NAME attribute
#define ARRAY_TYPE struct attribute
#define ARRAY_TO_STRING
#include "boxes/array.h"
static void attributes_(struct attribute_array *const atts) {
	struct attribute *a;
	if(!atts) return;
	while((a = attribute_array_pop(atts)))
		token_array_(&a->header), token_array_(&a->contents);
	attribute_array_(atts);
}


/** Classified to a section of the document and can have documentation
 including attributes and code. */
struct segment {
	enum division division;
	struct token_array doc, code;
	struct index_array code_params;
	struct attribute_array attributes;
};
/** Provides a default token for `segment` to print. */
static const struct token *segment_fallback(const struct segment *const segment,
	const struct token_array **const ta_ptr) {
	const struct token_array *ta = 0;
	const struct token *t = 0;
	assert(segment);
	if(segment->code_params.size) {
		const size_t i = segment->code_params.data[0];
		ta = &segment->code;
		assert(i < ta->size);
		t = ta->data + i;
	} else if(segment->code.size) {
		ta = &segment->code;
		t = ta->data;
	} else if(!ta_ptr && segment->doc.size) {
		/* /\ Raw pointers in the text are problematic since maybe we will
		 convert it to a string and most text does not support that. */
		ta = &segment->doc;
		t = ta->data;
	} else if(!ta_ptr && segment->attributes.size) {
		ta = &segment->doc;
		t = &segment->attributes.data->token;
	}
	if(ta_ptr) *ta_ptr = ta;
	return t;
	/*return index_array_size(&segment->code_params)
		? segment->code.data
		+ segment->code_params.data[0]
		: token_array_size(&segment->code) ? segment->code.data
		: token_array_size(&segment->doc) ? segment->doc.data
		: attribute_array_size(&segment->attributes)
		? &attribute_array_get(&segment->attributes)->token : 0;*/
}

/* For <fn:segment_to_string>. */
static int print_token_s(struct token_array_cursor *const tok,
	const char **fill_buffer);

static void segment_to_string(const struct segment *segment,
	char (*const a)[12]) {
	const struct token_array *ta;
	const struct token *const fallback = segment_fallback(segment, &ta);
	const char *temp = division[segment->division].symbol;
	size_t temp_len, i = 0;
	if(fallback) {
		style_push(ST_TO_RAW);
		temp = print_token_s(ta, fallback);
		style_pop();
	}
	temp_len = strlen(temp);
	if(temp_len > sizeof *a - 3) temp_len = sizeof *a - 3;
	(*a)[i++] = 'S';
	(*a)[i++] = '_';
	memcpy(*a + i, temp, temp_len);
	i += temp_len;
	(*a)[i++] = '\0';
	assert(i <= sizeof *a);
}
static void erase_segment(struct segment *const segment) {
	char a[12];
	assert(segment);
	segment_to_string(segment, &a);
	segment->division = DIV_PREAMBLE;
	token_array_(&segment->doc);
	token_array_(&segment->code);
	if(cdoc_get_debug() & DBG_ERASE && segment->code_params.size)
		fprintf(stderr, "*** Erasing %s: %s.\n",
		a, index_array_to_string(&segment->code_params));
	index_array_(&segment->code_params);
	attributes_(&segment->attributes);
}
#define ARRAY_NAME segment
#define ARRAY_TYPE struct segment
#define ARRAY_TO_STRING
#include "boxes/array.h"
/*static void segment_array_clear(struct segment_array *const sa) {
	struct segment *segment;
	if(!sa) return;
	while((segment = segment_arrayPop(sa)))
		token_array_Clear(&segment->doc), token_array_Clear(&segment->code),
		Indexarray_clear(&segment->code_params),
		attributes_(&segment->attributes);
}*/
static const struct token *param_no(const struct segment *const segment,
	const size_t param) {
	size_t *pidx;
	assert(segment);
	if(param >= segment->code_params.size) return 0;
	pidx = segment->code_params.data + param;
	/* This is really careful. */
	if(*pidx >= segment->code.size) {
		char a[12];
		segment_to_string(segment, &a);
		fprintf(stderr, "%s: param index %lu is greater then code size.\n",
			a, (unsigned long)segment->code.size);
		return 0;
	}
	return segment->code.data + *pidx;
}



/** Top-level static document. */
static struct segment_array report;
static struct token_array brief;



/** Destructor for the static document. Also destucts the string used for
 tokens. */
void report_(void) {
	struct segment *segment;
	token_array_(&brief);
	while(segment = segment_array_pop(&report)) erase_segment(segment);
	segment_array_(&report);
	semantic(0);
	style_();
}

/** @return A new empty segment from `segments`, defaults to the preamble, or
 null on error. */
static struct segment *new_segment(struct segment_array *const segments) {
	struct segment *segment;
	assert(segments);
	if(!(segment = segment_array_new(segments))) return 0;
	segment->division = DIV_PREAMBLE; /* Default. */
	segment->doc = token_array();
	segment->code = token_array();
	segment->code_params = index_array();
	segment->attributes = attribute_array();
	return segment;
}

/** Initializes `token` with `scan`. @return Success.
 @throws[EILSEQ] The token cannot be represented as an `int` offset. */
static int init_token(struct token *const token,
	const struct scanner *const scan) {
	const char *const from = scanner_from(scan), *const to = scanner_to(scan);
	assert(scan && token && from && from <= to);
	if(from + INT_MAX < to) return errno = EILSEQ, 0;
	token->symbol = scanner_symbol(scan);
	token->from = from;
	token->length = (size_t)(to - from);
	token->label = scanner_label(scan);
	token->line = scanner_line(scan);
	return 1;
}

/** @return A new `Attribute` on `segment` with `symbol`, (should be a
 attribute symbol.) Null on error. */
static struct attribute *new_attribute(struct segment *const segment,
	const struct scanner *const scan) {
	struct attribute *att;
	assert(scan && segment);
	if(!(att = attribute_array_new(&segment->attributes))) return 0;
	init_token(&att->token, scan);
	att->header = token_array();
	att->contents = token_array();
	return att;
}

/** Creates a new token from `tokens` and fills it with `symbol` and the
 most recent scanner location.
 @return Token or failure. */
static struct token *new_token(struct token_array *const tokens,
	const struct scanner *const scan) {
	struct token *token;
	if(!(token = token_array_new(tokens))) return 0;
	init_token(token, scan);
	/*fprintf(stderr, "new_token: %s %.*s\n", symbols[token->symbol],
		token->length, token->from); <- If one really wants spam. */
	return token;
}

/** Wrapper for `semantic.h`; extracts semantic information from `segment`. */
static int report_semantic(struct segment *const segment) {
	size_t no, i;
	const size_t *source;
	size_t *dest;
	if(!segment) return 0;
	if(!semantic(&segment->code)) return 0;
	segment->division = semantic_division();
	/* Copy `semantic` size array to this size array,
	 (not the same, local scope; kind of a hack.) */
	semantic_params(&no, &source);
	if(!no) return 1; /* We will cull them later. */
	if(!(dest = index_array_append(&segment->code_params, no))) return 0;
	for(i = 0; i < no; i++) dest[i] = source[i];
	if(cdoc_get_debug() & DBG_ERASE) {
		char a[12];
		segment_to_string(segment, &a);
		fprintf(stderr, "*** Adding %lu to %s: %s.\n",
			(unsigned long)no, a, index_array_to_string(&segment->code_params));
	}
	return 1;
}



/** Prints `segment` to `stderr`. */
static void print_segment_debug(const struct segment *const segment) {
	struct token *doc, *code;
	size_t i = 0;
	if(!(cdoc_get_debug() & DBG_OUTPUT)) return;
	code = segment->code.size ? segment->code.data + 0 : 0;
	doc  = segment->doc.size ? segment->doc.data + 0 : 0;
	fprintf(stderr, "segment_ division %s:\n"
		"%s:%lu code: %s;\n"
		"of which params: %s;\n"
		"%s:%lu doc: %s.\n",
		division[segment->division].symbol,
		code ? code->label : "N/A", code ? code->line : 0,
		token_array_to_string(&segment->code),
		index_array_to_string(&segment->code_params),
		doc ? doc->label : "N/A", doc ? doc->line : 0,
		token_array_to_string(&segment->doc));
	while(i < segment->attributes.size) {
		const struct attribute *const att = segment->attributes.data + i;
		fprintf(stderr, "%s{%s} %s.\n", symbols[att->token.symbol],
			token_array_to_string(&att->header),
			token_array_to_string(&att->contents));
	}
}

/** Helper for next. Note that if it stops on a preamble command, this is not
 printed. */
static void cut_segment_here(struct segment **const psegment) {
	const struct segment *segment = 0;
	assert(psegment);
	if(!(segment = *psegment)) return;
	print_segment_debug(segment);
	*psegment = 0;
}

/** Prints line info into a static buffer. */
static const char *oops(const struct scanner *const scan) {
	static char p[128];
	assert(scan);
	sprintf(p, "%.32s:%lu, %s", scanner_label(scan),
		(unsigned long)scanner_line(scan), symbols[scanner_symbol(scan)]);
	return p;
}

void report_last_segment_debug(void) {
	if(!report.size) return;
	print_segment_debug(report.data + report.size - 1);
}

/** This appends the current token based on the state it was last in.
 @return Success. */
int report_notify(const struct scanner *const scan) {
	const enum symbol symbol = scanner_symbol(scan);
	const char symbol_mark = symbol_marks[symbol];
	int is_differed_cut = 0;
	static struct {
		enum { SORT_CODE, SORT_DOC, SORT_ARGS } state;
		size_t last_doc_line;
		struct segment *segment;
		struct attribute *attribute;
		unsigned space, newline;
		int is_code_ignored, is_semantic_set;
	} sorter = { 0, 0, 0, 0, 0, 0, 0, 0 };
	/* These symbols require special consideration. */
	switch(symbol) {
	case DOC_BEGIN:
		if(sorter.state != SORT_CODE) return fprintf(stderr,
			"%s: sneak url; was expecting code.\n",
			oops(scan)), errno = EDOM, 0;
		sorter.state = SORT_DOC;
		/* Reset attribute. */
		sorter.attribute = 0;
		/* Two docs on top of each other without code, the top one belongs to
		 the preamble. */
		if(sorter.segment && !sorter.segment->code.size)
			cut_segment_here(&sorter.segment);
		return 1;
	case DOC_END:
		if(sorter.state != SORT_DOC) return fprintf(stderr,
			"%s: sneak url; was expecting doc.\n",
			oops(scan)), errno = EDOM, 0;
		sorter.state = SORT_CODE;
		sorter.last_doc_line = scanner_line(scan);
		return 1;
	case DOC_LEFT:
		if(sorter.state != SORT_DOC || !sorter.segment || !sorter.attribute)
			return fprintf(stderr,
			"%s: sneak url; was expecting doc with attribute.\n", oops(scan)),
			errno = EDOM, 0;
		sorter.state = SORT_ARGS;
		return 1;
	case DOC_RIGHT:
		if(sorter.state != SORT_ARGS || !sorter.segment || !sorter.attribute)
			return fprintf(stderr,
			"%s: sneak url; was expecting args with attribute.\n", oops(scan)),
			errno = EDOM, 0;
		sorter.state = SORT_DOC;
		return 1;
	case DOC_COMMA: /* @arg[,,] */
		if(sorter.state != SORT_ARGS || !sorter.segment || !sorter.attribute)
			return fprintf(stderr,
			"%s: sneak url; was expecting args with attribute.\n", oops(scan)),
			errno = EDOM, 0;
		return 1;
	case SPACE:   sorter.space++; return 1;
	case NEWLINE: sorter.newline++; return 1;
	case SEMI:
		/* Break on global semicolons only. */
		if(scanner_indent_level(scan) != 0 || !sorter.segment) break;
		/* Find out what this line means if one hasn't already. */
		if(!sorter.is_semantic_set && !report_semantic(sorter.segment)) return 0;
		sorter.is_semantic_set = 1;
		is_differed_cut = 1;
		break;
	case LBRACE:
		/* If it's a leading brace, see what the semantic says about it. */
		if(scanner_indent_level(scan) != 1 || sorter.is_semantic_set
			|| !sorter.segment) break;
		if(!report_semantic(sorter.segment)) return 0;
		sorter.is_semantic_set = 1;
		if(sorter.segment->division == DIV_FUNCTION) sorter.is_code_ignored = 1;
		break;
	case RBRACE:
		/* Functions don't have ';' to end them. */
		if(scanner_indent_level(scan) != 0 || !sorter.segment) break;
		if(sorter.segment->division == DIV_FUNCTION) is_differed_cut = 1;
		break;
	case LOCAL_INCLUDE: /* Include file. */
		assert(sorter.state == SORT_CODE);
		{
			const char *fn = 0;
			struct scanner *subscan = 0;
			struct text *text = 0;
			int success = 0;
			if(!(fn = url_from_here((size_t)(scanner_to(scan)
				- scanner_from(scan)), scanner_from(scan)))) goto include_catch;
			if(!(text = text_open(fn))) goto include_catch;
			cut_segment_here(&sorter.segment);
			if(!(subscan = scanner(text_base_name(text), text_get(text),
				&report_notify, START_CODE))) goto include_catch;
			cut_segment_here(&sorter.segment);
			success = 1;
			goto include_finally;
include_catch:
			fprintf(stderr, "%s: \"%s\" couldn't resolve name.\n",
				oops(scan), fn);
			/* if(errno) perror("including"); <- Handled farther up. */
include_finally:
			scanner_(&subscan);
			return success;
		}
	default: break;
	}

	/* Code that starts far away from docs goes in it's own segment. */
	if(sorter.segment && symbol_mark != '~' && symbol_mark != '@'
		&& !sorter.segment->code.size && sorter.last_doc_line
		&& sorter.last_doc_line + 2 < scanner_line(scan))
		cut_segment_here(&sorter.segment);

	/*if(sorter.segment && symbol_mark == 'm') {
		fprintf(stderr, "OHNO!!!\n");
	} wtf? */

	/* Make a new segment if needed. */
	if(!sorter.segment) {
		if(!(sorter.segment = new_segment(&report))) return 0;
		sorter.attribute = 0;
		sorter.space = sorter.newline = 0;
		sorter.is_code_ignored = sorter.is_semantic_set = 0;
	}

	/* Make a `token` where the context places us. */
	switch(symbol_mark) {
	case '~': /* General docs. */
		assert(sorter.state == SORT_DOC || sorter.state == SORT_ARGS);
		{ /* This lazily places whitespace and newlines. */
			struct token_array *selected = sorter.attribute
				? (sorter.state == SORT_ARGS ? &sorter.attribute->header
				: &sorter.attribute->contents) : &sorter.segment->doc;
			struct token *tok;
			const int is_para = sorter.newline > 1,
				is_space = sorter.space || sorter.newline,
				is_doc_empty = !sorter.segment->doc.size,
				is_selected_empty = !selected->size;
			sorter.space = sorter.newline = 0;
			if(is_para) {
				/* Switch out of attribute when on new paragraph. */
				sorter.attribute = 0, sorter.state = SORT_DOC;
				selected = &sorter.segment->doc;
				if(!is_doc_empty) {
					if(!(tok = new_token(selected, scan))) return 0;
					tok->symbol = NEWLINE; /* Override whatever's there. */
				}
			} else if(is_space && !is_selected_empty) {
				if(!(tok = new_token(selected, scan))) return 0;
				tok->symbol = SPACE; /* Override. */
			}
			if(!new_token(selected, scan)) return 0;
		}
		break;
	case '@': /* An attribute marker. */
		assert(sorter.state == SORT_DOC);
		if(!(sorter.attribute = new_attribute(sorter.segment, scan)))
			return 0;
		sorter.space = sorter.newline = 0; /* Also reset this for attributes. */
		break;
	default: /* Code. */
		assert(sorter.state == SORT_CODE);
		if(sorter.is_code_ignored) break;
		if(!new_token(&sorter.segment->code, scan)) return 0;
		break;
	}

	/* End the segment. */
	if(is_differed_cut) cut_segment_here(&sorter.segment);

	return 1;
}

/** Used for temporary things in doc mode. */
static int notify_brief(const struct scanner *const scan) {
	struct token *tok;
	assert(scan);
	/* `brief` is just documentation; no code. */
	if(!(tok = new_token(&brief, scan))) fprintf(stderr,
		"%s: something went wrong with this operation.\n", oops(scan)), 0;
	return 1;
}

/* Output. */

/** Prints line info in a static buffer, (to be printed?) */
static const char *pos(const struct token *const token) {
	static char p[128];
	if(!token) {
		sprintf(p, "Unknown position in report");
	} else {
		const int max_size = 16, tok_len = (token->length > max_size)
			? max_size : (int)token->length;
		sprintf(p, "%.32s:%lu, %s \"%.*s\"", token->label,
			(unsigned long)token->line, symbols[token->symbol], tok_len,
			token->from);
	}
	return p;
}

/** @return If the `code` is static. */
static int is_static(const struct token_array *const code) {
	size_t code_size = code->size;
	const struct token *tokens = code->data;
	assert(code);
	/* This is a real hack. Macros have no semicolons, so on X-macros, the
	 function below thinks that this is part of the function declaration.
	 I can't figure out how to insert a segment any more, so this will have to
	 do. Probably better with multi-trees. Also it assumes that the function
	 will be static and not shown, because arg! */
	do {
		unsigned level = 0;
		if(code_size && tokens[0].symbol == MACRO)
			tokens++, code_size--; else break;
		if(code_size && tokens[0].symbol == LPAREN)
			tokens++, code_size--, level = 1; else break;
		while(level && code_size) {
			switch (tokens[0].symbol) {
			case LPAREN: level++; break;
			case RPAREN: level--; break;
			default: break;
			}
			tokens++, code_size--;
		}
	} while(1);
	/* `main` is static, so hack it; we can't really do this because it's
	 general, but it works 99%. 80%? */
	return (code_size >= 1 && tokens[0].symbol == STATIC)
		|| (code_size >= 3
		&& tokens[0].symbol == ID && tokens[0].length == 3
		&& tokens[1].symbol == ID && tokens[1].length == 4
		&& tokens[2].symbol == LPAREN
		&& !strncmp(tokens[0].from, "int", 3)
		&& !strncmp(tokens[1].from, "main", 4));
}

/** @implements{Predicate<segment_>} */
static int keep_segment(const struct segment *const s) {
	int keep = 0;
	assert(s);
	if(s->doc.size || s->attributes.size
		|| s->division == DIV_FUNCTION) {
		/* `static` and containing `@allow`. */
		if(is_static(&s->code)) {
			size_t i = 0;
			while(i < s->attributes.size
				&& s->attributes.data[i].token.symbol != ATT_ALLOW) i++;
			if(i != s->attributes.size) keep = 1;
		} else keep = 1;
	}
	/* But wait, everything except the preamble has to have a title! */
	if(s->division != DIV_PREAMBLE && !s->code_params.size)
		keep = 0;
	if(!keep && cdoc_get_debug() & DBG_ERASE) {
		char a[12];
		segment_to_string(s, &a);
		fprintf(stderr, "keep_segment: erasing %s.\n", a);
	}
	return keep;
}

/** Keeps only the stuff we care about; discards no docs except fn and `static`
 if not `@allow`. */
void report_cull(void) {
	segment_array_keep_if(&report, &keep_segment, &erase_segment);
}

#include "report_warning.h"
