#ifndef SYMBOL_H /* <-- !sym */
#define SYMBOL_H

#ifndef PARAM3_A
#include "XMacro.h"
#endif

/* Define `Symbols` -- these are the numerical values given to a section of
 text. */
#define SYMBOL(X) \
	/* EOF -- marked '\0' in memory. */ \
	X(END, '\0', 0), \
	/* `C` syntax; 2nd is other stuff than '@'/'~' for inclusion in marker. */ \
	X(OPERATOR, '*', &lit), X(COMMA, ',', &lit), X(SEMI, ';', &lit), \
	X(LBRACE, '{', &lit), X(RBRACE, '}', &lit), X(LPAREN, '(', &lit), \
	X(RPAREN, ')', &lit), X(LBRACK, '[', &lit), X(RBRACK, ']', &lit), \
	X(CONSTANT, '#', &lit), X(ID, 'x', &lit), \
	X(ID_ONE_GENERIC, '1', &gen1), X(ID_TWO_GENERICS, '2', &gen2), \
	X(ID_THREE_GENERICS, '3', &gen3), X(STRUCT, 's', &lit), \
	X(UNION, 's', &lit), X(ENUM, 's', &lit), X(TYPEDEF, 't', &lit), \
	X(STATIC, 'z', &lit), X(VOID, 'v', &lit), X(ELLIPSIS, '.', &lit), \
	X(ASSIGNMENT, '=', &lit), \
	/* Each-block-tags; 2nd is '@' because we want them to have special
	meaning. */ \
	X(ATT_TITLE, '@', &lit), X(ATT_PARAM, '@', &lit), \
	X(ATT_AUTHOR, '@', &lit), X(ATT_STD, '@', &lit), X(ATT_DEPEND, '@', &lit), \
	X(ATT_VERSION, '@', &lit), X(ATT_FIXME, '@', &lit), \
	X(ATT_RETURN, '@', &lit), X(ATT_THROWS, '@', &lit), \
	X(ATT_IMPLEMENTS, '@', &lit), X(ATT_ORDER, '@', &lit), \
	X(ATT_ALLOW, '@', &lit), \
	/* Documentation syntax; 2nd is '~' because it's documentation, it's just
	comments as far as `C` is concerned. */ \
	X(DOC_BEGIN, '~', 0), X(DOC_END, '~', 0), \
	X(WORD, '~', &lit), X(SPACE, '~', 0), X(NEWLINE, '~', &par), \
	X(NBSP, '~', 0), X(NBTHINSP, '~', 0), X(ESCAPE, '~', &esc_bs), \
	/* Like <http://foo.com/>, <Cite1999>, [Foo](http://foo.com),
	![Foo](foo.png), <fn:foo>, _etc_. */ \
	X(URL, '~', &url), X(CITE, '~', &cite), X(LINK, '~', 0), \
	X(IMAGE, '~', 0), X(SEE_FN, '~', &see), X(SEE_TAG, '~', &see), \
	X(SEE_TYPEDEF, '~', &see), X(SEE_DATA, '~', &see), \
	/* Like `this` or _this_. */ \
	X(MATH_BEGIN, '~', &math), X(MATH_END, '~', &math), \
	X(EM_BEGIN, '~', 0), X(EM_END, '~', 0), \
	/* Like @param[a, b, c]. */ \
	X(DOC_LEFT, '~', 0), X(DOC_RIGHT, '~', 0), \
	X(DOC_ID, '~', &lit), X(DOC_COMMA, '~', &lit), \
	/* List items. " -* " */ \
	X(LIST_ITEM, '~', 0), \
	/* Preformated. */ \
	X(PREFORMATED, '~', 0)

enum Symbol { SYMBOL(PARAM3_A) };
static const char *const symbols[] = { SYMBOL(STRINGISE3_A) };
static const char symbol_marks[] = { SYMBOL(PARAM3_B) };

#endif /* !sym --> */
