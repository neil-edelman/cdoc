/** 2019 Neil Edelman, distributed under the terms of the MIT License,
 \url{ https://opensource.org/licenses/MIT }.

 Decodes the meaning of a statement of code.

 @title Marker.re
 @author Neil
 @version 2019-06
 @std C89
 @depend re2c (\url{http://re2c.org/})
 @fixme Different doc comments need new paragraphs.
 @fixme Lists in comments, etc.
 @fixme {void A_BI_(Create, Thing)(void)} -> {<A>Create<BI>Thing(void)}.
 @fixme Trigraph support, (haha.)
 @fixme Old-style function support. */

#include <stdio.h>  /* printf */
#include <string.h> /* memset */
/* #define NDEBUG */
#include <assert.h> /* assert */
#include <limits.h> /* INT_MAX */

#include "../src/Scanner.h" /* Symbol */
#include "../src/Sorter.h" /* Token */
#include "../src/Marker.h" /* Marker */

/* In `Scanner.h`. */
static const char symbol_mark[] = { SYMBOL(PARAM3_C) };

/* Define {CharArray}, a vector of characters -- dynamic string; (again.) */
#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "../src/Array.h"



/** Marker copies marks in code and then figures out what the code is. */
struct Marker {
	const char *limit, *cursor, *marker, *from;
	struct CharArray buffer;
} marker;

/* Have {re2c} generate {YYMAXFILL}.
 \url{ http://re2c.org/examples/example_02.html }. */
/*!max:re2c*/

/** @param{ta} Set to null to destroy. */
int Marker(const struct TokenArray *const ta) {
	char *a, *end;
	size_t size, big_size;
	struct Token *token;
	if(!ta) {
		CharArray_(&marker.buffer);
		marker.limit = marker.cursor = marker.marker = marker.from = 0;
		return 1;
	}
	CharArrayClear(&marker.buffer);
	size = TokensSize(ta);
	big_size = size + YYMAXFILL;
	if(!(a = CharArrayBuffer(&marker.buffer, big_size))) return 0;
	while((token = TokensNext(ta, token)))
		*a++ = symbol_mark[TokenSymbol(token)];
	CharArrayAddSize(&marker.buffer, size);
	assert(a + big_size - size == CharArrayEnd(&marker.buffer));
	memset(a, '\0', big_size - size);
	printf("%s\n", CharArrayGet(&marker.buffer));
	return 1;
}

#if 0

/** Lexes the next token. This will update `ScannerToken` and
 `ScannerTokenInfo`.
 @return If the scanner had more tokens. */
int ScannerNext(void) {
	enum State *ps;
	if(!(ps = StateArrayPeek(&scanner.states))) return END;
	return scanner.symbol = state_fn[*ps]();
}

static enum State state_look(void);

/** Fills `token` with the last token.
 @param{token} If null, does nothing. */
void ScannerToken(struct Token *const token) {
	if(!token) return;
	assert(scanner.symbol && scanner.from && scanner.from <= scanner.cursor);
	token->symbol = scanner.symbol;
	token->from = scanner.from;
	if(scanner.from + INT_MAX < scanner.cursor) {
		fprintf(stderr, "Length of string chopped to " QUOTE(INT_MAX) ".\n");
		token->length = INT_MAX;
	} else {
		token->length = (int)(scanner.cursor - scanner.from);
	}
	token->line = scanner.line;
}

/*!re2c
re2c:define:YYCTYPE  = char;
re2c:define:YYFILL   = "return END;";
re2c:define:YYFILL:naked = 1;
re2c:define:YYLIMIT  = scanner.limit;
re2c:define:YYCURSOR = scanner.cursor;
re2c:define:YYMARKER = scanner.marker;
re2c:yyfill:enable = 0;
*/

/** Scans C code. See \see{ http://re2c.org/examples/example_07.html }.
 @implements ScannerFn */
static enum Symbol scan_code(void) {
	assert(state_look() == CODE);
code:
	scanner.from = scanner.cursor;
/*!re2c
	// http://re2c.org/examples/example_03.html
	"\x00" { if(scanner.limit - scanner.cursor <= YYMAXFILL) return END;
		goto code; }
	* { printf("Unknown, <%.*s>, just roll with it.\n",
		(int)(scanner.cursor - scanner.from), scanner.from); goto code; }

	// Documentation is not / * * /, but other then that.
	doc = "/""**";
	doc / [^/] { return push(DOC) ? BEGIN_DOC : END; }

	comment = "/""*";
	comment { return push_call(COMMENT); }
	cxx_comment = "//" [^\n]*;

	macro = ("#" | "%:");
	macro { return push_call(MACRO); }

	// Number separate from minus then numbers can't have whitespace, simple.
	oct = "0" [0-7]*;
	dec = [1-9][0-9]*;
	hex = '0x' [0-9a-fA-F]+;
	frc = [0-9]* "." [0-9]+ | [0-9]+ ".";
	exp = 'e' [+-]? [0-9]+;
	flt = (frc exp? | [0-9]+ exp) [fFlL]?;
	number = (oct | dec | hex | flt) [uUlL]*;
	number { return CONSTANT; }

	// Strings are more complicated because they can have whitespace and that
	// messes up the line count.
	// @fixme No trigraph support.
	// char_type = "u8"|"u"|"U"|"L"; <- These get caught in id; don't care.
	"L"? "\"" { return push_call(STRING); }
	"'" { return push_call(CHAR); }

	// Extension (hack) for generic macros; if one names them this way, it will
	// be documented nicely; the down side is, these are legal names for
	// identifiers; will be confused if you name anything this way that IS an
	// identifier.
	generic = [A-Z]+ "_";
	generic { return ID_GENERIC; }
	generic generic { return ID_GENERIC_TWO; }
	generic generic generic { return ID_GENERIC_THREE; }

	id = [a-zA-Z_][a-zA-Z_0-9]*;
	id { return ID; }

	operator = ":" | "..." | "::" | "?" | "+" | "-" | "*" | "/" | "%" | "^"
		| "xor" | "&" | "bitand" | "|" | "bitor" | "~" | "compl" | "!" | "not"
		| "=" | "<" | ">" | "+=" | "-=" | "%=" | "^=" | "xor_eq"
		| "&=" | "and_eq" | "|=" | "or_eq" | "<<" | ">>" | ">>=" | "<<="
		| "!=" | "not_eq" | "<=" | ">=" | "&&" | "and" | "||" | "or" | "++"
		| "--" | "." | "->";
	operator { return OPERATOR; }

	"struct"     { return STRUCT; }
	"union"      { return UNION; } // +fn are these all that can be braced?
	"enum"       { return ENUM; } // forgot one
	"typedef"    { return TYPEDEF; }
	("{" | "<%") { scanner.indent_level++; return LBRACE; }
	("}" | "%>") { scanner.indent_level--; return RBRACE; }
	("[" | "<:") { return LBRACK; }
	("]" | ":>") { return RBRACK; }
	"("          { return LPAREN; }
	")"          { return RPAREN; }
	","          { return COMMA; }
	";"          { return SEMI; }
*/
}

#endif
