#include <stddef.h>

#include "format.h"
#include "symbol.h"

#define HTML_AMP "&amp;"
#define HTML_GT  "&gt;"
#define HTML_LT  "&lt;"

/** Don't change the styles without changing this. */
enum style_punctuate { ST_NO_STYLE, ST_PLAIN, ST_PAREN, ST_LICENSE, ST_CSV,
	ST_SSV, ST_TO_RAW, ST_TO_HTML, ST_TITLE, ST_DIV, ST_P, ST_UL, ST_LI,
	ST_CODE, ST_PRE, ST_PRELINE, ST_H1, ST_H2, ST_H3, ST_DL, ST_DT, ST_DD,
	ST_EM, ST_STRONG, ST_EM_HTML, ST_STRONG_HTML };

enum format style_format(void);
void style_(void);
void style_push(const enum style_punctuate);
void style_pop(void);
void style_pop_strong(void);
void style_pop_push(void);
int style_Is_top(const enum style_punctuate p);
int style_is_empty(void);
void style_separate(void);
void style_expect(const enum style_punctuate);
void style_flush_symbol(const enum symbol symbol);
void style_flush(void);
void style_highlight_on(const enum style_punctuate);
void style_highlight_off(void);
void style_encode_length(const size_t length, const char *const from);
void style_encode(const char *const str);
const char *style_encode_length_cat_to_buffer(const size_t length,
	const char *const from);
const char *style_encode_length_raw_to_buffer(const size_t length,
	const char *const from);
