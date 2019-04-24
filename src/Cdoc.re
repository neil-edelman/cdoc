/** 2019 Neil Edelman, distributed under the terms of the MIT License,
 \url{ https://opensource.org/licenses/MIT }.

 {Doxygen} is great and standard; one should use that if possible. However,
 maybe one is too hip to use it? or it uses libraries that are too new for your
 computer?

 Parses and extracts the documentation commands in a {.c} file. A documentation
 command begins with {/\**}.

 @title Cdoc.c
 @author Neil
 @version 2019-05 Re-done in {re2c}.
 @since 2017-03 Initial version.
 @std C89
 @depend re2c (\url{http://re2c.org/})
 @fixme Lists in comments, etc.
 @fixme Support Kernel-style comments where the " * " starts a line.
 @fixme {void A_BI_(Create, Thing)(void)} -> {<A>Create<BI>Thing(void)}. */

#include <stdlib.h> /* EXIT malloc free */
#include <stdio.h>  /* FILE printf fputc perror */
#include <string.h> /* memmove memset */
#include <errno.h>  /* EDOM ERANGE */
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
	X(ESCAPED_BACKSLASH, 0), X(ESCAPED_LBRACE, 0), X(ESCAPED_RBRACE, 0), \
	X(ESCAPED_EACH, 0), X(WHITESPACE, 0), X(NEWLINE, 0), \
	X(BS_URL, 0), X(BS_CITE, 0), X(BS_SEE, 0), X(BS_PRE, 0), \
	X(TAG_TITLE, 0), X(TAG_PARAM, 0), X(TAG_AUTHOR, 0), X(TAG_STD, 0), X(TAG_DEPEND, 0), \
	X(TAG_VERSION, 0), X(TAG_SINCE, 0), X(TAG_FIXME, 0), X(TAG_DEPRICATED, 0), \
	X(TAG_RETURN, 0), X(TAG_THROWS, 0), X(TAG_IMPLEMENTS, 0), X(TAG_ORDER, 0), \
	X(TAG_ALLOW, 0), X(HTML_AMP, 0), X(HTML_LT, 0), X(HTML_GT, 0)
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

/* Define {StateArray}, a stack of states. */
#define ARRAY_NAME State
#define ARRAY_TYPE enum State
#define ARRAY_STACK
#include "../src/Array.h"

/** Scanner reads a file and puts it in memory. */
struct Scanner {
	struct CharArray buffer;
	struct StateArray states;
	int indent_level;
	size_t line;
	/* {re2c} variables. These point directly into {buffer} so no modifying. */
	const char *limit, *cursor, *marker, *token;
};

/** Fill the structure with default values. */
static void zero(struct Scanner *const s) {
	assert(s);
	CharArray(&s->buffer);
	StateArray(&s->states);
	s->indent_level = 0;
	s->line = 0;
	s->limit = s->cursor = s->marker = s->token = 0;
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
 @allow */
static int Scanner(struct Scanner *const s, const char *const fn) {
	const size_t granularity = 80;
	FILE *fp = 0;
	int is_done = 0;
	assert(s && fn);
	zero(s);
	do { /* Try: read all contents at once. */
		char *a;
		enum State *ps;
		if(!(fp = fopen(fn, "r"))) break;
		for( ; ; ) {
			if(!(a = CharArrayBuffer(&s->buffer, granularity))) break;
			if(!fgets(a, (int)granularity, fp))
				{ is_done = !ferror(fp); break; }
			if(!CharArrayAddSize(&s->buffer, strlen(a))) break;
		}
		if(!is_done) break;
		/* Fill the past the file with '\0' for fast lexing. */
		is_done = 0;
		if(!(a = CharArrayBuffer(&s->buffer, YYMAXFILL))) break;
		memset(a, '\0', YYMAXFILL);
		if(!CharArrayAddSize(&s->buffer, YYMAXFILL)) break;
		/* Point these toward the first char; {s->buffer} is necessarily done
		 growing, or we could not do this. */
		s->cursor = s->marker = s->token = CharArrayNext(&s->buffer, 0);
		s->line = 1;
		if(!(ps = StateArrayNew(&s->states))) break;
		*ps = CODE;
		is_done = 1;
	} while(0); { /* Finally. */
		if(fp) { if(fclose(fp) == EOF) is_done = 0; fp = 0; }
	} if(!is_done) { /* Catch. */
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
static enum Token state_look(struct Scanner *const s) {
	enum State *ps;
	assert(s);
	if(!(ps = StateArrayPeek(&s->states))) return END;
	return *ps;
}

/** Pushes the new state and calls the state function.
 @return What the state function returns. END if there's an error. */
static enum Token push(struct Scanner *const s, const enum State state) {
	enum State *ps;
	assert(s);
	if(!(ps = StateArrayNew(&s->states))) return END;
	*ps = state;
	return state_fn[state](s);
}

/** Pops and calls the state on top.
 @return What the state function returns. END if there's an error or the state
 is empty. */
static enum Token pop(struct Scanner *const s) {
	enum State *ps;
	assert(s);
	if(!StateArrayPop(&s->states) || !(ps = StateArrayPeek(&s->states)))
		return END;
	return state_fn[*ps](s);
}



/* {ScannerFn}'s. */

/*!re2c
re2c:define:YYCTYPE  = char;
re2c:define:YYFILL   = "return END;";
re2c:define:YYFILL:naked = 1;
re2c:define:YYLIMIT  = s->limit;
re2c:define:YYCURSOR = s->cursor;
re2c:define:YYMARKER = s->marker; /* Rules overlap. */
/* Don't know what this does, but it fails without it. */
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
doc:
	s->token = s->cursor;
/*!re2c
	"\x00" { if(s->limit - s->cursor <= YYMAXFILL) return END; goto doc; }
	"*/" { return pop(s); }
	* { goto doc; }

	whitespace = [ \t\v\f];
	whitespace+ { return WHITESPACE; }

	newline = "\n" | "\r" "\n"?;
	newline { s->line++; return NEWLINE; }

	word = [^ \t\n\v\f\r\\,@{}&<>*]*; /* This is kind of sketchy. */
	word { return ID; }

	"\\\\" { return ESCAPED_BACKSLASH; }
	"\\{" { return ESCAPED_LBRACE; }
	"\\}" { return ESCAPED_RBRACE; }
	"\\@" { return ESCAPED_EACH; }
	"{" { return LBRACE; }
	"}" { return RBRACE; }
	"," { return COMMA; }

	/* These are recognised in the documentation as stuff. */
	"\\url" { return BS_URL; }
	"\\cite" { return BS_CITE; }
	"\\see" { return BS_SEE; }
	"\\$" { return BS_PRE; }

	/* These are tags. */
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

	/* Also escape these for {HTML}. */
	"&" { return HTML_AMP; }
	"<" { return HTML_LT; }
	">" { return HTML_GT; }
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
	/* http://re2c.org/examples/example_03.html */
	"\x00" { if(s->limit - s->cursor <= YYMAXFILL) return END; goto code; }
	* { printf("Unknown, <%.*s>, just roll with it.\n",
		(int)(s->cursor - s->token), s->token); goto code; }

	newline { s->line++; goto code; }

	/* Documentation is not '/ * * /', but other then that. */
	doc = "/""**";
	doc / [^/] { return push(s, DOC); }

	comment = "/""*";
	comment { return push(s, COMMENT); }
	cxx_comment = "//" [^\n]*;
	whitespace+ | cxx_comment { goto code; }

	macro = ("#" | "%:");
	macro { return push(s, MACRO); }

	/* Number separate from minus then numbers can't have whitespace, simple. */
	oct = "0" [0-7]*;
	dec = [1-9][0-9]*;
	hex = '0x' [0-9a-fA-F]+;
	frc = [0-9]* "." [0-9]+ | [0-9]+ ".";
	exp = 'e' [+-]? [0-9]+;
	flt = (frc exp? | [0-9]+ exp) [fFlL]?;
	number = (oct | dec | hex | flt) ("u"|"l"|"ul"|"lu")?;
	number { return CONSTANT; }

	/* Strings are more complicated because they can have whitespace.
	 @fixme No trigraph support. */
	/* char_type = "u8"|"u"|"U"|"L"; <- These get caught in id; don't care. */
	"\"" { return push(s, STRING); }
	"'" { return push(s, CHAR); }

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
	"union"      { return UNION; } /* +fn are these all that can be braced? */
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
comment:
/*!re2c
	"\x00" { if(s->limit - s->cursor <= YYMAXFILL) return END; goto comment; }
	* { goto comment; }
	"\\". { goto comment; }
	newline { s->line++; goto comment; }
	"*""/" { return pop(s); }
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
	"\"" {
		/* We can't just pop, we have to return {CONSTANT}. */
		if(!StateArrayPop(&s->states)) return END;
		return CONSTANT;
	}
	"\\" newline { s->line++; goto string; } /* Continuation. */
	"\\" . { goto string; } /* All additional chars are not escaped. */
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
	"'" {
		/* We can't just pop, we have to return {CONSTANT}. */
		if(!StateArrayPop(&s->states)) return END;
		return CONSTANT;
	}
	"\\" newline { s->line++; goto character; } /* Continuation. */
	"\\" . { goto character; } /* All additional chars are not escaped. */
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
	doc / [^/] { return push(s, DOC); }
	comment { return push(s, COMMENT); }
	whitespace+ | cxx_comment { goto macro; }
	"\\" newline { s->line++; goto macro; } /* Continuation. */
	newline { s->line++; return pop(s); }
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

#define ARRAY_NAME Segment
#define ARRAY_TYPE struct Segment
#include "../src/Array.h"

int main(void) {
	const char *const fn = "x.c";
	struct Scanner scan;
	enum Token t;
	int is_done = 0;
	struct SegmentArray text;
	struct Segment *segment = 0;
	struct Symbol *symbol;
	enum State *state;
	int is_indent = 0, is_struct = 0, is_line = 0;
	SegmentArray(&text);
	do {
		if(!Scanner(&scan, fn)) break;
		while((t = ScannerScan(&scan))) {
			int i;
			state = StateArrayPeek(&scan.states);
			assert(state);
			printf("%lu%c\t", (unsigned long)scan.line,
				*state == DOC ? '~' : ':');
			for(i = -1; i < scan.indent_level; i++) fputc('\t', stdout);
			printf("%s \"%.*s\"\n", tokens[t],
				(int)(scan.cursor - scan.token), scan.token);
			if(!is_indent) {
				if(scan.indent_level == 1) { /* Entering a code block. */
					assert(*state == CODE && t == LBRACE);
					is_indent = 1;
					/* Determine if this is function. */
					if((symbol = SymbolArrayPop(&segment->code))
						&& symbol->token != RPAREN) {
						if(segment) segment->type = DECLARATION;
						is_struct = 1;
					} else if(segment) segment->type = FUNCTION;
				} else if(t == SEMI) { /* Semicolons on indent level 0. */
					is_line = 1;
				}
			} else { /* {is_indent}. */
				if(!scan.indent_level) { /* Exiting to indent level 0. */
					is_indent = 0, assert(t == RBRACE);
					if(!is_struct) is_line = 1; /* Functions (fixme: sure?) */
				} else if(!is_struct && *state == CODE) {
					continue; /* Code in functions: don't care. */
				}
			}
			/* Create new segment if need be. */
			if(!segment) {
				if(!(segment = SegmentArrayNew(&text))) break;
				SymbolArray(&segment->doc);
				SymbolArray(&segment->code);
				segment->type = HEADER; /* Default. */
			}
			/* Create a new symbol for this segment. */
			if(!(symbol = SymbolArrayNew(*state == DOC
				? &segment->doc : &segment->code))) break;
			symbol->token = t;
			symbol->from = scan.token;
			symbol->to = scan.cursor;
			/* @fixme
			 Different doc comments should definitely be paragraphed. */
			/* Create another segment next time. */
			if(is_line) {
				/* General declaration. */
				if(t == SEMI && segment->type == HEADER)
					segment->type = DECLARATION;
				is_line = 0, is_struct = 0, segment = 0;
			}
		}
		if(t) break;
		is_done = 1;
	} while(0); if(!is_done) {
		perror(fn);
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
		while((segment = SegmentArrayPop(&text)))
			SymbolArray_(&segment->doc), SymbolArray_(&segment->code);
		SegmentArray_(&text);
		Scanner_(&scan);
	}
	return is_done ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* Also provide these for convenience: common compsci math and Greek.
{ "\\dot",   0,   &html_dot },
{ "\\lceil", 0,   &html_lceil },
{ "\\rceil", 0,   &html_rceil },
{ "\\lfloor",0,   &html_lfloor },
{ "\\rfloor",0,   &html_rfloor },
{ "\\to",    0,   &html_to },
{ "\\ge",    0,   &html_ge },
{ "\\le",    0,   &html_le },
{ "\\ne",    0,   &html_ne },
{ "\\cap",   0,   &html_cap },
{ "\\cup",   0,   &html_cup },
{ "\\vee",   0,   &html_vee },
{ "\\wedge", 0,   &html_wedge },
{ "\\sum",   0,   &html_sum },
{ "\\prod",  0,   &html_prod },
{ "\\in",    0,   &html_in },
{ "\\exists",0,   &html_exists },
{ "\\forall",0,   &html_forall },
{ "\\neg",   0,   &html_neg },
{ "\\times", 0,   &html_times },
{ "\\sqrt",  0,   &html_sqrt },
{ "\\propto",0,   &html_propto },
{ "\\pm",    0,   &html_pm },
{ "\\partial",0,  &html_partial },
{ "\\int",   0,   &html_int },
{ "\\infty", 0,   &html_infty },
{ "\\Gamma", 0,   &html_Gamma },
{ "\\Delta", 0,   &html_Delta },
{ "\\Lambda",0,   &html_Lambda },
{ "\\Phi",   0,   &html_Phi },
{ "\\Pi",    0,   &html_Pi },
{ "\\Psi",   0,   &html_Psi },
{ "\\Sigma", 0,   &html_Sigma },
{ "\\Theta", 0,   &html_Theta },
{ "\\Upsilon",0,  &html_Upsilon },
{ "\\Xi",    0,   &html_Xi },
{ "\\Omega", 0,   &html_Omega },
{ "\\alpha", 0,   &html_alpha },
{ "\\beta",  0,   &html_beta },
{ "\\gamma", 0,   &html_gamma },
{ "\\delta", 0,   &html_delta },
{ "\\epsilon",0,  &html_epsilon },
{ "\\zeta",  0,   &html_zeta },
{ "\\eta",   0,   &html_eta },
{ "\\theta", 0,   &html_theta },
{ "\\iota",  0,   &html_iota },
{ "\\kappa", 0,   &html_kappa },
{ "\\lambda",0,   &html_lamda },
{ "\\mu",    0,   &html_mu },
{ "\\nu",    0,   &html_nu },
{ "\\xi",    0,   &html_xi },
{ "\\rho",   0,   &html_rho },
{ "\\sigma", 0,   &html_sigma },
{ "\\tau",   0,   &html_tau },
{ "\\upsilon",0,  &html_upsilon },
{ "\\phi",   0,   &html_phi },
{ "\\chi",   0,   &html_chi },
{ "\\psi",   0,   &html_psi },
{ "\\omega", 0,   &html_omega }
*/
