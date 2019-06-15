/* X-Marco is great for debug. */
#define PARAM(A) A
#define STRINGISE(A) #A
#define PARAM_A(A, B) A
#define PARAM_B(A, B) B
#define STRINGISE_A(A, B) #A

/* Define the symbols. */
#define SYMBOL(X) \
	X(END, 0), X(OPERATOR, 0), X(COMMA, 0), X(SEMI, 0), \
	X(LBRACE, 0), X(RBRACE, 0), X(LPAREN, 0), X(RPAREN, 0), \
	X(LBRACK, 0), X(RBRACK, 0), X(CONSTANT, 0), X(ID, 0), \
	X(STRUCT, 0), X(UNION, 0), X(TYPEDEF, 0), X(PREPROCESSOR, 0), \
	X(BEGIN_DOC, 0), \
	X(ESCAPED_BACKSLASH, 0), X(ESCAPED_LBRACE, 0), X(ESCAPED_RBRACE, 0), \
	X(ESCAPED_EACH, 0), X(WHITESPACE, 0), X(NEWLINE, 0), \
	X(BS_URL, 0), X(BS_CITE, 0), X(BS_SEE, 0), X(BS_PRE, 0), \
	X(TAG_TITLE, 0), X(TAG_PARAM, 0), X(TAG_AUTHOR, 0), \
	X(TAG_STD, 0), X(TAG_DEPEND, 0), X(TAG_VERSION, 0), X(TAG_SINCE, 0), \
	X(TAG_FIXME, 0), X(TAG_DEPRICATED, 0), X(TAG_RETURN, 0), X(TAG_THROWS, 0), \
	X(TAG_IMPLEMENTS, 0), X(TAG_ORDER, 0), X(TAG_ALLOW, 0), \
	X(HTML_AMP, 0), X(HTML_LT, 0), X(HTML_GT, 0), \
	X(HTML_DOT, 0), X(HTML_LCEIL, 0), X(HTML_RCEIL, 0), X(HTML_LFLOOR, 0), \
	X(HTML_RFLOOR, 0), X(HTML_TO, 0), X(HTML_GE, 0), X(HTML_LE, 0), \
	X(HTML_NE, 0), X(HTML_CAP, 0), X(HTML_CUP, 0), X(HTML_VEE, 0), \
	X(HTML_WEDGE, 0), X(HTML_SUM, 0), X(HTML_PROD, 0), X(HTML_IN, 0), \
	X(HTML_EXISTS, 0), X(HTML_FORALL, 0), X(HTML_NEG, 0), X(HTML_TIMES, 0), \
	X(HTML_SQRT, 0), X(HTML_PROPTO, 0), X(HTML_PM, 0), X(HTML_PARTIAL, 0), \
	X(HTML_INT, 0), X(HTML_INFTY, 0), X(HTML_UGAMMA, 0), X(HTML_UDELTA, 0), \
	X(HTML_ILAMBDA, 0), X(HTML_UPHI, 0), X(HTML_UPI, 0), X(HTML_UPSY, 0), \
	X(HTML_USIGMA, 0), X(HTML_UTHETA, 0), X(HTML_UUPSILON, 0), X(HTML_UXI, 0), \
	X(HTML_UOMEGA, 0), X(HTML_ALPHA, 0), X(HTML_BETA, 0), X(HTML_GAMMA, 0), \
	X(HTML_DELTA, 0), X(HTML_EPSILON, 0), X(HTML_ZETA, 0), X(HTML_ETA, 0), \
	X(HTML_THETA, 0), X(HTML_IOTA, 0), X(HTML_KAPPA, 0), X(HTML_LAMBDA, 0), \
	X(HTML_MU, 0), X(HTML_NU, 0), X(HTML_XI, 0), X(HTML_RHO, 0), \
	X(HTML_SIGMA, 0), X(HTML_TAU, 0), X(HTML_UPSILON, 0), X(HTML_PHI, 0), \
	X(HTML_CHI, 0), X(HTML_PSI, 0), X(HTML_OMEGA, 0)

enum Symbol { SYMBOL(PARAM_A) };
static const char *const symbols[] = { SYMBOL(STRINGISE_A) };

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

enum State { STATE(PARAM_A) };

struct Scanner;

void Scanner_(void);
int Scanner(void);
int ScannerNext(void);
void ScannerToken(struct Token *const token);
void ScannerTokenInfo(struct TokenInfo *const info);
const char *ScannerStates(void);
