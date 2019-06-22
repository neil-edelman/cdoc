/** 2019 Neil Edelman, distributed under the terms of the MIT License,
 \url{ https://opensource.org/licenses/MIT }.

 This is a context-sensitive parser. Lexes C documents on-line and (partially)
 parses them to give the state, (comments, code, strings, character literals,
 documentation,) the indent level, etc. (Imperfectly, but good enough?)

 @title Scanner.re
 @author Neil
 @version 2019-06
 @std C89
 @depend re2c (\url{http://re2c.org/})
 @fixme Different doc comments need new paragraphs.
 @fixme Lists in comments, etc.
 @fixme {void A_BI_(Create, Thing)(void)} -> {<A>Create<BI>Thing(void)}.
 @fixme Trigraph support, (haha.)
 @fixme Old-style function support. */

#include <stdio.h>  /* printf */
#include <string.h> /* memset */
/* #define NDEBUG */
#include <assert.h> /* assert */
#include <limits.h> /* INT_MAX */

#include "../src/Scanner.h"

/** Private state information. Forward-declared for {state_fn}. From
 `Scanner.h`. */
typedef enum Symbol (*ScannerFn)(void);
static enum Symbol scan_eof(void);
static enum Symbol scan_doc(void);
static enum Symbol scan_code(void);
static enum Symbol scan_comment(void);
static enum Symbol scan_string(void);
static enum Symbol scan_char(void);
static enum Symbol scan_macro(void);
static const char *const states[] = { STATE(STRINGISE2_A) };
static const ScannerFn state_fn[] = { STATE(PARAM2_B) };

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

/* Define {CharArray}, a vector of characters -- dynamic string. */
#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "../src/Array.h"



/* Define QUOTE. */
#ifdef QUOTE
#undef QUOTE
#endif
#ifdef QUOTE_
#undef QUOTE_
#endif
#define QUOTE_(name) #name
#define QUOTE(name) QUOTE_(name)

/** Scanner reads a file and extracts semantic information. */
struct Scanner {
	/* `buffer` {re2c} variables. These point directly into {buffer} so no
	 modifying. */
	const char *limit, *cursor, *marker, *ctx_marker, *from;
	/* Weird {c2re} stuff: these fields have to come after when >5? */
	struct CharArray buffer, marks;
	struct StateArray states;
	enum Symbol symbol;
	int indent_level;
	int ignore_block;
	size_t line, doc_line;
} scanner;

/** Unloads Scanner from memory. */
void Scanner_(void) {
	CharArray_(&scanner.buffer);
	CharArray_(&scanner.marks);
	StateArray_(&scanner.states);
	scanner.symbol = END;
	scanner.indent_level = 0;
	scanner.ignore_block = 0;
	scanner.line = scanner.doc_line = 0;
	scanner.limit = scanner.cursor = scanner.marker = scanner.ctx_marker
		= scanner.from = 0;
}

/* Have {re2c} generate {YYMAXFILL}.
 \url{ http://re2c.org/examples/example_02.html }. */
/*!max:re2c*/

/** New Scanner. Reads from `stdin` until done, (it doesn't make sense to
 call this twice.)
 @return Success.
 @throws malloc free fread */
int Scanner(void) {
	const size_t granularity = 1024;
	int is_done = 0;
	do { /* Try: read all contents from `stdin` at once. */
		size_t nread;
		char *buf;
		enum State *ps;
		/* This assumes that the `scanner` is zeroed, thus in a valid state. */
		for( ; ; ) { /* Read in `granularity` sized chunks. */
			if(!(buf = CharArrayBuffer(&scanner.buffer, granularity))) break;
			nread = fread(buf, 1, granularity, stdin);
			if(ferror(stdin)) break;
			assert(nread >= 0 && nread <= granularity);
			if(!CharArrayAddSize(&scanner.buffer, nread)) break;
			if(nread != granularity) { is_done = 1; break; }
		}
		if(!is_done) break;
		/* Fill the past the file with '\0' for fast lexing. */
		is_done = 0;
		if(!(buf = CharArrayBuffer(&scanner.buffer, YYMAXFILL))) break;
		memset(buf, '\0', YYMAXFILL);
		if(!CharArrayAddSize(&scanner.buffer, YYMAXFILL)) break;
		/* Point these toward the first char; `buffer` is necessarily done
		 growing, or we could not do this. `limit` and `ctx_marker` are set? */
		scanner.cursor = scanner.marker = scanner.from
			= CharArrayGet(&scanner.buffer);
		scanner.line = scanner.doc_line = 1;
		if(!(ps = StateArrayNew(&scanner.states))) break;
		*ps = CODE;
		is_done = 1;
	} while(0); if(!is_done) { /* Catch. */
		Scanner_();
		return 0;
	}
	return 1;
}

/* Needed in `debug`. */
static enum State state_look(void);

static void debug(void) {
	int indent;
	printf("%lu%c\t", (unsigned long)scanner.line,
		state_look() == DOC ? '~' : ':');
	for(indent = 0; indent < scanner.indent_level; indent++)
		fputc('\t', stdout);
	printf("%s %s \"%.*s\", ignore %d, indent %d\n",
		StateArrayToString(&scanner.states),
		symbols[scanner.symbol], (int)(scanner.cursor - scanner.from),
		scanner.from, scanner.ignore_block, scanner.indent_level);
}

/** Lexes the next token. This will update `ScannerToken` and
 `ScannerTokenInfo`.
 @return If the scanner had more tokens.
 @fixme Is it really neccesary to have docs inside of functions? This would be
 so much easier logic without. */
int ScannerNext(void) {
	enum State *state;
	/* Ignore a block of code if `scanner.ignore_block` is on. */
	do {
		if(!(state = StateArrayPeek(&scanner.states))
			|| !(scanner.symbol = state_fn[*state]())) return 0;
		debug();
		if(*state != CODE) return 1; /* Return not-code in `ignore_block`. */
	} while(scanner.ignore_block && scanner.indent_level);
	scanner.ignore_block = 0;
	return 1;
}

/** We don't care what's in the function, just that it's a function. */
void ScannerIgnoreBlock(void) { scanner.ignore_block = 1; }

/** Fills `token` with the last token.
 @param{token} If null, does nothing. */
void ScannerToken(struct Token *const token) {
	if(!token) return;
	assert(scanner.symbol && scanner.from && scanner.from <= scanner.cursor);
	token->symbol = scanner.symbol;
	token->from = scanner.from;
	if(scanner.from + INT_MAX < scanner.cursor) {
		fprintf(stderr, "Length of string chopped to " QUOTE(INT_MAX) ".\n");
		token->length = INT_MAX;
	} else {
		token->length = (int)(scanner.cursor - scanner.from);
	}
	token->line = scanner.line;
}

/** Fills `info` with the last token information not stored in the token. */
void ScannerTokenInfo(struct TokenInfo *const info) {
	if(!info) return;
	info->indent_level = scanner.indent_level;
	info->is_doc = state_look() == DOC;
	info->is_doc_far = scanner.doc_line + 2 < scanner.line;
}



/* Scanner helper functions. */



/** @return The state of {s}. */
static enum State state_look(void) {
	enum State *ps;
	if(!(ps = StateArrayPeek(&scanner.states))) return END_OF_FILE;
	return *ps;
}

/** Pushes the new state.
 @return Success. */
static int push(const enum State state) {
	enum State *ps;
	if(!(ps = StateArrayNew(&scanner.states))) return 0;
	*ps = state;
	return 1;
}

/** Pushes the new state and calls the state function.
 @return What the state function returns. END if there's an error. */
static enum Symbol push_call(const enum State state) {
	if(!(push(state))) return END;
	return state_fn[state]();
}

/** Pops and calls the state on top.
 @return Success. */
static int pop(void) {
	return !!StateArrayPop(&scanner.states);
}

/** Pops and calls the state on top and calls the state underneath.
 @return What the state function returns. END if there's an error or the state
 is empty. */
static enum Symbol pop_call(void) {
	enum State *ps;
	if(!pop() || !(ps = StateArrayPeek(&scanner.states))) return END;
	return state_fn[*ps]();
}



/* {ScannerFn}'s. */



/*!re2c
re2c:define:YYCTYPE  = char;
re2c:define:YYFILL   = "return END;";
re2c:define:YYFILL:naked = 1;
re2c:define:YYLIMIT  = scanner.limit;
re2c:define:YYCURSOR = scanner.cursor;
re2c:define:YYMARKER = scanner.marker; // Rules overlap.
// Might or might not require this (depending on the version?)
re2c:define:YYCTXMARKER = scanner.ctx_marker;
// Don't know what this does, but it fails without it.
re2c:yyfill:enable = 0;
*/

/** Returns eof.
 @implements ScannerFn */
static enum Symbol scan_eof(void) { return END; }

/** Scans documentation.
 @implements ScannerFn */
static enum Symbol scan_doc(void) {
	assert(state_look() == DOC);
	scanner.doc_line = scanner.line;
doc:
	scanner.from = scanner.cursor;
/*!re2c
	"\x00" { if(scanner.limit - scanner.cursor <= YYMAXFILL) return END;
		goto doc; }
	"*""/" { return pop_call(); }
	* { goto doc; }

	whitespace = [ \t\v\f];
	whitespace+ { goto doc; }

	newline = "\n" | "\r" "\n"?;
	art = "*"? newline " *";
	newline { scanner.line++; return NEWLINE; }
	art / [^/] { scanner.line++; return NEWLINE; }

	word = [^ \t\n\v\f\r\\,@{}&<>*]*; // This is kind of sketchy.
	word { return WORD; }

	"\\\\" { return ESCAPED_BACKSLASH; }
	"\`" { return ESCAPED_BACKQUOTE; }
	"\\@" { return ESCAPED_EACH; }
	"`" { return BACKQUOTE; }
	"{" { return DOC_LBRACE; }
	"}" { return DOC_RBRACE; }
	"," { return DOC_COMMA; }

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
 @implements ScannerFn */
static enum Symbol scan_code(void) {
	assert(state_look() == CODE);
code:
	scanner.from = scanner.cursor;
/*!re2c
	// http://re2c.org/examples/example_03.html
	"\x00" { if(scanner.limit - scanner.cursor <= YYMAXFILL) return END;
		goto code; }
	* { printf("Unknown, <%.*s>, just roll with it.\n",
		(int)(scanner.cursor - scanner.from), scanner.from); goto code; }

	newline { scanner.line++; goto code; }

	// Documentation is not / * * /, but other then that.
	doc = "/""**";
	doc / [^/] { return push(DOC) ? BEGIN_DOC : END; }

	comment = "/""*";
	comment { return push_call(COMMENT); }
	cxx_comment = "//" [^\n]*;
	whitespace+ | cxx_comment { goto code; }

	macro = ("#" | "%:");
	macro { return push_call(MACRO); }

	// Strings are more complicated because they can have whitespace and that
	// messes up the line count.
	// @fixme No trigraph support.
	// char_type = "u8"|"u"|"U"|"L"; <- These get caught in id; don't care.
	"L"? "\"" { return push_call(STRING); }
	"'" { return push_call(CHAR); }

	// Number separate from minus then numbers can't have whitespace, simple.
	oct = "0" [0-7]*;
	dec = [1-9][0-9]*;
	hex = '0x' [0-9a-fA-F]+;
	frc = [0-9]* "." [0-9]+ | [0-9]+ ".";
	exp = 'e' [+-]? [0-9]+;
	flt = (frc exp? | [0-9]+ exp) [fFlL]?;
	number = (oct | dec | hex | flt) [uUlL]*;
	number { return CONSTANT; }

	// Extension (hack) for generic macros; if one names them this way, it will
	// be documented nicely; the down side is, these are legal names for
	// identifiers; will be confused if you name anything this way that IS an
	// identifier.
	generic = [A-Z]+ "_";
	generic { return ID_ONE_GENERIC; }
	generic generic { return ID_TWO_GENERICS; }
	generic generic generic { return ID_THREE_GENERICS; }

	operator = ":" | "..." | "::" | "?" | "+" | "-" | "*" | "/" | "%" | "^"
		| "xor" | "&" | "bitand" | "|" | "bitor" | "~" | "compl" | "!" | "not"
		| "=" | "<" | ">" | "+=" | "-=" | "%=" | "^=" | "xor_eq"
		| "&=" | "and_eq" | "|=" | "or_eq" | "<<" | ">>" | ">>=" | "<<="
		| "!=" | "not_eq" | "<=" | ">=" | "&&" | "and" | "||" | "or" | "++"
		| "--" | "." | "->";
	operator { return OPERATOR; }

	"struct"     { return STRUCT; }
	"union"      { return UNION; } // +fn are these all that can be braced?
	"enum"       { return ENUM; } // forgot one
	"typedef"    { return TYPEDEF; }
	"static"     { return STATIC; }
	("{" | "<%") { scanner.indent_level++; return LBRACE; }
	("}" | "%>") { scanner.indent_level--; return RBRACE; }
	("[" | "<:") { return LBRACK; }
	("]" | ":>") { return RBRACK; }
	"("          { return LPAREN; }
	")"          { return RPAREN; }
	","          { return COMMA; }
	";"          { return SEMI; }

	id = [a-zA-Z_][a-zA-Z_0-9]*;
	id { return ID; }
*/
}

/** C style comments. Actively ignore.
 @implements ScannerFn
 @allow */
static enum Symbol scan_comment(void) {
	assert(state_look() == COMMENT);
	printf("Comment Line%lu.\n", scanner.line);
comment:
/*!re2c
	"\x00" { if(scanner.limit - scanner.cursor <= YYMAXFILL) return END;
		goto comment; }
	* { goto comment; }
	"\\". { goto comment; }
	newline { scanner.line++; goto comment; }
	"*""/" { return pop_call(); }
*/
}

/** String constant. It reports one string constant for every string, even if
 they are actually concatenated. This is hard because one could have, eg, LINE
 and FILE pre-processor commands. Keep it simple.
 @implements ScannerFn
 @allow */
static enum Symbol scan_string(void) {
	assert(state_look() == STRING);
string:
/*!re2c
	"\x00" { if(scanner.limit - scanner.cursor <= YYMAXFILL) return END;
		goto string; }
	* { goto string; }
	"\"" { return pop() ? CONSTANT : END; }
	"\\" newline { scanner.line++; goto string; } // Continuation.
	"\\". { goto string; } // All additional chars are not escaped.
	newline { scanner.line++; return END; }
*/
}

/** Character constant.
 @implements ScannerFn
 @allow */
static enum Symbol scan_char(void) {
	assert(state_look() == CHAR);
character:
/*!re2c
	"\x00" { if(scanner.limit - scanner.cursor <= YYMAXFILL) return END;
		goto character; }
	* { goto character; }
	"'" { return pop() ? CONSTANT : END; }
	"\\" newline { scanner.line++; goto character; } /* Continuation. */
	"\\" . { goto character; } /* All additional chars are not escaped. */
	newline { scanner.line++; return END; }
*/
}

/** Actively ignore macros.
 @implements ScannerFn
 @allow */
static enum Symbol scan_macro(void) {
	assert(state_look() == MACRO);
macro:
/*!re2c
	"\x00" { if(scanner.limit - scanner.cursor <= YYMAXFILL) return END;
		goto macro; }
	* { goto macro; }
	doc / [^/] { return push_call(DOC); }
	comment { return push_call(COMMENT); }
	whitespace+ | cxx_comment { goto macro; }
	"\\" newline { scanner.line++; goto macro; } /* Continuation. */
	newline { scanner.line++; return pop_call(); }
*/
}
