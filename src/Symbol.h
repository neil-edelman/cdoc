#ifndef SYMBOL_H /* <-- !sym */
#define SYMBOL_H

#ifndef PARAM3_A
#include "XMacro.h"
#endif

/*struct SymbolOutput {
	int left, right;
	
}*/

#define NON_PRINT { 0, 0, 0 }
#define PRINT { 1, 1, &lit }
#define RIGHT_PRINT { 0, 1, &lit }
#define NOSPACE_PRINT { 0, 0, &lit }

/* Define `Symbols` -- these are the numerical values given to a section of
 text. The second argument is the mark of the symbol; `Semantic.c.re_tag`
 assumes that the marks are like this. The third is the output. */
#define SYMBOL(X) \
	/* EOF -- marked '\0' in memory. */ \
	X(END,        '\0', 0 ), \
	/* `C` syntax; 2nd is other stuff than '@'/'~' for inclusion in marker. */ \
	X(OPERATOR,   '*', /*1,1*/&lit ), \
	X(COMMA,      ',', /*0,1*/&lit ), \
	X(SEMI,       ';', /*0,1*/&lit ), \
	X(LBRACE,     '{', /*1,1*/&lit ), \
	X(RBRACE,     '}', /*1,1*/&lit ), \
	X(LPAREN,     '(', /*0,0*/&lit ), \
	X(RPAREN,     ')', /*0,0*/&lit ), \
	X(LBRACK,     '[', /*0,0*/&lit ), \
	X(RBRACK,     ']', /*0,0*/&lit ), \
	X(CONSTANT,   '#', /*1,1*/&lit ), \
	X(ID,         'x', /*1,1*/&lit ), \
	X(ID_ONE_GENERIC, '1', &gen1 ), \
	X(ID_TWO_GENERICS, '2', &gen2 ), \
	X(ID_THREE_GENERICS, '3', &gen3 ), \
	X(STRUCT,     's', /*1,1*/&lit ), \
	X(UNION,      's', /*1,1*/&lit ), \
	X(ENUM,       's', /*1,1*/&lit ), \
	X(TYPEDEF,    't', /*1,1*/&lit ), \
	X(STATIC,     'z', /*1,1*/&lit ), \
	X(VOID,       'v', /*1,1*/&lit ), \
	X(ELLIPSIS,   '.', /*1,1*/&lit ), \
	X(ASSIGNMENT, '=', /*1,1*/&lit ), \
	/* Each-block-tags; 2nd is '@' because we want them to have special
	meaning. */ \
	X(ATT_TITLE,      '@', 0 ), \
	X(ATT_PARAM,      '@', 0 ), \
	X(ATT_AUTHOR,     '@', 0 ), \
	X(ATT_STD,        '@', 0 ), \
	X(ATT_DEPEND,     '@', 0 ), \
	X(ATT_VERSION,    '@', 0 ), \
	X(ATT_FIXME,      '@', 0 ), \
	X(ATT_RETURN,     '@', 0 ), \
	X(ATT_THROWS,     '@', 0 ), \
	X(ATT_IMPLEMENTS, '@', 0 ), \
	X(ATT_ORDER,      '@', 0 ), \
	X(ATT_ALLOW,      '@', 0 ), \
	/* Documentation syntax; 2nd is '~' because it's documentation, it's just
	comments as far as `C` is concerned. */ \
	X(DOC_BEGIN, '~', 0 ), \
	X(DOC_END,   '~', 0 ), \
	X(WORD,      '~', &lit ), \
	X(SPACE,     '~', 0 ), \
	X(NEWLINE,   '~', &par ), \
	X(NBSP,      '~', 0 ), \
	X(NBTHINSP,  '~', 0 ), \
	X(ESCAPE,    '~', &esc_bs ), \
	/* Like <http://foo.com/>, <Cite1999>, [Foo](http://foo.com),
	![Foo](foo.png), <fn:foo>, _etc_. */ \
	X(URL,         '~', &url), \
	X(CITE,        '~', &cite), \
	X(LINK,        '~', 0), \
	X(IMAGE,       '~', 0), \
	X(SEE_FN,      '~', &see), \
	X(SEE_TAG,     '~', &see), \
	X(SEE_TYPEDEF, '~', &see), \
	X(SEE_DATA,    '~', &see), \
	/* Like `this` or _this_. */ \
	X(MATH_BEGIN,  '~', &math), \
	X(MATH_END,    '~', &math), \
	X(EM_BEGIN,    '~', &em), \
	X(EM_END,      '~', &em), \
	/* Like @param[a, b, c]. */ \
	X(DOC_LEFT,    '~', 0), \
	X(DOC_RIGHT,   '~', 0), \
	X(DOC_ID,      '~', &lit), \
	X(DOC_COMMA,   '~', &lit), \
	/* List items. " -* " */ \
	X(LIST_ITEM,   '~', 0), \
	/* Preformated. */ \
	X(PREFORMATED, '~', 0)

enum Symbol { SYMBOL(PARAM3_A) };
static const char *const symbols[] = { SYMBOL(STRINGISE3_A) };
static const char symbol_marks[] = { SYMBOL(PARAM3_B) };

#endif /* !sym --> */
