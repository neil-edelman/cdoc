/** 2019 Neil Edelman, distributed under the terms of the MIT License,
 \url{ https://opensource.org/licenses/MIT }.

 {Doxygen} should be used if possible. However, maybe one is too hip to use it?
 (or it uses libraries that are too new for one's computer.) The documentation
 is a perversion and simplification of {Doxygen}.

 Parses and extracts the documentation commands in a {.c} file. A documentation
 command begins with {/\**}.

 @title Cdoc.c
 @author Neil
 @version 2019-05 Re-done in {re2c}.
 @since 2017-03 Initial version.
 @std C89
 @depend re2c (\url{http://re2c.org/})
 @fixme Different doc comments need new paragraphs.
 @fixme Lists in comments, etc.
 @fixme {void A_BI_(Create, Thing)(void)} -> {<A>Create<BI>Thing(void)}.
 @fixme Trigraph support. */

#include <stdlib.h> /* EXIT malloc free */
#include <stdio.h>  /* FILE printf fputc perror */
#include <string.h> /* memmove memset */
#include <errno.h>  /* EDOM ERANGE */
/* #define NDEBUG */
#include <assert.h> /* assert */

/* X-Marco is great for debug. */
#define PARAM(A) A
#define STRINGISE(A) #A
#define PARAM_A(A, B) A
#define PARAM_B(A, B) B
#define STRINGISE_A(A, B) #A



/* Definitions. */

/* Define the tokens. */
#define TOKEN(X) \
	X(END, 0), X(OPERATOR, 0), X(COMMA, 0), X(SEMI, 0), \
	X(LBRACE, 0), X(RBRACE, 0), X(LPAREN, 0), X(RPAREN, 0), \
	X(LBRACK, 0), X(RBRACK, 0), X(CONSTANT, 0), X(ID, 0), \
	X(STRUCT, 0), X(UNION, 0), X(TYPEDEF, 0), X(PREPROCESSOR, 0), \
	X(BEGIN_DOC, 0), \
	X(ESCAPED_BACKSLASH, 0), X(ESCAPED_LBRACE, 0), X(ESCAPED_RBRACE, 0), \
	X(ESCAPED_EACH, 0), X(WHITESPACE, 0), X(NEWLINE, 0), \
	X(BS_URL, 0), X(BS_CITE, 0), X(BS_SEE, 0), X(BS_PRE, 0), \
	X(TAG_TITLE, 0), X(TAG_PARAM, 0), X(TAG_AUTHOR, 0), X(TAG_STD, 0), \
	X(TAG_DEPEND, 0), X(TAG_VERSION, 0), X(TAG_SINCE, 0), X(TAG_FIXME, 0), \
	X(TAG_DEPRICATED, 0), X(TAG_RETURN, 0), X(TAG_THROWS, 0), \
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

enum Token { TOKEN(PARAM_A) };
static const char *const tokens[] = { TOKEN(STRINGISE_A) };
static const int token_flags[] = { TOKEN(PARAM_B) };

struct Scanner;
/** {ScannerFn} functions which {re2c} lex. Forward-declared for {state_fn}. */
typedef enum Token (*ScannerFn)(struct Scanner *const);

/* Forward-declare all {ScannerFn}. */
static enum Token scan_eof(struct Scanner *const s);
static enum Token scan_doc(struct Scanner *const s);
static enum Token scan_code(struct Scanner *const s);
static enum Token scan_comment(struct Scanner *const s);
static enum Token scan_string(struct Scanner *const s);
static enum Token scan_char(struct Scanner *const s);
static enum Token scan_macro(struct Scanner *const s);

/* Define the states of the input file. */
#define STATE(X) X(END_OF_FILE, &scan_eof), X(DOC, &scan_doc), \
	X(CODE, &scan_code), X(COMMENT, &scan_comment), X(STRING, &scan_string), \
	X(CHAR, &scan_char), X(MACRO, &scan_macro)
enum State { STATE(PARAM_A) };
static const char *const states[] = { STATE(STRINGISE_A) };
static const ScannerFn state_fn[] = { STATE(PARAM_B) };

/* Define the types out output. */
#define TYPE(X) X(HEADER), X(DECLARATION), X(FUNCTION)
enum Type { TYPE(PARAM) };
static const char *const types[] = { TYPE(STRINGISE) };



/* Define Scanner. */

/* Define {CharArray}, a vector of characters -- dynamic string. */
#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "../src/Array.h"

static void state_to_string(const enum State *s, char (*const a)[12]) {
	strncpy(*a, states[*s], sizeof *a - 1);
	*a[sizeof *a - 1] = '\0';
}

/* Define {StateArray}, a stack of states. */
#define ARRAY_NAME State
#define ARRAY_TYPE enum State
#define ARRAY_TO_STRING &state_to_string
#define ARRAY_STACK
#include "../src/Array.h"

/** Scanner reads a file and puts it in memory. */
struct Scanner {
	/* {re2c} variables. These point directly into {buffer} so no modifying. */
	const char *limit, *cursor, *marker, *ctx_marker, *token;
	/* Weird {c2re} stuff: these fields have to come after when >5? */
	struct CharArray buffer;
	struct StateArray states;
	int indent_level;
	size_t line, doc_line;
};

/** Fill the structure with default values. */
static void zero(struct Scanner *const s) {
	assert(s);
	CharArray(&s->buffer);
	StateArray(&s->states);
	s->indent_level = 0;
	s->line = s->doc_line = 0;
	s->limit = s->cursor = s->marker = s->ctx_marker = s->token = 0;
}

/** Uninitialises {s}.
 @allow */
static void Scanner_(struct Scanner *const s) {
	assert(s);
	CharArray_(&s->buffer);
	StateArray_(&s->states);
	zero(s);
}

/* Have {re2c} generate {YYMAXFILL}.
 \url{ http://re2c.org/examples/example_02.html }. */
/*!max:re2c*/

/** Initialises {s} with the file {fn}.
 @return Success.
 @throws fopen malloc free
 @fixme Does not allow embedded zeros. Limitation of {fgets}?
 @fixme fn is stupid and a hack left over from fread, just read from stdin.
 @allow */
static int Scanner(struct Scanner *const s) {
	const size_t granularity = 1024;
	int is_done = 0;
	assert(s);
	zero(s);
	do { /* Try: read all contents from stdin at once. */
		size_t nread;
		char *buf;
		enum State *ps;
		for( ; ; ) {
			if(!(buf = CharArrayBuffer(&s->buffer, (int)granularity))) break;
			nread = fread(buf, 1, granularity, stdin);
			if(ferror(stdin)) break;
			assert(nread >= 0 && nread <= granularity);
			if(!CharArrayAddSize(&s->buffer, nread)) break;
			if(nread != granularity) { is_done = 1; break; }
		}
		if(!is_done) break;
		/* Fill the past the file with '\0' for fast lexing. */
		is_done = 0;
		if(!(buf = CharArrayBuffer(&s->buffer, YYMAXFILL))) break;
		memset(buf, '\0', YYMAXFILL);
		if(!CharArrayAddSize(&s->buffer, YYMAXFILL)) break;
		/* Point these toward the first char; {s->buffer} is necessarily done
		 growing, or we could not do this. */
		s->cursor = s->marker = s->token = CharArrayNext(&s->buffer, 0);
		s->line = 1;
		if(!(ps = StateArrayNew(&s->states))) break;
		*ps = CODE;
		is_done = 1;
	} while(0); if(!is_done) { /* Catch. */
		Scanner_(s);
	}
	return is_done;
}

/** Scans the file for the next token.
 @return The next token; {END} when it's finished. */
static enum Token ScannerScan(struct Scanner *const s) {
	enum State *ps;
	if(!s || !(ps = StateArrayPeek(&s->states))) return END;
	return state_fn[*ps](s);
}

/** @return The state of {s}. */
static enum State state_look(struct Scanner *const s) {
	enum State *ps;
	assert(s);
	if(!(ps = StateArrayPeek(&s->states))) return END_OF_FILE;
	return *ps;
}

/** Pushes the new state.
 @return Success. */
static int push(struct Scanner *const s, const enum State state) {
	enum State *ps;
	assert(s);
	if(!(ps = StateArrayNew(&s->states))) return 0;
	*ps = state;
	return 1;
}

/** Pushes the new state and calls the state function.
 @return What the state function returns. END if there's an error. */
static enum Token push_call(struct Scanner *const s, const enum State state) {
	assert(s);
	if(!(push(s, state))) return END;
	return state_fn[state](s);
}

/** Pops and calls the state on top.
 @return Success. */
static int pop(struct Scanner *const s) {
	assert(s);
	return !!StateArrayPop(&s->states);
}

/** Pops and calls the state on top and calls the state underneath.
 @return What the state function returns. END if there's an error or the state
 is empty. */
static enum Token pop_call(struct Scanner *const s) {
	enum State *ps;
	assert(s);
	if(!pop(s) || !(ps = StateArrayPeek(&s->states))) return END;
	return state_fn[*ps](s);
}



/* {ScannerFn}'s. */

/*!re2c
re2c:define:YYCTYPE  = char;
re2c:define:YYFILL   = "return END;";
re2c:define:YYFILL:naked = 1;
re2c:define:YYLIMIT  = s->limit;
re2c:define:YYCURSOR = s->cursor;
re2c:define:YYMARKER = s->marker; // Rules overlap.
// Might or might not require this (depending on the version?)
re2c:define:YYCTXMARKER = s->ctx_marker;
// Don't know what this does, but it fails without it.
re2c:yyfill:enable = 0;
*/

/** Returns eof.
 @implements ScannerFn
 @allow */
static enum Token scan_eof(struct Scanner *const s) { (void)s; return END; }

/** Scans documentation.
 @implements ScannerFn
 @allow */
static enum Token scan_doc(struct Scanner *const s) {
	assert(s && state_look(s) == DOC);
	s->doc_line = s->line;
	s->token = s->cursor;
doc:
/*!re2c
	"\x00" { if(s->limit - s->cursor <= YYMAXFILL) return END; goto doc; }
	"*""/" { return pop_call(s); }
	* { goto doc; }

	whitespace = [ \t\v\f];
	whitespace+ { return WHITESPACE; }

	newline = "\n" | "\r" "\n"?;
	art = "*"? newline " *";
	newline { s->line++; return NEWLINE; }
	art / [^/] { s->line++; return NEWLINE; }

	word = [^ \t\n\v\f\r\\,@{}&<>*]*; // This is kind of sketchy.
	word { return ID; }

	"\\\\" { return ESCAPED_BACKSLASH; }
	"\\{" { return ESCAPED_LBRACE; }
	"\\}" { return ESCAPED_RBRACE; }
	"\\@" { return ESCAPED_EACH; }
	"{" { return LBRACE; }
	"}" { return RBRACE; }
	"," { return COMMA; }

	// These are recognised in the documentation as stuff.
	"\\url" { return BS_URL; }
	"\\cite" { return BS_CITE; }
	"\\see" { return BS_SEE; }
	"\\$" { return BS_PRE; }

	// These are tags.
	"@title" { return TAG_TITLE; }
	"@param" { return TAG_PARAM; }
	"@author" { return TAG_AUTHOR; }
	"@std" { return TAG_STD; }
	"@depend" { return TAG_DEPEND; }
	"@version" { return TAG_VERSION; }
	"@since" { return TAG_SINCE; }
	"@fixme" { return TAG_FIXME; }
	"@deprecated" { return TAG_DEPRICATED; }
	"@return" { return TAG_RETURN; }
	"@throws" { return TAG_THROWS; }
	"@implements" { return TAG_IMPLEMENTS; }
	"@order" { return TAG_ORDER; }
	"@allow" { return TAG_ALLOW; }

	// Also escape these for {HTML}.
	"&" { return HTML_AMP; }
	"<" { return HTML_LT; }
	">" { return HTML_GT; }
	
	// Also provide these for convenience: common compsci math and Greek.
	"\\dot" { return HTML_DOT; }
	"\\lceil" { return HTML_LCEIL; }
	"\\rceil" { return HTML_RCEIL; }
	"\\lfloor" { return HTML_LFLOOR; }
	"\\rfloor" { return HTML_RFLOOR; }
	"\\to" { return HTML_TO; }
	"\\ge" { return HTML_GE; }
	"\\le" { return HTML_LE; }
	"\\ne" { return HTML_NE; }
	"\\cap" { return HTML_CAP; }
	"\\cup" { return HTML_CUP; }
	"\\vee" { return HTML_VEE; }
	"\\wedge" { return HTML_WEDGE; }
	"\\sum" { return HTML_SUM; }
	"\\prod" { return HTML_PROD; }
	"\\in" { return HTML_IN; }
	"\\exists" { return HTML_EXISTS; }
	"\\forall" { return HTML_FORALL; }
	"\\neg" { return HTML_NEG; }
	"\\times" { return HTML_TIMES; }
	"\\sqrt" { return HTML_SQRT; }
	"\\propto" { return HTML_PROPTO; }
	"\\pm" { return HTML_PM; }
	"\\partial" { return HTML_PARTIAL; }
	"\\int" { return HTML_INT; }
	"\\infty" { return HTML_INFTY; }
	"\\Gamma" { return HTML_UGAMMA; }
	"\\Delta" { return HTML_UDELTA; }
	"\\Lambda" { return HTML_ILAMBDA; }
	"\\Phi" { return HTML_UPHI; }
	"\\Pi" { return HTML_UPI; }
	"\\Psi" { return HTML_UPSY; }
	"\\Sigma" { return HTML_USIGMA; }
	"\\Theta" { return HTML_UTHETA; }
	"\\Upsilon" { return HTML_UUPSILON; }
	"\\Xi" { return HTML_UXI; }
	"\\Omega" { return HTML_UOMEGA; }
	"\\alpha" { return HTML_ALPHA; }
	"\\beta" { return HTML_BETA; }
	"\\gamma" { return HTML_GAMMA; }
	"\\delta" { return HTML_DELTA; }
	"\\epsilon" { return HTML_EPSILON; }
	"\\zeta" { return HTML_ZETA; }
	"\\eta" { return HTML_ETA; }
	"\\theta" { return HTML_THETA; }
	"\\iota" { return HTML_IOTA; }
	"\\kappa" { return HTML_KAPPA; }
	"\\lambda" { return HTML_LAMBDA; }
	"\\mu" { return HTML_MU; }
	"\\nu" { return HTML_NU; }
	"\\xi" { return HTML_XI; }
	"\\rho" { return HTML_RHO; }
	"\\sigma" { return HTML_SIGMA; }
	"\\tau" { return HTML_TAU; }
	"\\upsilon" { return HTML_UPSILON; }
	"\\phi" { return HTML_PHI; }
	"\\chi" { return HTML_CHI; }
	"\\psi" { return HTML_PSI; }
	"\\omega" { return HTML_OMEGA; }
*/
}

/** Scans C code. See \see{ http://re2c.org/examples/example_07.html }.
 @implements ScannerFn
 @allow */
static enum Token scan_code(struct Scanner *const s) {
	assert(s && state_look(s) == CODE);
code:
	s->token = s->cursor;
/*!re2c
	// http://re2c.org/examples/example_03.html
	"\x00" { if(s->limit - s->cursor <= YYMAXFILL) return END; goto code; }
	* { printf("Unknown, <%.*s>, just roll with it.\n",
		(int)(s->cursor - s->token), s->token); goto code; }

	newline { s->line++; goto code; }

	// Documentation is not / * * /, but other then that.
	doc = "/""**";
	doc / [^/] { return push(s, DOC) ? BEGIN_DOC : END; }

	comment = "/""*";
	comment { return push_call(s, COMMENT); }
	cxx_comment = "//" [^\n]*;
	whitespace+ | cxx_comment { goto code; }

	macro = ("#" | "%:");
	macro { return push_call(s, MACRO); }

	// Number separate from minus then numbers can't have whitespace, simple.
	oct = "0" [0-7]*;
	dec = [1-9][0-9]*;
	hex = '0x' [0-9a-fA-F]+;
	frc = [0-9]* "." [0-9]+ | [0-9]+ ".";
	exp = 'e' [+-]? [0-9]+;
	flt = (frc exp? | [0-9]+ exp) [fFlL]?;
	number = (oct | dec | hex | flt) [uUlL]*;
	number { return CONSTANT; }

	// Strings are more complicated because they can have whitespace and that
	// messes up the line count.
	// @fixme No trigraph support.
	// char_type = "u8"|"u"|"U"|"L"; <- These get caught in id; don't care.
	"L"? "\"" { return push_call(s, STRING); }
	"'" { return push_call(s, CHAR); }

	id = [a-zA-Z_][a-zA-Z_0-9]*;
	id           { return ID; }

	operator = ":" | "..." | "::" | "?" | "+" | "-" | "*" | "/" | "%" | "^"
		| "xor" | "&" | "bitand" | "|" | "bitor" | "~" | "compl" | "!" | "not"
		| "=" | "<" | ">" | "+=" | "-=" | "%=" | "^=" | "xor_eq"
		| "&=" | "and_eq" | "|=" | "or_eq" | "<<" | ">>" | ">>=" | "<<="
		| "!=" | "not_eq" | "<=" | ">=" | "&&" | "and" | "||" | "or" | "++"
		| "--" | "." | "->";
	operator     { return OPERATOR; }

	"struct"     { return STRUCT; }
	"union"      { return UNION; } // +fn are these all that can be braced?
	"typedef"    { return TYPEDEF; }
	("{" | "<%") { s->indent_level++; return LBRACE; }
	("}" | "%>") { s->indent_level--; return RBRACE; }
	("[" | "<:") { return LBRACK; }
	("]" | ":>") { return RBRACK; }
	"("          { return LPAREN; }
	")"          { return RPAREN; }
	","          { return COMMA; }
	";"          { return SEMI; }
*/
}

/** C style comments. Actively ignore.
 @implements ScannerFn
 @allow */
static enum Token scan_comment(struct Scanner *const s) {
	assert(s && state_look(s) == COMMENT);
	printf("Comment Line%lu.\n", s->line);
comment:
/*!re2c
	"\x00" { if(s->limit - s->cursor <= YYMAXFILL) return END; goto comment; }
	* { goto comment; }
	"\\". { goto comment; }
	newline { s->line++; goto comment; }
	"*""/" { return pop_call(s); }
*/
}

/** String constant. It reports one string constant for every string, even if
 they are actually concatenated. This is hard because one could have, eg, LINE
 and FILE pre-processor commands. Keep it simple.
 @implements ScannerFn
 @allow */
static enum Token scan_string(struct Scanner *const s) {
	assert(s && state_look(s) == STRING);
string:
/*!re2c
	"\x00" { if(s->limit - s->cursor <= YYMAXFILL) return END; goto string; }
	* { goto string; }
	"\"" { return pop(s) ? CONSTANT : END; }
	"\\" newline { s->line++; goto string; } // Continuation.
	"\\". { goto string; } // All additional chars are not escaped.
	newline { s->line++; return END; }
*/
}

/** Character constant.
 @implements ScannerFn
 @allow */
static enum Token scan_char(struct Scanner *const s) {
	assert(s && state_look(s) == CHAR);
character:
/*!re2c
	"\x00" { if(s->limit - s->cursor <= YYMAXFILL) return END; goto character; }
	* { goto character; }
	"'" { return pop(s) ? CONSTANT : END; }
	"\\" newline { s->line++; goto character; } /* Continuation. */
	"\\" . { goto character; } /* All additional chars are not escaped. */
	newline { s->line++; return END; }
*/
}

/** Actively ignore macros.
 @implements ScannerFn
 @allow */
static enum Token scan_macro(struct Scanner *const s) {
	assert(s && state_look(s) == MACRO);
macro:
/*!re2c
	"\x00" { if(s->limit - s->cursor <= YYMAXFILL) return END; goto macro; }
	* { goto macro; }
	doc / [^/] { return push_call(s, DOC); }
	comment { return push_call(s, COMMENT); }
	whitespace+ | cxx_comment { goto macro; }
	"\\" newline { s->line++; goto macro; } /* Continuation. */
	newline { s->line++; return pop_call(s); }
*/
}



/* Parser. */

/** Parser is only valid when Scanner is active and in steady-state. */
struct Symbol {
	enum Token token;
	const char *from, *to;
};

static void symbol_to_string(const struct Symbol *s, char (*const a)[12]) {
	int len = (int)(s->to - s->from);
	if(len > 5) len = 5;
	sprintf(*a, "%.4s\"%.*s\"", tokens[s->token], len, s->from);
	/*strncpy(*a, tokens[s->token], sizeof *a - 1);*/
	*a[sizeof *a - 1] = '\0';
}

#define ARRAY_NAME Symbol
#define ARRAY_TYPE struct Symbol
#define ARRAY_TO_STRING &symbol_to_string
#include "../src/Array.h"

struct Segment {
	struct SymbolArray doc, code;
	enum Type type;
};

/* A {SegmentArray} is the top level parser. */
#define ARRAY_NAME Segment
#define ARRAY_TYPE struct Segment
#include "../src/Array.h"

static void DeleteAllSegments(struct SegmentArray *const sa) {
	struct Segment *s;
	if(!sa) return;
	while((s = SegmentArrayPop(sa)))
		SymbolArray_(&s->doc), SymbolArray_(&s->code);
}

static struct Segment *NewSegment(struct SegmentArray *const sa) {
	struct Segment *s;
	assert(sa);
	if(!(s = SegmentArrayNew(sa))) return 0;
	SymbolArray_(&s->doc);
	SymbolArray_(&s->code);
	s->type = HEADER; /* Default. */
	return s;
}

/* Create a new symbol for this segment. */
static int PushSymbol(struct Segment *const s, const enum State state,
	const enum Token token, const struct Scanner *const scan) {
	struct Symbol *symbol;
	assert(s && (state == CODE || state == DOC) && scan);
	if(!(symbol = SymbolArrayNew(state == DOC ? &s->doc : &s->code))) return 0;
	symbol->token = token;
	symbol->from = scan->token;
	symbol->to = scan->cursor;
	return 1;
}



static void stripn(struct SymbolArray *const syms, const enum Token t,
	size_t a, size_t b) {
	struct Symbol *s;
	assert(syms && a <= b && b <= SymbolArraySize(syms));
	while(a < b && (s = SymbolArrayGet(syms, b - 1))->token == t)
		SymbolArrayRemove(syms, s), b--;
	if(a >= b) return; /* It was all {t}. */
	while(a < b && (s = SymbolArrayGet(syms, a))->token == t)
		SymbolArrayRemove(syms, s), b--;
	assert(a < b); /* Or else it would have already returned. */
}

static void strip(struct SymbolArray *const syms, const enum Token t) {
	assert(syms);
	stripn(syms, t, 0, SymbolArraySize(syms));
}

int main(int argc, char **argv) {
	struct Scanner scan;
	enum Token t = END;
	enum State state;
	struct SegmentArray text;
	struct Segment *segment = 0;
	struct Symbol *symbol;
	int is_done = 0;

	/* https://stackoverflow.com/questions/10293387/piping-into-application-run-under-xcode/13658537 */
	if (argc == 2 && strcmp(argv[1], "debug") == 0 ) {
		const char *test_file_path = "/Users/neil/Movies/Cdoc/c.txt";
		printf("== [RUNNING IN DEBUG MODE with %s]==\n\n", test_file_path);
		freopen(test_file_path, "r", stdin);
	}

	SegmentArray(&text);
	do { /* Try. */
		int is_indent = 0, is_struct = 0, is_line = 0;
		struct Segment *last_segment;

		/* Lex. */

		if(!Scanner(&scan)) break; /* First. */
		while((t = ScannerScan(&scan))) {
			int indent; /* Debug. */
			state = state_look(&scan);
			printf("%lu%c\t", (unsigned long)scan.line,
				state == DOC ? '~' : ':');
			for(indent = 0; indent < scan.indent_level; indent++)
				fputc('\t', stdout);
			printf("%s %s \"%.*s\"\n", StateArrayToString(&scan.states),
				tokens[t], (int)(scan.cursor - scan.token), scan.token);
			if(!is_indent) {
				if(scan.indent_level) { /* Entering a code block. */
					assert(scan.indent_level == 1 && state == CODE
						&& t == LBRACE);
					is_indent = 1;
					/* Determine if this is function. */
					if((symbol = SymbolArrayPop(&segment->code))
						&& symbol->token != RPAREN) {
						if(segment) segment->type = DECLARATION;
						is_struct = 1;
					} else if(segment) segment->type = FUNCTION;
				} else if(t == SEMI) { /* Semicolons on indent level 0. */
					/* General declaration? */
					if(segment && segment->type == HEADER)
						segment->type = DECLARATION;
					is_line = 1;
				} else if(segment && SymbolArraySize(&segment->doc)
					&& !SymbolArraySize(&segment->code)
					&& (t == BEGIN_DOC
					|| (state != DOC && scan.doc_line + 2 < scan.line))) {
					/* Hasn't scanned any code and is on the top level, cut
					 multiple docs and the doc has to be within a reasonable
					 distance. */
					printf("<cut>\n"), segment = 0;
				}
			} else { /* {is_indent}. */
				if(!scan.indent_level) { /* Exiting to indent level 0. */
					is_indent = 0, assert(t == RBRACE);
					if(!is_struct) is_line = 1; /* Functions (fixme: sure?) */
				} else if(!is_struct && state == CODE) {
					continue; /* Code in functions: don't care. */
				}
			}
			if(t == BEGIN_DOC) continue;
			/* Create new segment if need be. */
			if(!segment) {
				printf("<new segment>\n");
				if(!(segment = NewSegment(&text))) break;
			}
			if(!PushSymbol(segment, state, t, &scan)) break;
			/* Create another segment next time. */
			if(is_line) is_line = 0, is_struct = 0, segment = 0;
		}
		if(t) break;
		if(scan.indent_level) { errno = EILSEQ; break; }

		/* Cull. */

		last_segment = segment = 0;
		while((segment = SegmentArrayNext(&text, segment))) {
			/* Remove any segments that we don't need. */
			if(segment->type != FUNCTION && !SymbolArraySize(&segment->doc)) {
				printf("Segment %s.\n", SymbolArrayToString(&segment->code));
				SegmentArrayRemove(&text, segment);
				printf("REMOVED!!\n");
				segment = last_segment;
				printf("Now %s.\n",
					segment ? SymbolArrayToString(&segment->code) : "head");
				continue;
			}
			last_segment = segment;
			switch(segment->type) {
			case HEADER:
			case DECLARATION:
			case FUNCTION:
				break;
			}
			/* This happens after so that if the entire comment is a dud, it
			 still gets included. */
			strip(&segment->doc, WHITESPACE);
			/* fixme: Strip recusively along {}. */
		}
		is_done = 1;
	} while(0); if(!is_done) {
		perror("Cdoc");
		fprintf(stderr, "At %lu%c indent level %d; state stack %s; %s \"%.*s\"."
			"\n", (unsigned long)scan.line, state == DOC ? '~' : ':',
			scan.indent_level, StateArrayToString(&scan.states),
			tokens[t], (int)(scan.cursor - scan.token), scan.token);
	} else {
		fputs("\n\n*****\n\n", stdout);
		segment = 0;
		while((segment = SegmentArrayNext(&text, segment))) {
			printf("Segment(%s):\n\tdoc: %s.\n\tcode: %s.\n\n",
				types[segment->type],
				SymbolArrayToString(&segment->doc),
				SymbolArrayToString(&segment->code));
		}
		fputc('\n', stdout);
	} {
		DeleteAllSegments(&text), SegmentArray_(&text);
		Scanner_(&scan);
	}
	return is_done ? EXIT_SUCCESS : EXIT_FAILURE;
}
