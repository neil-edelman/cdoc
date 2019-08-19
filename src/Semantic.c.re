/** Divides up the code into divisions based on `symbol_marks` in `Symbol.h`. */

#include <stdio.h>
#include "../src/Semantic.h"

#define ARRAY_NAME Size
#define ARRAY_TYPE size_t
#include "../src/Array.h"

enum Recursion { BRACE, PARENTHESIS, BRACKET };

#define ARRAY_NAME Recursion
#define ARRAY_TYPE enum Recursion
#define ARRAY_STACK
#include "../src/Array.h"

static struct RecursionArray check_recursion;

/* Define {CharArray}, a vector of characters -- again! */
#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "../src/Array.h"

static struct {
	struct CharArray copy;
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
	const char *cursor = code, *marker = code;
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

static void simplify_brackets() {
}

static int check_symbols(const char *const code, int *const checks) {
	const char *cursor = code, *marker = code;
	enum Recursion *rec;
	assert(code && checks);
	RecursionArrayClear(&check_recursion);
	*checks = 1;
	do {
/*!re2c
		symbol { continue; }
		"{" { if(!(rec = RecursionArrayNew(&check_recursion))) return 0;
			*rec = BRACE; continue; }
		"(" { if(!(rec = RecursionArrayNew(&check_recursion))) return 0;
			*rec = PARENTHESIS; continue; }
		"[" { if(!(rec = RecursionArrayNew(&check_recursion))) return 0;
			*rec = BRACE; continue; }
		"}" { rec = RecursionArrayPop(&check_recursion);
			if(!rec || *rec != BRACE) { *checks = 0; break; }
			continue; }
		")" { rec = RecursionArrayPop(&check_recursion);
			if(!rec || *rec != PARENTHESIS) { *checks = 0; break; }
			continue; }
		"]" { rec = RecursionArrayPop(&check_recursion);
			if(!rec || *rec != BRACKET) { *checks = 0; break; }
			continue; }
		"\x00" { break; }
		* { *checks = 0; break; }
*/
	} while(1);
	if(RecursionArraySize(&check_recursion)) *checks = 0;
	return 1;
}

/************/

/** Analyse a new string.
 @param[code] If null, frees the global semantic data. Otherwise, a string that
 consists of characters from `symbol_marks` defined in `Symbol.h`. */
int Semantic(const char *const code) {
	int checks;
	/* `Semantic(0)` should clear out memory and reset. */
	if(!code) {
		CharArray_(&semantic.copy);
		semantic.division = DIV_PREAMBLE;
		SizeArray_(&semantic.params);
		RecursionArray_(&check_recursion);
		return 1;
	}
	/* Reset the semantic to the most general state. */
	semantic.division = DIV_GENERAL_DECLARATION;
	SizeArrayClear(&semantic.params);

	/* Checks whether this makes sense. */
	if(!check_symbols(code, &checks)) return 0;
	if(!checks) { fprintf(stderr,
		"Classifying unknown statement as a general declaration.\n");
		return 1; }

	parse(code);
	return 1;
}

enum Division SemanticDivision(void) {
	return semantic.division;
}
