/** Divides up the code into divisions based on `symbol_marks` in `Symbol.h`. */

#include <stdio.h>
#include <string.h>
#include "../src/Report.h"
#include "../src/Semantic.h"

/* fixme: range. */
#define ARRAY_NAME Size
#define ARRAY_TYPE size_t
#include "../src/Array.h"

/* Define {CharArray}, a vector of characters -- (again!) */
#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "../src/Array.h"

static struct {
	struct CharArray buffer, work;
	enum Division division;
	struct SizeArray params;
} semantic;

/*!re2c
re2c:yyfill:enable   = 0;
re2c:define:YYCTYPE  = char;
re2c:define:YYCURSOR = cursor;
re2c:define:YYMARKER = marker;

// The entire C language has been reduced to these tokens.
end = "\x00";
symbol = "*" | "," | "#" | "x" | "1" | "2" | "3" | "s" | "t" | "z" | "v" | "."
| "=";
separator = ";";
recursion = "{" | "}" | "(" | ")" | "[" | "]";
ignore = "_";
statement = symbol | recursion;
language = statement | separator | ignore;

qualifier = "x" | "*"; // Eg, __Atomic const * & ->.
part = "x" | "s" | "t" | "z" | "v"; // Part of an identifier.
struct_union_or_enum = "s";
// Eg, X_(Array)
generic = "x"
| "1(" part ")"
| "2(" part "," part ")"
| "3(" part "," part "," part ")";
// Eg, struct X_(Array)
type = ignore* struct_union_or_enum? ignore* generic ignore*;
typedef = "t";
const = "#" | "x"; // Eg, 42, SEMANTIC_NO.
identifier = "x";
static = "z";

postfix = qualifier* type identifier;

fn = static? qualifier*;

*/

/* X(DIV_PREAMBLE), X(DIV_FUNCTION), X(DIV_TAG), \
	X(DIV_TYPEDEF), X(DIV_GENERAL_DECLARATION) */

static int parse(void) {
	const char *cursor = CharArrayGet(&semantic.buffer), *marker = cursor;

	semantic.division = DIV_FUNCTION;
	printf("parse <%s>.\n", cursor);

start:
	/*!re2c
	// "typedef anything" is considered a typedef.
	typedef ignore* statement+ ignore* end {
		semantic.division = DIV_TYPEDEF;
		return 1;
	}
	// "something tag [id]" is a tag.
	language* struct_union_or_enum ignore* generic? ignore* end {
		semantic.division = DIV_TAG;
		return 1;
	}
	* { printf("* rule '%c'.\n", yych); goto start; }
	end { printf("end.\n"); return 0; }
	fn { printf("fn.\n"); }
	*/
	return 1;
}

/** From `buffer` it removes all between `left` and `right` and replaces the
 characters with underscore. */
static void remove_recursive(char *const buffer,
	const char left, const char right, const char output) {
	char *b;
	int level;
	assert(buffer && left && right && output);
	for(level = 0, b = buffer; *b != '\0'; b++) {
		if(*b == left)  level++;
		if(!level) continue;
		if(*b == right) level--;
		*b = output;
	}
}

/** `right` is in the string `buffer`. Has assumed <fn:remove_recursive> has
 been called to eliminate `[]`. Very ad-hoc.
 @return What looks like a type starting at the right. */
static char *type_from_right(char *const buffer, char *const right) {
	int is_type = 0, is_expecting_generic = 0, is_parentheses = 0;
	char *left = 0, *ch = right;
	assert(buffer && right);
	if(right < buffer) return 0;
	do {
		if(is_expecting_generic) {
			is_expecting_generic = 0;
			if(!strchr("123", *ch)) return 0;
			is_type = 1;
			continue;
		}
		if(*ch == ')') { /* Generic. */
			if(is_parentheses) return 0;
			is_parentheses = 1;
			continue;
		}
		if(*ch == '(') {
			if(!is_parentheses) break;
			is_expecting_generic = 1;
			continue;
		}
		if(is_parentheses) { continue; }
		/* Very lazy; if the thing doesn't look like a tag, id, or operator. */
		if(!strchr("sx*_", *ch)) { break; }
		is_type = 1;
	} while(left = ch, --ch >= buffer);
	return is_type ? left : 0;
}

/* Eg, x(*(*x(xxx))(x))(x(*)(x)) -> ___(*x(xxx))(x)__________
 -> _____x(xxx)______________
 Make sure to <fn:check_symbols> before using; it assumes parentheses are
 well-formed and <fn:remove_recursive> has been called to eliminate `[]`. */
static void effectively_typedef_fn_ptr(char *const buffer) {
	char *middle, *prefix, *suffix, *operator, *ret_type;
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
		if(!(ret_type = type_from_right(buffer, prefix - 1))) continue;
		/* Make sure there's an operator (*) somewhere. */
		for(operator = prefix + 1;
			operator < middle && *operator != '*' && strchr("x_", *operator);
			operator++);
		if(*operator != '*') continue;
		printf("typedef?: <%.*s> <%.*s>.\n",
			(int)(prefix - ret_type), ret_type,
			(int)(suffix - prefix + 1), prefix);
		
	}
}

/** It is the same class of declaration, just the able to be parsed by regular
 expression. */
static void make_regex_compatible(void) {
	char *const buffer = CharArrayGet(&semantic.buffer);
	assert(buffer);
	/* Git rid of code. */
	remove_recursive(buffer, '{', '}', '_');
	/* "Returning an array of this" and "returning this" are isomorphic. */
	remove_recursive(buffer, '[', ']', '_');
	/* Typedef all function pointers. */
	effectively_typedef_fn_ptr(buffer);
}

static int check_symbols(int *const checks) {
	const char *cursor = CharArrayGet(&semantic.buffer);
	char *stack;
	assert(checks && cursor);
	printf("check_symbols: buffer %s.\n", cursor);
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

/** Analyse a new string.
 @param[code] If null, frees the global semantic data. Otherwise, a string that
 consists of characters from `symbol_marks` defined in `Symbol.h`.
 @return Success, otherwise `errno` may (POSIX will) be set. */
int Semantic(const struct TokenArray *const code) {
	size_t buffer_size;

	/* `Semantic(0)` should clear out memory and reset. */
	if(!code) {
		CharArray_(&semantic.buffer);
		CharArray_(&semantic.work);
		semantic.division = DIV_PREAMBLE;
		SizeArray_(&semantic.params);
		return 1;
	}

	/* Reset the semantic to the most general state. */
	CharArrayClear(&semantic.buffer);
	semantic.division = DIV_GENERAL_DECLARATION;
	SizeArrayClear(&semantic.params);

	/* Make a string from `symbol_marks` and allocate maximum memory. */
	buffer_size = TokensMarkSize(code);
	assert(buffer_size);
	if(!CharArrayBuffer(&semantic.buffer, buffer_size)) return 0;
	TokensMark(code, CharArrayGet(&semantic.buffer));
	CharArrayExpand(&semantic.buffer, buffer_size);
	assert(CharArrayGet(&semantic.buffer)[buffer_size - 1] == '\0');
	printf("Semantic: %s\n", CharArrayGet(&semantic.buffer));

	{ /* Checks whether this makes sense. */
		int checks = 0;
		if(!check_symbols(&checks)) return 0;
		if(!checks) { fprintf(stderr,
			"Classifying unknown statement as a general declaration.\n");
			return 1; }
	}

	make_regex_compatible();
	printf("-> Now <%s>\n", CharArrayGet(&semantic.buffer));
	parse();
	return 1;
}

enum Division SemanticDivision(void) {
	return semantic.division;
}
