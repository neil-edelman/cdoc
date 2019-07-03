/* X-Marco is great for debug. */
#define PARAM(A) A
#define STRINGISE(A) #A
#define PARAM2_A(A, B) A
#define PARAM2_B(A, B) B
#define STRINGISE2_A(A, B) #A
#define PARAM3_A(A, B, C) A
#define PARAM3_B(A, B, C) B
#define PARAM3_C(A, B, C) C
#define STRINGISE3_A(A, B, C) #A

/* Define the symbols. */
#define SYMBOL(X) \
	X(END, '~', 0), X(BEGIN_DOC, '~', 0), \
	/* `C` syntax. */ \
	X(OPERATOR, '*', &lit), X(COMMA, ',', &lit), X(SEMI, ';', &lit), \
	X(LBRACE, '{', &lit), X(RBRACE, '}', &lit), X(LPAREN, '(', &lit), \
	X(RPAREN, ')', &lit), X(LBRACK, '[', &lit), X(RBRACK, ']', &lit), \
	X(CONSTANT, '#', &lit), X(ID, 'x', &lit), \
	X(ID_ONE_GENERIC, '1', &gen1), X(ID_TWO_GENERICS, '2', &gen2), \
	X(ID_THREE_GENERICS, '3', &gen3), X(STRUCT, 's', &lit), \
	X(UNION, 's', &lit), X(ENUM, 's', &lit), X(TYPEDEF, 't', &lit), \
	X(STATIC, 'z', &lit), X(VOID, 'v', &lit), X(END_BLOCK, ';', 0), \
	/* Document syntax. */ \
	X(TAG_TITLE, '@', &lit), X(TAG_PARAM, '@', &lit), \
	X(TAG_AUTHOR, '@', &lit), X(TAG_STD, '@', &lit), X(TAG_DEPEND, '@', &lit), \
	X(TAG_VERSION, '@', &lit), X(TAG_SINCE, '@', &lit), \
	X(TAG_FIXME, '@', &lit), X(TAG_DEPRICATED, '@', &lit), \
	X(TAG_RETURN, '@', &lit), X(TAG_THROWS, '@', &lit), \
	X(TAG_IMPLEMENTS, '@', &lit), X(TAG_ORDER, '@', &lit), \
	X(TAG_ALLOW, '@', &lit), \
	/* Meaning/escapes document syntax. */ \
	X(ESCAPED_BACKSLASH, '\\', &esc_bs), X(ESCAPED_BACKQUOTE, '\\', &esc_bq), \
	X(ESCAPED_EACH, '\\', &esc_each), X(ESCAPED_UNDERSCORE, '\\', &esc_under), \
	/*X(ESCAPED_ASTERISK, '\\', &esc_ast),*/ \
	X(URL, '\\', &url), X(CITE, '\\', &cite), X(SEE, '\\', &see), \
	X(MATH, '$', &math), X(ITALICS, '_', &it), X(DOC_LBRACE, '<', &lb), \
	X(DOC_RBRACE, '>', &rb), X(DOC_COMMA, '.', &lit), X(NEWLINE, 'n', &par), \
	X(WORD, 'w', &lit), \
	/* Also do these from LaTeX to HTML. */ \
	X(HTML_AMP, '&', &esc_amp), X(HTML_LT, '&', &esc_lt), \
	X(HTML_GT, '&', &esc_gt)

enum Symbol { SYMBOL(PARAM3_A) };
static const char *const symbols[] = { SYMBOL(STRINGISE3_A) };
static const char symbol_mark[] = { SYMBOL(PARAM3_B) };

/* Define the states of the input file. */
#define STATE(X) X(END_OF_FILE, &scan_eof), X(DOC, &scan_doc), \
	X(DOC_MATH, &scan_doc_math), X(CODE, &scan_code), \
	X(COMMENT, &scan_comment), X(STRING, &scan_string), \
	X(CHAR, &scan_char), X(MACRO, &scan_macro)

enum State { STATE(PARAM2_A) };

/* Define the sections of output. */
#define SECTION(X) X(HEADER), X(DECLARATION), X(FUNCTION)
enum Section { SECTION(PARAM) };
static const char *const sections[] = { SECTION(STRINGISE) };

/** `Token` has a `Symbol` and is associated with an area of the text.
 Tokenisation can only to done when the `Scanner` is active and in
 steady-state, and is done by the lexer calling `ScannerFillToken`. */
struct Token {
	enum Symbol symbol;
	const char *from;
	int length;
	size_t line;
};

/** `TokenInfo` is associated to the `Token`, but not stored. */
struct TokenInfo {
	int indent_level;
	int is_doc, is_doc_far;
	enum State state;
};

struct Scanner;

void Scanner_(void);
int Scanner(void);
int ScannerNext(void);
void ScannerIgnoreBlock(void);
void ScannerToken(struct Token *const token);
void ScannerTokenInfo(struct TokenInfo *const info);
const char *ScannerStates(void);
