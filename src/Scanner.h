/* X-Marco is great for debug. */
#define PARAM(A) A
#define STRINGISE(A) #A
#define PARAM2_A(A, B) A
#define PARAM2_B(A, B) B
#define STRINGISE2_A(A, B) #A

/* Define the symbols. */
#define SYMBOL(X) \
	X(END, '~'), X(BEGIN_DOC, '~'), \
	/* `C` syntax. */ \
	X(OPERATOR, '*'), X(COMMA, ','), X(SEMI, ';'), \
	X(LBRACE, '{'), X(RBRACE, '}'), X(LPAREN, '('), X(RPAREN, ')'), \
	X(LBRACK, '['), X(RBRACK, ']'), X(CONSTANT, '#'), X(ID, 'x'), \
	X(ID_ONE_GENERIC, '1'), X(ID_TWO_GENERICS, '2'), \
	X(ID_THREE_GENERICS, '3'), X(STRUCT, 's'), X(UNION, 's'), X(ENUM, 's'), \
	X(TYPEDEF, 't'), X(STATIC, 'z'), X(VOID, 'v'), X(END_BLOCK, ';'), \
	/* Document syntax. */ \
	X(TAG_TITLE, '@'), X(TAG_PARAM, '@'), X(TAG_AUTHOR, '@'), X(TAG_STD, '@'), \
	X(TAG_DEPEND, '@'), X(TAG_VERSION, '@'), X(TAG_SINCE, '@'), \
	X(TAG_FIXME, '@'), X(TAG_DEPRICATED, '@'), X(TAG_RETURN, '@'), \
	X(TAG_THROWS, '@'), X(TAG_IMPLEMENTS, '@'), \
	X(TAG_ORDER, '@'), X(TAG_ALLOW, '@'), \
	/* Meaning/escapes document syntax. */ \
	X(ESCAPED_BACKSLASH, '\\'), X(ESCAPED_BACKQUOTE, '\\'), \
	X(ESCAPED_EACH, '\\'), X(ESCAPED_UNDERSCORE, '\\'), \
	X(ESCAPED_ASTERISK, '\\'), \
	X(BS_URL, '\\'), X(BS_CITE, '\\'), X(BS_SEE, '\\'), \
	X(BS_PRE, '\\'), X(BACKQUOTE, '`'), X(ITALICS, '_'), X(DOC_LBRACE, '<'), \
	X(DOC_RBRACE, '>'), X(DOC_COMMA, '.'), X(NEWLINE, 'n'), X(WORD, 'w'), \
	/* Also do these from LaTeX to HTML. */ \
	X(HTML_AMP, '&'), X(HTML_LT, '&'), X(HTML_GT, '&'), X(HTML_DOT, '&'), \
	X(HTML_LCEIL, '&'), X(HTML_RCEIL, '&'), X(HTML_LFLOOR, '&'), \
	X(HTML_RFLOOR, '&'), X(HTML_TO, '&'), X(HTML_GE, '&'), X(HTML_LE, '&'), \
	X(HTML_NE, '&'), X(HTML_CAP, '&'), X(HTML_CUP, '&'), X(HTML_VEE, '&'), \
	X(HTML_WEDGE, '&'), X(HTML_SUM, '&'), X(HTML_PROD, '&'), \
	X(HTML_IN, '&'), X(HTML_EXISTS, '&'), X(HTML_FORALL, '&'), \
	X(HTML_NEG, '&'), X(HTML_TIMES, '&'), X(HTML_SQRT, '&'), \
	X(HTML_PROPTO, '&'), X(HTML_PM, '&'), X(HTML_PARTIAL, '&'), \
	X(HTML_INT, '&'), X(HTML_INFTY, '&'), \
	X(HTML_UGAMMA, '&'), X(HTML_UDELTA, '&'), X(HTML_ILAMBDA, '&'), \
	X(HTML_UPHI, '&'), X(HTML_UPI, '&'), X(HTML_UPSY, '&'), \
	X(HTML_USIGMA, '&'), X(HTML_UTHETA, '&'), \
	X(HTML_UUPSILON, '&'), X(HTML_UXI, '&'), \
	X(HTML_UOMEGA, '&'), X(HTML_ALPHA, '&'), X(HTML_BETA, '&'), \
	X(HTML_GAMMA, '&'), X(HTML_DELTA, '&'), \
	X(HTML_EPSILON, '&'), X(HTML_ZETA, '&'), X(HTML_ETA, '&'), \
	X(HTML_THETA, '&'), X(HTML_IOTA, '&'), X(HTML_KAPPA, '&'), \
	X(HTML_LAMBDA, '&'), X(HTML_MU, '&'), X(HTML_NU, '&'), \
	X(HTML_XI, '&'), X(HTML_RHO, '&'), X(HTML_SIGMA, '&'), \
	X(HTML_TAU, '&'), X(HTML_UPSILON, '&'), X(HTML_PHI, '&'), \
	X(HTML_CHI, '&'), X(HTML_PSI, '&'), X(HTML_OMEGA, '&')

enum Symbol { SYMBOL(PARAM2_A) };
static const char *const symbols[] = { SYMBOL(STRINGISE2_A) };
static const char symbol_mark[] = { SYMBOL(PARAM2_B) };

/* Define the states of the input file. */
#define STATE(X) X(END_OF_FILE, &scan_eof), X(DOC, &scan_doc), \
	X(CODE, &scan_code), X(COMMENT, &scan_comment), X(STRING, &scan_string), \
	X(CHAR, &scan_char), X(MACRO, &scan_macro)

enum State { STATE(PARAM2_A) };

/* Define the sections of output. */
#define SECTION(X) X(HEADER), X(DECLARATION), X(FUNCTION)
enum Section { SECTION(PARAM) };

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
