/** Divides up the code into divisions based on `symbol_marks` in `Symbol.h`. */

#include <stdio.h>
#include "../src/Report.h"
#include "../src/Semantic.h"

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

// The entire C langauge has been reduced to these tokens and "{}()[]".
symbol = "*" | "," | ";" | "#" | "x" | "1" | "2" | "3" | "s" | "t" | "z" | "v"
| "." | "=";

qualifier = "x" | "*"; // Eg, __Atomic const * & ->.
part = "x" | "s" | "t" | "z" | "v"; // Part of an identfier.
generic = "x"
| "1(" part ")"
| "2(" part "," part ")"
| "3(" part "," part "," part ")"; // Eg, X_(Array)
type = "s"? generic; // Eg, struct X_(Array)
const = "#" | "x"; // Eg, 42, SEMANTIC_NO.
identfier = "x";
static = "z";
array = "[" const? "]"; // no

postfix = qualifier* type identfier;

fn = static? qualifier*;

*/

/* X(DIV_PREAMBLE), X(DIV_FUNCTION), X(DIV_TAG), \
	X(DIV_TYPEDEF), X(DIV_GENERAL_DECLARATION) */

static int parse(const char *const code) {
	const char *cursor = code/*, *marker = code*/;
	assert(code);
	semantic.division = DIV_FUNCTION;
	printf("parse <%s>.\n", code);
	/*!re2c
	* { printf("* rule.\n"); }
	"\x00" { printf("end.\n"); }
	fn { printf("fn.\n"); }
	*/
	return 1;
}

/*static void simplify_brackets() {
}*/

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
	int checks;
	char *buffer;
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

	/* Make a string from `symbol_marks`. */
	if(!(buffer = CharArrayBuffer(&semantic.buffer, TokensMarkSize(code))))
		return 0;
	CharArrayExpand(&semantic.buffer, TokensMark(code, buffer));
	printf("Semantic: %s\n", buffer);

	/* Checks whether this makes sense. */
	if(!check_symbols(&checks)) return 0;
	if(!checks) { fprintf(stderr,
		"Classifying unknown statement as a general declaration.\n");
		return 1; }

	parse(buffer);
	return 1;
}

enum Division SemanticDivision(void) {
	return semantic.division;
}
