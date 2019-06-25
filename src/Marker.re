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
	char *a;
	size_t size, big_size;
	struct Token *token = 0;
	if(!ta) {
		CharArray_(&marker.buffer);
		marker.limit = marker.cursor = marker.marker = marker.from = 0;
		return 1;
	}
	CharArrayClear(&marker.buffer);
	size = TokensSize(ta);
	big_size = size + YYMAXFILL;
	/*printf("marker: \"%.*s\" size = %lu/%lu\n", (int)CharArraySize(&marker.buffer), CharArrayGet(&marker.buffer), size, big_size);*/
	if(!(a = CharArrayBuffer(&marker.buffer, big_size))) return 0;
	printf("--> Marker: ");
	while((token = TokensNext(ta, token)))
		*a++ = symbol_mark[token->symbol],
		printf("%.*s ", token->length, token->from);
	assert((size_t)(a - CharArrayGet(&marker.buffer)) == size);
	CharArrayAddSize(&marker.buffer, size);
	memset(a, '\0', big_size - size);
	marker.cursor = marker.marker = marker.from = CharArrayGet(&marker.buffer);
	marker.limit = marker.cursor + size;
	printf("\"%s\" Is:%s%s.\n", CharArrayGet(&marker.buffer),
		MarkerIsFunction() ? " fn" : "", MarkerIsDefinition() ? " dn" : "");
	return 1;
}

/*!re2c
re2c:define:YYCTYPE  = char;
re2c:define:YYFILL   = "return 0;";
re2c:define:YYFILL:naked = 1;
re2c:define:YYLIMIT  = marker.limit;
re2c:define:YYCURSOR = marker.cursor;
re2c:define:YYMARKER = marker.marker;
re2c:yyfill:enable = 0;
*/

int MarkerIsFunction(void) {
	marker.cursor = CharArrayGet(&marker.buffer);
/*!re2c
	"\x00" { return 0; }
	unknown = "x";
	// generic types are "A_(Foo)" which we assume is "<A>Foo"
	generic = "x" | "1(x)" | "2(x,x)" | "3(x,x,x)"; // fixme: kind of
	void = "v";
	struct = "s";
	static = "z";
	typename = ((struct? generic) | void) unknown* "*"* unknown*;
	array = "[" "x"? "]";
	declaration = "x"* typename array* "x" array*;
	param = "x"* declaration;
	paramlist = param ("," param)*;
	// fixme: This does not take into account function pointers.
	// fixme: Old-style function definitions.
	newfn = unknown* static? unknown* typename "x(" (void | paramlist) ")";
	newfn "\x00" { return 1; }
*/
	return 0;
}

int MarkerIsDefinition(void) {
	marker.cursor = CharArrayGet(&marker.buffer);
/*!re2c
	"\x00" { return 0; }
	definition = unknown* static? unknown* typename "x";
	equals = "=";
	typedef = "t";
	definition "\x00" | definition equals | typedef { return 1; }
*/
	return 0;
}
