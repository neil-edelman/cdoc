/* `Sorter.re` is processed into `Sorter.re.h` and included in
 `Scanner.re.cnd`, (which is processed into `Scanner.re.cnd.c`.)
 This is because we are very lazy and want two `re2c` passes. */

/** This holds the sorting state information for `append`. */
static struct {
	struct Segment *segment;
	struct TagArray *tag_array;
	struct TokenArray *current;
} sorter;

/*!re2c
re2c:yyfill:enable   = 0;
re2c:define:YYCTYPE  = char;
re2c:define:YYCURSOR = cursor;
re2c:define:YYMARKER = marker;
suffix = [A-Z_]* "\x00";
*/

/** This appends the current token based on the global state in `sorter`.
 @return Success. */
static int sort(const enum Symbol symbol) {
	struct TokenArray *chosen = 0;
	struct Token *token = 0;
	enum { DISCARD, SORT_CODE, SORT_WORD, SORT_BEGIN_TAG }
		sort = DISCARD;
	const char *const symbol_string = symbols[symbol], *cursor = symbol_string,
		*marker;
	assert(symbol);
	/* Decide what to do based on the prefix. */
/*!re2c
	* { goto end; }
	"C_" suffix { sort = SORT_CODE; goto end; }
	("DOC_" | "MATH_" | "PARAM_") suffix { sort = SORT_WORD; goto end; }
	"TAG_" suffix { sort = SORT_BEGIN_TAG; goto end; }
*/
end:
	/* That's weird but, okay. */
	if(sort == DISCARD) return fprintf(stderr,
		"sort: Symbol %s is unspecified.\n", symbol_string), 1;
	/* Make a new segment if needed. */
	if(!sorter.segment && !(sorter.tag_array = 0, sorter.current = 0,
		sorter.segment = SegmentArrayNew(&doc))) return 0;
	/* Choose which `TokenArray`. */
	switch(sort) {
	case SORT_CODE: chosen = &sorter.segment->code; break;
	case SORT_WORD:
		/*chosen = TagArrayPop(sorter.tag_array);*/ /* careful! */
		if(!chosen) chosen = &sorter.segment->doc;
		break;
	case SORT_BEGIN_TAG:
		chosen = &sorter.segment->doc; break; /* fixme */
	default:
		assert(0);
	}
	assert(chosen);
	/* Make the token. */
	if(!(token = TokenArrayNew(chosen))) return 0;
	token_current(token, symbol);
	return 1;
}
