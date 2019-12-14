
/** Don't change the styles without changing this. */
enum StylePunctuate { ST_NO_STYLE, ST_PLAIN, ST_PAREN, ST_LICENSE, ST_CSV,
	ST_SSV, ST_TO_RAW, ST_TO_HTML, ST_TITLE, ST_DIV, ST_P, ST_UL, ST_LI,
	ST_CODE, ST_PRE, ST_PRELINE, ST_H1, ST_H2, ST_H3, ST_DL, ST_DT, ST_DD,
	ST_DESC, ST_EM, ST_STRONG, ST_EM_HTML, ST_STRONG_HTML };

void Style_(void);
void StylePush(const enum StylePunctuate);
void StylePop(void);
void StylePopStrong(void);
void StylePopPush(void);
void StyleSeparate(void);
void StyleExpect(const enum StylePunctuate);
void StyleFlushSymbol(const enum Symbol symbol);
void StyleFlush(void);
void StyleHighlightOn(const enum StylePunctuate);
void StyleHighlightOff(void);

void StyleEncode(const char *const str);
