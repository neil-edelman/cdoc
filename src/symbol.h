#ifndef SYMBOL_H
#define SYMBOL_H

/* Define `Symbols` -- these are the numerical values given to a section of
 text. The format is
 `{ symbols, symbol_marks, symbol_md_out, symbol_html_out, symbol_lspaces,
 symbol_rspaces, symbol_attribute_titles (must be short) }`. */
#define SYMBOL \
	/* EOF -- marked '\0' in memory. */ \
	X(END,        '\0', 0, 0, 0, 0), \
	/* `C` syntax; 2nd is other stuff than '@'/'~' for inclusion in marker. */ \
	X(OPERATOR,   '*', &out_lit, 1, 0, 0), \
	X(COMMA,      ',', &out_lit, 0, 1, 0), \
	X(SEMI,       ';', &out_lit, 0, 1, 0), \
	X(LBRACE,     '{', &out_lit, 1, 1, 0), \
	X(RBRACE,     '}', &out_lit, 1, 1, 0), \
	X(LPAREN,     '(', &out_lit, 0, 0, 0), \
	X(RPAREN,     ')', &out_lit, 0, 0, 0), \
	X(LBRACK,     '[', &out_lit, 0, 0, 0), \
	X(RBRACK,     ']', &out_lit, 0, 0, 0), \
	X(CONSTANT,   '#', &out_lit, 1, 1, 0), \
	X(ID,         'x', &out_lit, 1, 1, 0), \
	X(MACRO,      'm', &out_lit, 1, 1, 0), \
	X(ID_ONE_GENERIC, '1', &out_gen1, 1, 1, 0), \
	X(ID_TWO_GENERICS, '2', &out_gen2, 1, 1, 0), \
	X(ID_THREE_GENERICS, '3', &out_gen3, 1, 1, 0), \
	X(STRUCT,     's', &out_lit, 1, 1, 0), \
	X(UNION,      's', &out_lit, 1, 1, 0), \
	X(ENUM,       's', &out_lit, 1, 1, 0), \
	X(TYPEDEF,    't', &out_lit, 1, 1, 0), \
	X(STATIC,     'z', &out_lit, 1, 1, 0), \
	X(VOID,       'v', &out_lit, 1, 1, 0), \
	X(ELLIPSIS,   '.', &out_lit, 1, 1, 0), \
	X(ASSIGNMENT, '=', &out_lit, 1, 1, 0), \
	X(LOCAL_INCLUDE, 'i', 0, 0, 0, 0), \
	/* Each-block-tags; 2nd is '@' because we want them to have special
	 meaning. Non-printable, so it doesn't matter about the last few. */ \
	X(ATT_SUBTITLE,   '@', 0, 0, 0, "Subtitle"), \
	X(ATT_PARAM,      '@', 0, 0, 0, "Parameter"), \
	X(ATT_AUTHOR,     '@', 0, 0, 0, "Author"), \
	X(ATT_STD,        '@', 0, 0, 0, "Standard"), \
	X(ATT_DEPEND,     '@', 0, 0, 0, "Dependancies"), \
	X(ATT_FIXME,      '@', 0, 0, 0, "Caveat"), \
	X(ATT_RETURN,     '@', 0, 0, 0, "Return"), \
	X(ATT_THROWS,     '@', 0, 0, 0, "Exceptional return"), \
	X(ATT_IMPLEMENTS, '@', 0, 0, 0, "Implements"), \
	X(ATT_ORDER,      '@', 0, 0, 0, "Order"), \
	X(ATT_ALLOW,      '@', 0, 0, 0, 0), \
	X(ATT_LICENSE,    '@', 0, 0, 0, "License"), \
	X(ATT_CF,         '@', 0, 0, 0, "See also"), \
	X(ATT_ABSTRACT,   '@', 0, 0, 0, "Abstract"), \
	/* Documentation syntax; 2nd is '~' because it's documentation, it's just
	 comments as far as `C` is concerned. */ \
	X(DOC_BEGIN, '~', 0, 0, 0, 0), \
	X(DOC_END,   '~', 0, 0, 0, 0), \
	X(WORD,      '~', &out_lit, 0, 0, 0), \
	X(SPACE,     '~', &out_ws, 0, 0, 0), \
	X(NEWLINE,   '~', &out_par, 0, 0, 0), \
	X(NBSP,      '~', &out_nbsp, 0, 0, 0), \
	X(NBTHINSP,  '~', &out_nbthinsp, 0, 0, 0), \
	X(MATHCALO,  '~', &out_mathcalo, 0, 0, 0), \
	X(CTHETA,    '~', &out_ctheta, 0, 0, 0), \
	X(COMEGA,    '~', &out_comega, 0, 0, 0), \
	X(TIMES,     '~', &out_times, 0, 0, 0), \
	X(CDOT,      '~', &out_cdot, 0, 0, 0), \
	X(LOG,       '~', &out_log, 0, 0, 0), \
	X(ESCAPE,    '~', &out_escape, 0, 0, 0), \
	/* Like <http://foo.com/>, <Cite1999>, [Foo](http://foo.com),
	 ![Foo](foo.png), <fn:foo>, _etc_. */ \
	X(URL,         '~', &out_url, 0, 0, 0), \
	X(CITE,        '~', &out_cite, 0, 0, 0), \
	X(LINK_START,  '~', &out_link, 0, 0, 0), \
	X(IMAGE_START, '~', &out_image, 0, 0, 0), \
	X(SEE_FN,      '~', &out_see_fn, 0, 0, 0), \
	X(SEE_TAG,     '~', &out_see_tag, 0, 0, 0), \
	X(SEE_TYPEDEF, '~', &out_see_typedef, 0, 0, 0), \
	X(SEE_DATA,    '~', &out_see_data, 0, 0, 0), \
	/* Like `this` or _this_. */ \
	X(MATH_BEGIN,  '~', &out_math_begin, 0, 0, 0), \
	X(MATH_END,    '~', &out_math_end, 0, 0, 0), \
	X(EM_BEGIN,    '~', &out_em_begin, 0, 0, 0), \
	X(EM_END,      '~', &out_em_end, 0, 0, 0), \
	/* Like @param[a, b, c]. */ \
	X(DOC_LEFT,    '~', 0, 0, 0, 0), \
	X(DOC_RIGHT,   '~', 0, 0, 0, 0), \
	X(DOC_ID,      '~', &out_lit, 0, 0, 0), \
	X(DOC_COMMA,   '~', &out_lit, 0, 0, 0), \
	/* List items. */ \
	X(LIST_ITEM,   '~', &out_list, 0, 0, 0), \
	/* Preformatted. */ \
	X(PREFORMATTED, '~', &out_pre, 0, 0, 0)

#define X(a, b, c, d, e, f) a
enum symbol { SYMBOL };
#undef X
#define X(a, b, c, d, e, f) #a
static const char *const symbols[] = { SYMBOL };
#undef X
#define X(a, b, c, d, e, f) b
static const char symbol_marks[] = { SYMBOL };
#undef X

#endif
