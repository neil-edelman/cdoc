/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 Divides up the code into divisions based on `symbol_marks` in `Symbol.h`. */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/Cdoc.h"
#include "../src/Report.h"
#include "../src/Semantic.h"

/** `right` is in the string `buffer`. Has assumed <fn:remove_recursive> has
 been called to eliminate `[]`. Very ad-hoc.
 @return What looks like a type starting at the right. */
static char *type_from_right(const char *const buffer, char *const right,
	const int is_eager) {
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
		memset(ret_type, '_', prefix - ret_type + 1);
		memset(middle, '_', suffix - middle + 1);
	}
}

static void index_to_string(const size_t *i, char (*const a)[12]) {
	sprintf(*a, "%lu", *(const unsigned long *)i % 100000000000u);
}

#define ARRAY_NAME Index
#define ARRAY_TYPE size_t
#define ARRAY_TO_STRING &index_to_string
#include "../src/Array.h"

/* Define {CharArray}, a vector of characters -- (again!) */
#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "../src/Array.h"

static struct {
	struct CharArray buffer, work;
	enum Division division;
	struct IndexArray params;
	const char *label;
	size_t line;
} semantic;

/** @param[name] In `semantic.buffer`.
 @return False on error. */
static int add_param(const char *const label) {
	size_t *param;
	const char *const acceptable = "x123";
	if(!label || !strchr(acceptable, *label)) return fprintf(stderr,
		"%.32s:%lu: param is '%c', not %s.\n", semantic.label,
		(unsigned long)semantic.line, label ? *label : '0', acceptable),
		errno = EILSEQ, 0;
	if(!(param = IndexArrayNew(&semantic.params))) return 0;
	*param = (size_t)(label - CharArrayGet(&semantic.buffer));
	return 1;
}

/*!stags:re2c format = 'char *@@;'; */

/*!re2c
re2c:yyfill:enable   = 0;
re2c:define:YYCTYPE  = char;
re2c:define:YYCURSOR = cursor;
re2c:define:YYMARKER = marker;
re2c:flags:tags      = 1;

// The entire C language has been reduced to these tokens.
end = "\x00";
symbol = "*" | "," | "#" | "x" | "1" | "2" | "3" | "s" | "t" | "z" | "v" | "."
| "=";
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
static = "z";
ellipses = ".";
const = "#" | "x"; // Eg, 42, SEMANTIC_NO.
qualifier = "x" | "*"; // Eg, __Atomic const * & ->.
part = "x" | "s" | "t" | "z" | "v"; // Part of an identifier.
generic = "x"
| "1(" part ")"
| "2(" part "," part ")"
| "3(" part "," part "," part ")"; // Eg, X_(Array)
type = tag? redact* generic; // Eg, struct X_(Array)
type_or_void = type | void;
argument = ("v" | "_" | "*" | "s" | "x" | "(" | ")" | generic)*
	("_" | "*" | "s" | "x" | "(" | ")" | generic)+;
*/

static int parse(void) {
	char *const buffer = CharArrayGet(&semantic.buffer), *cursor = buffer,
		*marker = cursor, *args = 0, *begin, *label = 0;
	int parens = 0;
	int is_not_likely = 0;

	assert(buffer);
/*!re2c
	// If there is no code, put it in the header.
	end {
		semantic.division = DIV_PREAMBLE;
		return 1;
	}
	// "typedef anything" is considered a typedef.
	typedef redact* skip_complex+ @label redact* end {
		semantic.division = DIV_TYPEDEF;
		label = type_from_right(buffer, label - 1, 1);
		if(!add_param(label)) return 0;
		return 1;
	}
	// "something tag [id]" is a tag.
	skip_simple* tag @label generic redact* end {
		semantic.division = DIV_TAG;
		if(!add_param(label)) return 0;
		return 1;
	}
	// "something [id]" is an anonymous tag and it is unlabelled.
	skip_simple* tag redact* end {
		semantic.division = DIV_TAG;
		return 1;
	}
	// Fixme: this is one of the . . . four? ways to define a function?
	static? redact* (@label (generic | type_or_void | qualifier) redact*){2,}
		@args "(" ( (argument ("," argument)* ",."?) | void ) ")" redact* end {
		semantic.division = DIV_FUNCTION;
		if(!add_param(label)) return 0;
		label = 0; /* For the args. */
		cursor = marker = args;
		goto params;
	}
	// All others are general declaration. See if we can extract a label.
	* {
		semantic.division = DIV_DATA;
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
		semantic.label, (unsigned long)semantic.line, buffer);
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
	const char *cursor = CharArrayGet(&semantic.buffer);
	char *stack;
	assert(checks && cursor);
	CharArrayClear(&semantic.work);
	*checks = 1;
check:
/*!re2c
	symbol { goto check; }
	"{" | "(" | "[" {
		if(!(stack = CharArrayNew(&semantic.work))) return 0;
		*stack = yych;
		goto check;
	}
	"}" | ")" | "]" {
		char left = yych == '}' ? '{' : yych == ')' ? '(' : '[';
		stack = CharArrayPop(&semantic.work);
		if(!stack || *stack != left) { *checks = 0; return 1; }
		goto check;
	}
	"\x00" { if(CharArraySize(&semantic.work)) *checks = 0; return 1; }
	* { *checks = 0; return 1; }
*/
}

/************/

/** Analyse a new string. Updates <fn:SemanticDivision> and
 <fn:SemanticParams>.
 @param[code] If null, frees the global semantic data. Otherwise, a string that
 consists of characters from `symbol_marks` defined in `Symbol.h`.
 @return Success, otherwise `errno` be set. */
int Semantic(const struct TokenArray *const code) {
	size_t buffer_size;
	char *buffer;

	/* `Semantic(0)` should clear out memory and reset. */
	if(!code) {
		CharArray_(&semantic.buffer);
		CharArray_(&semantic.work);
		semantic.division = DIV_PREAMBLE;
		IndexArray_(&semantic.params);
		return 1;
	}

	/* Reset the semantic to the most general state. */
	CharArrayClear(&semantic.buffer);
	semantic.division = DIV_DATA;
	IndexArrayClear(&semantic.params);
	semantic.label = TokensFirstLabel(code);
	semantic.line = TokensFirstLine(code);

	/* Make a string from `symbol_marks` and allocate maximum memory. */
	buffer_size = TokensMarkSize(code);
	assert(buffer_size);
	if(!CharArrayBuffer(&semantic.buffer, buffer_size)) return 0;
	buffer = CharArrayGet(&semantic.buffer);
	TokensMark(code, buffer);
	CharArrayExpand(&semantic.buffer, buffer_size);
	assert(buffer[buffer_size - 1] == '\0');

	{ /* Checks whether this makes sense. */
		int checks = 0;
		if(!check_symbols(&checks)) return 0;
		if(!checks) return fprintf(stderr,
		"%.32s:%lu: classifying unknown statement as a general declaration.\n",
			semantic.label, (unsigned long)semantic.line), 1;
	}

	/* Git rid of code. (Shouldn't happen!) */
	remove_recursive(buffer, '{', '}', '_');
	/* "Returning an array of this" and "returning this" are isomorphic. */
	remove_recursive(buffer, '[', ']', '_');
	/* Now with the {}[] removed. */
	effectively_typedef_fn_ptr(buffer);
	if(!parse()) return 0;
	if(CdocGetDebug() & DBG_SEMANTIC)
		fprintf(stderr, "%.32s:%lu: \"%s\" -> %s with params %s.\n",
		semantic.label, (unsigned long)semantic.line, buffer,
		divisions[semantic.division], IndexArrayToString(&semantic.params));
	assert(!IndexArraySize(&semantic.params)
		|| *IndexArrayPeek(&semantic.params) < buffer_size - 1);
	return 1;
}

/** Analyses of the last string's division. */
enum Division SemanticDivision(void) {
	return semantic.division;
}

/** Analyses of the last string's parameters, which can be anything.
 @param[no] Pass to get the number of `size_t`'s in the array.
 @param[array] Pass to get the size_t array. */
void SemanticParams(size_t *const no, const size_t **const array) {
	if(!no) { if(array) *array = 0; return; }
	*no = IndexArraySize(&semantic.params);
	if(!array) return;
	*array = IndexArrayGet(&semantic.params);
}
