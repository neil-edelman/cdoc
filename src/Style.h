#include "Format.h"

#define HTML_AMP "&amp;"
#define HTML_GT  "&gt;"
#define HTML_LT  "&lt;"

/** Don't change the styles without changing this. */
enum StylePunctuate { ST_NO_STYLE, ST_PLAIN, ST_PAREN, ST_LICENSE, ST_CSV,
	ST_SSV, ST_TO_RAW, ST_TO_HTML, ST_TITLE, ST_DIV, ST_P, ST_UL, ST_LI,
	ST_CODE, ST_PRE, ST_PRELINE, ST_H1, ST_H2, ST_H3, ST_DL, ST_DT, ST_DD,
	ST_DESC, ST_EM, ST_STRONG, ST_EM_HTML, ST_STRONG_HTML };

enum Format StyleFormat(void);
void Style_(void);
void StylePush(const enum StylePunctuate);
void StylePop(void);
void StylePopStrong(void);
void StylePopPush(void);
int StyleIsTop(const enum StylePunctuate p);
int StyleIsEmpty(void);
void StyleSeparate(void);
void StyleExpect(const enum StylePunctuate);
void StyleFlushSymbol(const enum Symbol symbol);
void StyleFlush(void);
void StyleHighlightOn(const enum StylePunctuate);
void StyleHighlightOff(void);
void StyleEncodeLength(const int length, const char *const from);
void StyleEncode(const char *const str);
const char *StyleEncodeLengthCatToBuffer(const int length,
	const char *const from);
const char *StyleEncodeLengthRawToBuffer(const int length,
	const char *const from);
