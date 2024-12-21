/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 Simplifies the code into divisions based on `symbol_marks` in `symbol.h`. */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/boxdoc.h"
#include "../src/report_print.h"
#include "../src/semantic.h"

/** `right` is in the string `buffer`. Has assumed <fn:remove_recursive> has
 been called to eliminate `[]`. Very ad-hoc.
 @return What looks like a type starting at the right. */
static char *type_from_right(const char *const buffer,
	char *const right, const int is_eager) {
	int is_type = 0;
	char *left = 0, *ch = right;
	assert(buffer && right);
	if(right < buffer) return 0;
	if(*ch == 'v') is_type = 1, left = ch; /* `void` sans-qualifier. */
	else do {
		if(*ch == '_') continue; /* Redacted. */
		if(*ch == ')') { /* Generic? Lookbehind. */
			do { ch--; if(ch <= buffer || *ch == ')') break; }
				while(*ch != '(');
			ch--;
			if(!strchr("123", *ch)) break;
			is_type = 1;
			if(is_eager) { left = ch; break; }
			continue;
		}
		/* Very lazy; if the thing doesn't look like a tag, id, or operator. */
		if(!strchr("sx*", *ch)) break;
		is_type = 1;
		if(is_eager && *ch == 'x') { left = ch; break; }
	} while(left = ch, --ch >= buffer);
	return is_type ? left : 0;
}

/* Make sure to <fn:check_symbols> before using; it assumes parentheses are
 well-formed and <fn:remove_recursive> has been called to eliminate `[]`. */
static void effectively_typedef_fn_ptr(char *const buffer) {
	char *middle, *prefix, *suffix, *operator;
	char *ret_type;
	int level;
	assert(buffer);
	for(middle = buffer; (middle = strstr(middle, ")(")); middle += 2) {
		/* `suffix` after the params of the function pointer. */
		suffix = middle + 1, level = 1; do {
			suffix++, assert(*suffix != '\0');
			if(*suffix == '(')      level++;
			else if(*suffix == ')') level--;
		} while(level);
		/* `prefix` is at the beginning of the function pointer. */
		prefix = middle, level = 1; do {
			prefix--, assert(buffer <= prefix);
			if(*prefix == ')') level++;
			else if(*prefix == '(') level--;
		} while(level);
		/* This has to have a return-type. Will fail on "123". */
		if(!(ret_type = type_from_right(buffer, prefix - 1, 0))) continue;
		/* Make sure there's an operator (*) somewhere. */
		for(operator = prefix + 1;
			operator < middle && *operator != '*' && strchr("x_", *operator);
			operator++);
		if(*operator != '*') continue;
		memset(ret_type, '_', (size_t)(prefix - ret_type) + 1u);
		memset(middle, '_', (size_t)(suffix - middle) + 1u);
	}
}

static void index_to_string(const size_t *i, char (*const a)[12]) {
	sprintf(*a, "%lu", *(const unsigned long *)i % 100000000000u);
}

/*#define ARRAY_NAME index
#define ARRAY_TYPE size_t
#define ARRAY_EXPECT_TRAIT
#include "../src/array.h"
#define ARRAY_TO_STRING &index_to_string
#include "../src/array.h"*/
#include "../src/index_array.h"

/* A vector of characters -- (again!) */
#define ARRAY_NAME char
#define ARRAY_TYPE char
#include "../src/boxes/array.h"

static struct {
	struct char_array buffer, work;
	enum division division;
	struct index_array params;
	const char *label;
	size_t line;
} semantics;

/** @param[name] In `semantics.buffer`.
 @return False on error. */
static int add_param(const char *const label) {
	size_t *param;
	const char *const acceptable = "x123";
	if(!label || !strchr(acceptable, *label)) return fprintf(stderr,
		"%.32s:%lu: param is '%c', not %s.\n", semantics.label,
		(unsigned long)semantics.line, label ? *label : '0', acceptable),
		errno = EILSEQ, 0;
	if(!(param = index_array_new(&semantics.params))) return 0;
	*param = (size_t)(label - semantics.buffer.data);
	return 1;
}

/*!re2c
re2c:yyfill:enable   = 0;
re2c:define:YYCTYPE  = char;
re2c:define:YYCURSOR = cursor;
re2c:define:YYMARKER = marker;
re2c:flags:tags      = 1;

// The entire C language has been reduced to these tokens.
end = "\x00";
symbol = "*" | "," | "#" | "x" | "m" | "1" | "2" | "3" | "s" | "t" | "z"
	| "v" | "." | "=";
separator = ";";
recursion = "{" | "}" | "(" | ")" | "[" | "]";
redact = "_";

skip_simple = symbol | redact;
skip_complex = skip_simple | recursion;
skip_multi = skip_complex | separator;

typedef = "t";
void = "v";
tag = "s";
identifier = "x";
macro = "m";
static = "z";
ellipses = ".";
const = "#" | "x"; // Eg, 42, SEMANTIC_NO.
qualifier = "x" | "*"; // Eg, __Atomic const * & ->.
part = "x" | "s" | "t" | "z" | "v"; // Part of an identifier.
generic = "x"
| "1(" part ")"
| "2(" part "," part ")"
| "3(" part "," part "," part ")"; // Eg, X_(array)
type = tag? redact* generic; // Eg, struct X_(array)
type_or_void = type | void;
argument = ("v" | "_" | "*" | "s" | "x" | "(" | ")" | generic)*
	("_" | "*" | "s" | "x" | "(" | ")" | generic)+;
*/

static int parse(void) {
	char *const buffer = semantics.buffer.data, *cursor = buffer,
		*marker = cursor, *args = 0, *begin, *label = 0;
	int parens = 0;
	int is_not_likely = 0;
	/*!stags:re2c format = 'char *@@;'; */

	/* fixme: warning: variable 'yyt2' may be uninitialized when
	 used here [-Wconditional-uninitialized]; I don't know what I'm doing. */
	yyt2 = buffer;

	assert(buffer);
/*!re2c
	// If there is no code, put it in the header.
	end {
		semantics.division = DIV_PREAMBLE;
		return 1;
	}
	// "typedef anything" is considered a typedef.
	redact* typedef redact* skip_complex+ @label redact* end {
		semantics.division = DIV_TYPEDEF;
		label = type_from_right(buffer, label - 1, 1);
		if(!add_param(label)) return 0;
		return 1;
	}
	// "something tag [id]" is a tag.
	skip_simple* tag @label generic redact* end {
		semantics.division = DIV_TAG;
		if(!add_param(label)) return 0;
		return 1;
	}
	// "something [id]" is an anonymous tag and it is unlabelled.
	skip_simple* tag redact* end {
		semantics.division = DIV_TAG;
		return 1;
	}
	// Fixme: this is one of the . . . four? ways to define a function?
	redact* static? redact*
		(@label (generic | type_or_void | qualifier) redact*){2,}
		@args "(" ( (argument ("," argument)* ",."?) | void ) ")" redact* end {
		semantics.division = DIV_FUNCTION;
		if(!add_param(label)) return 0;
		label = 0; /* For the args. */
		cursor = marker = args;
		goto params;
	}
	// All others are general declaration. See if we can extract a label.
	* {
		semantics.division = DIV_DATA;
		/* Start at the right of '=' and scan left until something looks like
		 a label. */
		if(!(label = strchr(buffer, '='))) label = buffer + strlen(buffer);
		while(--label >= buffer) {
			char *maybe = 0;
			while(label > buffer && strchr("_*,#.", *label)) label--;
			maybe = type_from_right(buffer, label, 1);
			if(maybe) { label = maybe; break; }
		}
		if(label >= buffer && !add_param(label)) return 0;
		return 1;
	}
*/
	/* The DIV_FUNCTION backtracks here to figure out the arguments. */
params:
/*!re2c
	// We don't care about these.
	[_*sv.]+ { goto params; }
	// Raise the level of parentheses.
	"(" { parens++; goto params; }
	// Lower the level; if zero, add the last param if you have it.
	")" {
		if(--parens <= 0) {
			if(label && !add_param(label)) return 0;
			return 1;
		}
		is_not_likely = 1;
		goto params;
	}
	// Update the label until `is_not_likely`; the label is generally the last.
	@begin generic { if(!label || !is_not_likely) label = begin; goto params; }
	// New label.
	"," {
		if(parens > 1) goto params;
		else if(parens < 1 || !label || !add_param(label)) goto unable;
		label = 0;
		is_not_likely = 0;
		goto params;
	}
	* { goto unable; }
*/
unable:
	fprintf(stderr, "%.32s:%lu: unable to extract parameter list from %s.\n",
		semantics.label, (unsigned long)semantics.line, buffer);
	return 1;
}

/** From `buffer` it removes all between `left` and `right` and replaces the
 characters with `output` respecting hierarchy. */
static void remove_recursive(char *const buffer,
	const char left, const char right, const char output) {
	char *b;
	int level = 0;
	assert(buffer && left && right && output);
	for(b = buffer; *b != '\0'; b++) {
		if(*b == left)  level++;
		if(!level) continue;
		if(*b == right) level--;
		*b = output;
	}
}

/** Checks that `checks`, a string, braces' match up. */
static int check_symbols(int *const checks) {
	const char *cursor = semantics.buffer.data;
	char *stack;
	assert(checks && cursor);
	char_array_clear(&semantics.work);
	*checks = 1;
check:
/*!re2c
	symbol { goto check; }
	"{" | "(" | "[" {
		if(!(stack = char_array_new(&semantics.work))) return 0;
		*stack = yych;
		goto check;
	}
	"}" | ")" | "]" {
		char left = yych == '}' ? '{' : yych == ')' ? '(' : '[';
		stack = char_array_pop(&semantics.work);
		if(!stack || *stack != left) { *checks = 0; return 1; }
		goto check;
	}
	"\x00" { if(semantics.work.size) *checks = 0; return 1; }
	* { *checks = 0; return 1; }
*/
}

/************/

/** Analyze a new string. Updates <fn:semantic_division> and
 <fn:semantic_params>.
 @param[code] If null, frees the global semantic data. Otherwise, a string that
 consists of characters from `symbol_marks` defined in `Symbol.h`.
 @return Success, otherwise `errno` be set. */
int semantic(const struct token_array *const code) {
	size_t buffer_size;
	char *buffer;

	/* `semantic(0)` should clear out memory and reset. */
	if(!code) {
		char_array_(&semantics.buffer);
		char_array_(&semantics.work);
		semantics.division = DIV_PREAMBLE;
		index_array_(&semantics.params);
		return 1;
	}

	/* Reset the semantic to the most general state. */
	char_array_clear(&semantics.buffer);
	semantics.division = DIV_DATA;
	index_array_clear(&semantics.params);
	semantics.label = tokens_first_label(code);
	semantics.line = tokens_first_line(code);

	/* Make a string from `symbol_marks` and allocate maximum memory. */
	buffer_size = tokens_mark_size(code);
	assert(buffer_size);
	if(!(buffer = char_array_buffer(&semantics.buffer, buffer_size))) return 0;
	tokens_mark(code, buffer);
	assert(buffer[buffer_size - 1] == '\0');

	{ /* Checks whether this makes sense. */
		int checks = 0;
		if(!check_symbols(&checks)) return 0;
		if(!checks) return fprintf(stderr,
		"%.32s:%lu: classifying unknown statement as a general declaration.\n",
			semantics.label, (unsigned long)semantics.line), 1;
	}

	/* Git rid of code. (Shouldn't happen!) */
	remove_recursive(buffer, '{', '}', '_');
	/* "Returning an array of this" and "returning this" are isomorphic. */
	remove_recursive(buffer, '[', ']', '_');
	/* Macros are not good. */
	{
		char *b = buffer;
		int level = 0, is_last_macro = 0;
		while(*b != '\0') {
			if(level) {
				if(*b == ')') level--;
				else if(*b == '(') level++;
				*b = '_';
			} else if(*b == 'm') {
				is_last_macro = 1;
				*b = '_';
			} else if(is_last_macro) {
				is_last_macro = 0;
				if(*b == '(') *b = '_', level = 1;
			}
			b++;
		}
	}
	/* Now with the {}[] removed. */
	effectively_typedef_fn_ptr(buffer);
	if(!parse()) return 0;
	if(cdoc_get_debug() & DBG_SEMANTIC)
		fprintf(stderr, "%.32s:%lu: \"%s\" -> %s with params %s.\n",
		semantics.label, (unsigned long)semantics.line, buffer,
		division[semantics.division].symbol,
		index_array_to_string(&semantics.params));
	assert(!semantics.params.size
		|| *index_array_peek(&semantics.params) < buffer_size - 1);
	return 1;
}

/** Analyses of the last string's division. */
enum division semantic_division(void) {
	return semantics.division;
}

/** Analyses of the last string's parameters, which can be anything.
 @param[no] Pass to get the number of `size_t`'s in the array.
 @param[array] Pass to get the size_t array. */
void semantic_params(size_t *const no, const size_t **const array) {
	if(!no) { if(array) *array = 0; return; }
	*no = semantics.params.size;
	if(!array) return;
	*array = semantics.params.data;
}
