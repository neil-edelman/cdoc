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

#define SYMBOL_OUTPUT(X) \
	X(OUT_DISCARD), X(OUT_C), X(OUT_DOC), X(OUT_TAG)

/* Define the symbols. */
#define SYMBOL(X) \
	X(END, OUT_DISCARD, '~'), X(OPERATOR, OUT_C, '*'), X(COMMA, OUT_C, ','), \
	X(SEMI, OUT_C, ';'), X(LBRACE, OUT_C, '{'), X(RBRACE, OUT_C, '}'), X(LPAREN, OUT_C, '('), \
	X(RPAREN, OUT_C, ')'), X(LBRACK, OUT_C, '['), X(RBRACK, OUT_C, ']'), \
	X(CONSTANT, OUT_C, '#'), X(ID, OUT_C, 'x'), X(ID_ONE_GENERIC, OUT_C, 'g'), \
	X(ID_TWO_GENERICS, OUT_C, 'h'), X(ID_THREE_GENERICS, OUT_C, 'i'), X(STRUCT, OUT_C, 's'), \
	X(UNION, OUT_C, 'u'), X(ENUM, OUT_C, 'e'), X(TYPEDEF, OUT_C, 't'), \
	X(STATIC, OUT_C, 'z'), \
	X(BEGIN_DOC, OUT_DISCARD, '~'), \
	X(TAG_TITLE, OUT_TAG, '@'), X(TAG_PARAM, OUT_TAG, '@'), X(TAG_AUTHOR, OUT_TAG, '@'), \
	X(TAG_STD, OUT_TAG, '@'), X(TAG_DEPEND, OUT_TAG, '@'), X(TAG_VERSION, OUT_TAG, '@'), \
	X(TAG_SINCE, OUT_TAG, '@'), X(TAG_FIXME, OUT_TAG, '@'), \
	X(TAG_DEPRICATED, OUT_TAG, '@'), X(TAG_RETURN, OUT_TAG, '@'), \
	X(TAG_THROWS, OUT_TAG, '@'), X(TAG_IMPLEMENTS, OUT_TAG, '@'), \
	X(TAG_ORDER, OUT_TAG, '@'), X(TAG_ALLOW, OUT_TAG, '@'), \
	X(ESCAPED_BACKSLASH, OUT_DOC, '@'), X(ESCAPED_BACKQUOTE, OUT_DOC, '@'), \
	X(ESCAPED_EACH, OUT_DOC, '@'), X(BACKQUOTE, OUT_DOC, '`'), \
	X(DOC_LBRACE, OUT_DOC, '<'), X(DOC_RBRACE, OUT_DOC, '>'), X(DOC_COMMA, OUT_DOC, '.'), \
	X(NEWLINE,	OUT_DOC, 'n'), X(WORD, OUT_DOC, 'w'), \
	X(BS_URL, OUT_DOC, '\\'), X(BS_CITE, OUT_DOC, '\\'), X(BS_SEE, OUT_DOC, '\\'), \
	X(BS_PRE, OUT_DOC, '\\'), \
	X(HTML_AMP, OUT_DOC, '&'), X(HTML_LT, OUT_DOC, '&'), X(HTML_GT, OUT_DOC, '&'), \
	X(HTML_DOT, OUT_DOC, '&'), X(HTML_LCEIL, OUT_DOC, '&'), X(HTML_RCEIL, OUT_DOC, '&'), \
	X(HTML_LFLOOR, OUT_DOC, '&'), X(HTML_RFLOOR, OUT_DOC, '&'), X(HTML_TO, OUT_DOC, '&'), \
	X(HTML_GE, OUT_DOC, '&'), X(HTML_LE, OUT_DOC, '&'), X(HTML_NE, OUT_DOC, '&'), \
	X(HTML_CAP, OUT_DOC, '&'), X(HTML_CUP, OUT_DOC, '&'), X(HTML_VEE, OUT_DOC, '&'), \
	X(HTML_WEDGE, OUT_DOC, '&'), X(HTML_SUM, OUT_DOC, '&'), X(HTML_PROD, OUT_DOC, '&'), \
	X(HTML_IN, OUT_DOC, '&'), X(HTML_EXISTS, OUT_DOC, '&'), X(HTML_FORALL, OUT_DOC, '&'), \
	X(HTML_NEG, OUT_DOC, '&'), X(HTML_TIMES, OUT_DOC, '&'), X(HTML_SQRT, OUT_DOC, '&'), \
	X(HTML_PROPTO, OUT_DOC, '&'), X(HTML_PM, OUT_DOC, '&'), X(HTML_PARTIAL, OUT_DOC, '&'), \
	X(HTML_INT, OUT_DOC, '&'), X(HTML_INFTY, OUT_DOC, '&'), X(HTML_UGAMMA, OUT_DOC, '&'), \
	X(HTML_UDELTA, OUT_DOC, '&'), X(HTML_ILAMBDA, OUT_DOC, '&'), \
	X(HTML_UPHI, OUT_DOC, '&'), X(HTML_UPI, OUT_DOC, '&'), X(HTML_UPSY, OUT_DOC, '&'), \
	X(HTML_USIGMA, OUT_DOC, '&'), X(HTML_UTHETA, OUT_DOC, '&'), \
	X(HTML_UUPSILON, OUT_DOC, '&'), X(HTML_UXI, OUT_DOC, '&'), \
	X(HTML_UOMEGA, OUT_DOC, '&'), X(HTML_ALPHA, OUT_DOC, '&'), X(HTML_BETA, OUT_DOC, '&'), \
	X(HTML_GAMMA, OUT_DOC, '&'), X(HTML_DELTA, OUT_DOC, '&'), \
	X(HTML_EPSILON, OUT_DOC, '&'), X(HTML_ZETA, OUT_DOC, '&'), X(HTML_ETA, OUT_DOC, '&'), \
	X(HTML_THETA, OUT_DOC, '&'), X(HTML_IOTA, OUT_DOC, '&'), X(HTML_KAPPA, OUT_DOC, '&'), \
	X(HTML_LAMBDA, OUT_DOC, '&'), X(HTML_MU, OUT_DOC, '&'), X(HTML_NU, OUT_DOC, '&'), \
	X(HTML_XI, OUT_DOC, '&'), X(HTML_RHO, OUT_DOC, '&'), X(HTML_SIGMA, OUT_DOC, '&'), \
	X(HTML_TAU, OUT_DOC, '&'), X(HTML_UPSILON, OUT_DOC, '&'), X(HTML_PHI, OUT_DOC, '&'), \
	X(HTML_CHI, OUT_DOC, '&'), X(HTML_PSI, OUT_DOC, '&'), X(HTML_OMEGA, OUT_DOC, '&')

enum Symbol { SYMBOL(PARAM3_A) };
static const char *const symbols[] = { SYMBOL(STRINGISE3_A) };

/* Define the states of the input file. */
#define STATE(X) X(END_OF_FILE, &scan_eof), X(DOC, &scan_doc), \
	X(CODE, &scan_code), X(COMMENT, &scan_comment), X(STRING, &scan_string), \
	X(CHAR, &scan_char), X(MACRO, &scan_macro)

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
};

enum State { STATE(PARAM2_A) };

struct Scanner;

void Scanner_(void);
int Scanner(void);
int ScannerNext(void);
void ScannerIgnoreBlock(void);
void ScannerToken(struct Token *const token);
void ScannerTokenInfo(struct TokenInfo *const info);
const char *ScannerStates(void);
