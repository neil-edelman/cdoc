/** 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 Guesses at decoding the meaning of a statement.

 @title Semantic.c.re
 @author Neil
 @version 2019-06
 @std C89
 @depend [re2c](http://re2c.org/)
 @fixme Old-style function support. */

#include <stdio.h>  /* printf */
/*#include <string.h>*/ /* memset */
/* #define NDEBUG */
/*#include <assert.h>*/ /* assert */
/*#include <limits.h>*/ /* INT_MAX */

#include "../src/XMacros.h" /* Needed for `#include "Namespace.h"`. */
#include "../src/Namespace.h"
#include "../src/Semantic.h" /* Semantic */


/** Copies marks in code and then figures out what the code is. */
struct Semantic {
	const char *buffer, *marker, *from, *cursor;
	/* fixme: The params; not used. */
	const char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8;
} semantic;

static enum Namespace namespace(void);

/** @param[marks] A zero-terminated string composed of characters from
 `symbol_marks` in `Scanner.c.re_c` or null.
 @return An educated guess of the namespace for the first occurence. */
enum Namespace Semantic(const char *const marks) {
	enum Namespace name = NAME_PREAMBLE;
	if(!marks) return name;
	semantic.buffer = semantic.marker = semantic.from = semantic.cursor = marks;
	semantic.s0 = semantic.s1 = semantic.s2 = semantic.s3 = semantic.s4
		= semantic.s5 = semantic.s6 = semantic.s7 = semantic.s8 = 0;
	printf("--> Semantic(\"%s\") = %s.\n", marks, namespaces[name]);
	return namespace();
}

/*!re2c
re2c:yyfill:enable  = 0;
re2c:define:YYCTYPE  = char;
re2c:define:YYCURSOR = semantic.cursor;
re2c:define:YYMARKER = semantic.marker;

unknown = "x";
// generic types are "A_(Foo)" which we assume is "<A>Foo"
generic = "x" | "1(".")" | "2(".",".")" | "3(".",".",".")";
void = "v";
struct = "s";
static = "z";
typename = ((struct? generic) | void) (unknown* "*"*)*;
array = "[" "x"? "]";
declaration = "x"* typename array* "x" array*;
param = "x"* declaration;
paramlist = param ("," param)*;
fn = unknown* static? unknown* typename "x(" (void | paramlist) ")";
typedef = "t";
equals = "=";

*/

/** This really is very complcated and differs from one compiler and version to
 to next. We might !stags:re2c format = 'const char *@@;'; but that's really
 difficult. */
static enum Namespace namespace(void) {
	semantic.cursor = semantic.buffer;
code:
	semantic.from = semantic.cursor;
/*!re2c
	"\x00" { return NAME_PREAMBLE; }
	"t" { return NAME_TYPEDEF; }
	"=" { return NAME_DATA; }
	"s" { return NAME_TAG; } // no
	generic { goto code; }
	* { goto code; }
	")\x00" { return NAME_FUNCTION; }
	// fixme: This does not take into account function pointers.
	// fixme: Old-style function definitions.
	// fixme: int (*foo)(void) would trivally break it.
*/
}
