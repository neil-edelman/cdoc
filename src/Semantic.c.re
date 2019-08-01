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


/** Not thread safe. */
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
	name = namespace();
	printf("--> Semantic(\"%s\") = %s.\n", marks, namespaces[name]);
	return name;
}

/*!re2c
re2c:yyfill:enable  = 0;
re2c:define:YYCTYPE  = char;
re2c:define:YYCURSOR = semantic.cursor;
re2c:define:YYMARKER = semantic.marker;

// Must match `SYMBOL` in `Scanner.c.re`.
end = "\x00";
star = [^\x00];
operator = "*";
constant = "#";
id = "x"; // Everything that's not recogised is id.
tag = "s";
typedef = "t";
static = "z";
void = "v";
ellipses = ".";
name = id | typedef | static | void;
generic = id
	| "1(" name ")"
	| "2(" name "," name ")"
	| "3(" name "," name "," name ")"; // Includes macros.

typename = id* (tag? generic) (id* operator*)*;
constant_or_macro = constant | id; 
array = "[" constant_or_macro? "]";

left_things = "(" | "[" | "]" | operator | id; // const is an id
right_things = ")" | "[" | "]" | operator;

id_detail = ("("|"["|"]"operator);
declaration = id* typename array* id array*;
param = id* declaration; // fixme: or fn declaration
paramlist = param ("," param)* ("," ellipses)?;
fn = id* static? id* typename id "(" (void | paramlist) ")"; // fixme: or old-

fn_ptr = (operator|id|tag|void)+ "(" operator (operator|id|tag)* generic ")"
	"(" (operator|id|tag|void|"("|")"|array)* ")" array*;

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
	"t" ("("|")"|operator|constant|id|tag|void|ellipses){2,} "\x00" { return NAME_TYPEDEF; }
	static? (operator|id|tag|void)+ array* "="  { return NAME_DATA; }
	generic { goto code; }
	* { goto code; }
	")\x00" { return NAME_FUNCTION; }
	fn ( ( "{" [^x00]* "}\x00" ) | ( ";\x00" ) ) { return NAME_FUNCTION; }
	// fixme: This does not take into account function pointers.
	// fixme: Old-style function definitions.
	// fixme: int (*foo)(void) would trivally break it.

	// something <tag> [name];
	static? id* tag id? end { return NAME_TAG; }
*/
}
