#ifndef SYMBOL_H /* <-- !sym */
#define SYMBOL_H

#ifndef PARAM5A
#include "XMacro.h"
#endif

/* Define `Symbols` -- these are the numerical values given to a section of
 text. The format is
 `{ symbols, symbol_marks, symbol_outs, symbol_lspaces, symbol_rspaces }`. */
#define SYMBOL(X) \
	/* EOF -- marked '\0' in memory. */ \
	X(END,        '\0', 0, 0, 0), \
	/* `C` syntax; 2nd is other stuff than '@'/'~' for inclusion in marker. */ \
	X(OPERATOR,   '*', &lit, 1, 1), \
	X(COMMA,      ',', &lit, 0, 1), \
	X(SEMI,       ';', &lit, 0, 1), \
	X(LBRACE,     '{', &lit, 1, 1), \
	X(RBRACE,     '}', &lit, 1, 1), \
	X(LPAREN,     '(', &lit, 0, 0), \
	X(RPAREN,     ')', &lit, 0, 0), \
	X(LBRACK,     '[', &lit, 0, 0), \
	X(RBRACK,     ']', &lit, 0, 0), \
	X(CONSTANT,   '#', &lit, 1, 1 ), \
	X(ID,         'x', &lit, 1, 1 ), \
	X(ID_ONE_GENERIC, '1', &gen1, 1, 1), \
	X(ID_TWO_GENERICS, '2', &gen2, 1, 1), \
	X(ID_THREE_GENERICS, '3', &gen3, 1, 1), \
	X(STRUCT,     's', &lit, 1, 1), \
	X(UNION,      's', &lit, 1, 1), \
	X(ENUM,       's', &lit, 1, 1), \
	X(TYPEDEF,    't', &lit, 1, 1), \
	X(STATIC,     'z', &lit, 1, 1), \
	X(VOID,       'v', &lit, 1, 1), \
	X(ELLIPSIS,   '.', &lit, 1, 1), \
	X(ASSIGNMENT, '=', &lit, 1, 1), \
	/* Each-block-tags; 2nd is '@' because we want them to have special
	meaning. Non-printable, so it doesn't matter about the last few. */ \
	X(ATT_TITLE,      '@', 0, 0, 0), \
	X(ATT_PARAM,      '@', 0, 0, 0), \
	X(ATT_AUTHOR,     '@', 0, 0, 0), \
	X(ATT_STD,        '@', 0, 0, 0), \
	X(ATT_DEPEND,     '@', 0, 0, 0), \
	X(ATT_VERSION,    '@', 0, 0, 0), \
	X(ATT_FIXME,      '@', 0, 0, 0), \
	X(ATT_RETURN,     '@', 0, 0, 0), \
	X(ATT_THROWS,     '@', 0, 0, 0), \
	X(ATT_IMPLEMENTS, '@', 0, 0, 0), \
	X(ATT_ORDER,      '@', 0, 0, 0), \
	X(ATT_ALLOW,      '@', 0, 0, 0), \
	/* Documentation syntax; 2nd is '~' because it's documentation, it's just
	comments as far as `C` is concerned. */ \
	X(DOC_BEGIN, '~', 0, 0, 0), \
	X(DOC_END,   '~', 0, 0, 0), \
	X(WORD,      '~', &lit, 0, 0), \
	X(SPACE,     '~', &ws, 0, 0), \
	X(NEWLINE,   '~', &par, 0, 0), \
	X(NBSP,      '~', &nbsp, 0, 0), \
	X(NBTHINSP,  '~', &nbthinsp, 0, 0), \
	X(ESCAPE,    '~', &escape, 0, 0), \
	/* Like <http://foo.com/>, <Cite1999>, [Foo](http://foo.com),
	![Foo](foo.png), <fn:foo>, _etc_. */ \
	X(URL,         '~', &url, 0, 0), \
	X(CITE,        '~', &cite, 0, 0), \
	X(LINK_START,  '~', &link, 0, 0), \
	X(IMAGE_START, '~', 0, 0, 0), /* fixme */ \
	X(SEE_FN,      '~', &see_fn, 0, 0), \
	X(SEE_TAG,     '~', &see_tag, 0, 0), \
	X(SEE_TYPEDEF, '~', &see_typedef, 0, 0), \
	X(SEE_DATA,    '~', &see_data, 0, 0), \
	/* Like `this` or _this_. */ \
	X(MATH_BEGIN,  '~', &math_begin, 0, 0), \
	X(MATH_END,    '~', &math_end, 0, 0), \
	X(EM_BEGIN,    '~', &em_begin, 0, 0), \
	X(EM_END,      '~', &em_end, 0, 0), \
	/* Like @param[a, b, c]. */ \
	X(DOC_LEFT,    '~', 0, 0, 0), \
	X(DOC_RIGHT,   '~', 0, 0, 0), \
	X(DOC_ID,      '~', &lit, 0, 0), \
	X(DOC_COMMA,   '~', &lit, 0, 0), \
	/* List items. " -* " */ \
	X(LIST_ITEM,   '~', &list, 0, 0), \
	/* Preformatted. */ \
	X(PREFORMATTED, '~', &pre, 0, 0)

enum Symbol { SYMBOL(PARAM5A) };
static const char *const symbols[] = { SYMBOL(STRINGISE5A) };
static const char symbol_marks[] = { SYMBOL(PARAM5B) };

#endif /* !sym --> */
