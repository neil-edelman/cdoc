/** 2019 Neil Edelman, distributed under the terms of the MIT License,
 \url{ https://opensource.org/licenses/MIT }.

 Parses and extracts the documentation commands in a {.c} file. A documentation
 command begins with {\/\*\*} and is outside of any nested braces. {Doxygen} is
 great and standard; one should use that if possible.

 If you write {void A_BI_(Create, Thing)(void)} it will transform it to
 {<A>Create<BI>Thing(void)}.

 These are recognised in-line in the documentation:

 - \\url\{<link>\},
 - \\cite\{<paper>\},
 - \\see\{<function>\},
 - \\$\{<pre>\},
 - \{<emphasis>\}.

 These are tags that modify things past it:

 - \@title [<title>],
 - \@param[\{<param>[, <param>]*\}] <description>,
 - \@author [<you>],
 - \@std [<standard>],
 - \@depend [<files, libraries>],
 - \@version[\{<version>\}] [description],
 - \@since[\{<version>\}] [description],
 - \@fixme [<error>],
 - \@deprecated [<why>],
 - \@return [<value>],
 - \@throws[\{errno\}] [<exceptional return>],
 - \@implements [<fuction type>],
 - \@order [<O(g(n))>],
 - \@allow (Allows {static} functions.)

 @title Cdoc.c
 @author Neil
 @version 2019-05 Re-done in {re2c}.
 @since 2017-03 Initial version.
 @fixme Lists.
 @fixme Support Kernel-style.
 @fixme \@depend
*/

#include <stdlib.h> /* EXIT malloc free */
#include <stdio.h>  /* FILE printf fputc perror */
#include <string.h> /* memmove memset */
#include <assert.h> /* assert */

/* X-Marco. */
#define IDENT(A, F) A
#define STRINGISE(A, F) #A
#define FLAGS(A, F) F
#define TOKEN(X) \
	X(END, 0), X(THING, 0), X(LBRACE, 0), X(RBRACE, 0), \
	X(LPAREN, 0), X(RPAREN, 0), X(LBRACK, 0), X(RBRACK, 0), \
	X(CONSTANT, 0), X(ID, 0), X(COMMA, 0), X(SEMI, 0), X(COLON, 0), X(ELLIPSES, 0), \
	X(AMPER, 0), X(STAR, 0), X(DOT, 0), X(ARROW, 0), X(EQUALS, 0), X(COMPARISON, 0), X(STRUCT, 0), \
	X(TYPEDEF, 0), X(PREPROCESSOR, 0), \
	X(ESCAPED_BACKSLASH, 0), X(ESCAPED_LBRACE, 0), X(ESCAPED_RBRACE, 0), \
	X(ESCAPED_EACH, 0), X(WHITESPACE, 0), \
	X(BS_URL, 0), X(BS_CITE, 0), X(BS_SEE, 0), X(BS_PRE, 0), \
	X(TAG_TITLE, 0), X(TAG_PARAM, 0), X(TAG_AUTHOR, 0), X(TAG_STD, 0), X(TAG_DEPEND, 0), \
	X(TAG_VERSION, 0), X(TAG_SINCE, 0), X(TAG_FIXME, 0), X(TAG_DEPRICATED, 0), \
	X(TAG_RETURN, 0), X(TAG_THROWS, 0), X(TAG_IMPLEMENTS, 0), X(TAG_ORDER, 0), \
	X(TAG_ALLOW, 0), X(HTML_AMP, 0), X(HTML_LT, 0), X(HTML_GT, 0)
enum Token { TOKEN(IDENT) };
static const char *const tokens[] = { TOKEN(STRINGISE) };
static const int token_flags[] = { TOKEN(FLAGS) };

/*!max:re2c*/
#define SIZE (64 * 1024)

struct Scanner {
	char buffer[SIZE + YYMAXFILL], *limit, *cursor, *marker, *token, *line;
	int is_eof, is_doc;
	int indent_level;
	const char *fn;
    FILE *fp;
};

/** Fill the structure with default values. */
static void Scanner_zero(struct Scanner *const s) {
	assert(s);
	s->limit = s->cursor = s->marker = s->token = s->line = s->buffer;
	s->is_eof = s->is_doc = 0;
	s->indent_level = 0;
	s->fn = 0;
	s->fp = 0;
}

/** Uninitialises the address of {ps}.
 @return Success; {*ps} is null whatever the return.
 @throws fclose
 @allow */
static int Scanner_(struct Scanner **const ps) {
	struct Scanner *s;
	int ret = 1;
	if(!ps || !(s = *ps)) return 1;
	if(s->fp) { if(fclose(s->fp) == EOF) ret = 0; s->fp = 0; }
	Scanner_zero(s);
	free(s), s = 0, *ps = 0;
	return ret;
}

/** Constructs a {struct Scanner}.
 @param{fn} File name that the scanner reads and stores in memory.
 @return The {Scanner} or null.
 @throws fopen malloc free
 @allow */
static struct Scanner *Scanner(const char *const fn) {
	struct Scanner *s;
	assert(s && fn);
	if(!(s = malloc(sizeof *s))) return 0;
	Scanner_zero(s);
	s->fn = fn;
	if(!(s->fp = fopen(s->fn, "r"))) { Scanner_(&s); return 0; }
	s->limit = s->cursor = s->marker = s->token = s->line = s->buffer + SIZE;
	return s;
}

static int Scanner_fill(struct Scanner *const s, const size_t need) {
	const size_t a = s->token - s->buffer;
	size_t read;
	assert(s);
	if(s->is_eof || a < need) return 0;
	memmove(s->buffer, s->token, s->limit - s->token);
	s->limit -= a;
	s->cursor -= a;
	s->marker -= a;
	s->token -= a;
	s->line -= a;
	read = fread(s->limit, 1, a, s->fp);
	if(ferror(s->fp)) return 0;
	s->limit += read;
	if(s->limit < s->buffer + SIZE) {
		s->is_eof = 1;
		memset(s->limit, 0, YYMAXFILL);
		s->limit += YYMAXFILL;
	}
	return 1;
}

/*!re2c
re2c:define:YYCTYPE = char;
re2c:define:YYCURSOR = s->cursor;
re2c:define:YYMARKER = s->marker;
re2c:define:YYLIMIT = s->limit;
re2c:yyfill:enable = 1;
re2c:define:YYFILL = "if(!Scanner_fill(s, @@)) return 0;";
re2c:define:YYFILL:naked = 1;
*/

static enum Token scan_doc(struct Scanner *const s);
static enum Token scan_code(struct Scanner *const s);

/** Lex. There are 2 states: {doc}, {code}.
 @allow */
static enum Token scan(struct Scanner *const s) {
	assert(s);
	return s->is_doc ? scan_doc(s) : scan_code(s);
}

/** Scans documentation. */
static enum Token scan_doc(struct Scanner *const s) {
doc:
	s->token = s->cursor;
/*!re2c
	whitespace = [ \t\n\v\f\r];
	whitespace* { return WHITESPACE; }
	word = [^ \t\n\v\f\r\\,@{}&<>*]*; /* This is kind of sketchy. */
	word { return THING; }

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

	"*/" { s->is_doc = 0; return scan_code(s); }
	* { goto doc; } /* @fixme: this probably hides stuff? */

	/* Also escape these for {HTML}. */
	"&" { return HTML_AMP; }
	"<" { return HTML_LT; }
	">" { return HTML_GT; }
*/
}

/** Scans C code. */
static enum Token scan_code(struct Scanner *const s) {
code:
	s->token = s->cursor;
/*!re2c
	* { printf("Unknown, <%.*s>, just roll with it.\n",
		(int)(s->cursor - s->token), s->token); goto code; }

	/* @fixme Don't see how this would happen? */
	/* end = "\x00";
	end { if(s->limit - s->token == YYMAXFILL) return END; goto code; } */

	macro = ("#" | "%:") ([^\n] | "\\\n")* "\n";
	macro        { s->line = s->cursor; return PREPROCESSOR; }

	doc = "/""**";
	doc { s->is_doc = 1; return scan_doc(s); }

	mcm = "/""*" ([^*] ([^*] | ("*" [^/]))*)? "*""/";
	scm = "//" [^\n]* "\n";
	wsp = ([ \t\v\n\r] | scm | mcm)+;
	wsp          { printf("< >\n"); goto code; }

	size_mod = "u"|"l"|"ul"|"lu";
	oct = "0" [0-7]* size_mod?;
	dec = "-"? [1-9][0-9]* size_mod?; /* What about: -- -74? */
	hex = '0x' [0-9a-fA-F]+ size_mod?;
	frc = "-"? [0-9]* "." [0-9]+ | [0-9]+ ".";
	exp = 'e' [+-]? [0-9]+;
	flt = (frc exp? | [0-9]+ exp) [fFlL]?;
	/* @fixme No trigraph support. */
	string_literal = "L"? "\"" ([^"] | "\\". | "\\\n")* "\"";
	char_literal = "\'" (. | "\\".) "\'";
	oct | dec | hex | flt | string_literal | char_literal { return CONSTANT; }

	"*"          { return STAR; }
	"&"          { return AMPER; }
	"."          { return DOT; }
	"->"         { return ARROW; }
	"=="         { return COMPARISON; }
	"="          { return EQUALS; }
	"struct"     { return STRUCT; }
	"typedef"    { return TYPEDEF; }
	("{" | "<%") { s->indent_level++; return LBRACE; }
	("}" | "%>") { s->indent_level--; return RBRACE; }
	("[" | "<:") { return LBRACK; }
	("]" | ":>") { return RBRACK; }
	"("          { return LPAREN; }
	")"          { return RPAREN; }
	","          { return COMMA; }
	";"          { s->line = s->cursor; return SEMI; }
	":"          { return COLON; }
	"..."        { return ELLIPSES; }

	id = [a-zA-Z_][a-zA-Z_0-9]*;
	id           { return ID; }
*/
}

/** Parser is only valid when Scanner is active. */
struct Symbol {
	enum Token token;
	const char *from, *to;
};

static void symbol_to_string(const struct Symbol *s, char (*const a)[12]) {
	int len = s->to - s->from;
	if(len > 5) len = 5;
	sprintf(*a, "%.4s\"%.*s\"", tokens[s->token], len, s->from);
	/*strncpy(*a, tokens[s->token], sizeof *a - 1);*/
	*a[sizeof *a - 1] = '\0';
}

#define ARRAY_NAME Symbol
#define ARRAY_TYPE struct Symbol
#define ARRAY_TO_STRING &symbol_to_string
#include "Array.h"

struct Segment {
	struct SymbolArray doc, code;
	enum { NOT_SET, HEADER, DECLARATION, FUNCTION } type;
};

#define ARRAY_NAME Segment
#define ARRAY_TYPE struct Segment
#include "Array.h"

int main(void) {
	const char *const fn = "x.c";
	struct Scanner *s = 0;
	enum Token t;
	int is_done = 0;
	struct SegmentArray text;
	struct Segment *segment = 0;
	struct Symbol *symbol;
	int is_fn = 0;
	SegmentArray(&text);
	do {
		if(!(s = Scanner(fn))) break;
		while((t = scan(s))) {
			int i;
			for(i = -2; i < s->indent_level; i++) fputc('\t', stdout);
			printf("%s \"%.*s\"\n", tokens[t],
				(int)(s->cursor - s->token), s->token);
			if(!is_fn) {
				if(s->indent_level == 1) { /* Entering a function. */
					assert(!s->is_doc);
					/* @fixme: not all {} are functions.
					 Check that -2 != struct and -1 == RBRACK.
					 Both will fail on generics and old-style.
					 Check that space before? two : one spaces there is
					 struct (fails on multi-parameter) */
					is_fn = 1, assert(t == LBRACE);
				}
			} else {
				if(!s->indent_level) { /* Exiting a function. */
					is_fn = 0, assert(t == RBRACE);
				} else if(!s->is_doc) { /* Code in functions; we don't care. */
					continue;
				}
			}
			/* Create new segment if need be. */
			if(!segment) {
				if(!(segment = SegmentArrayNew(&text))) break;
				SymbolArray(&segment->doc);
				SymbolArray(&segment->code);
				segment->type = NOT_SET;
			}
			/* Create a new symbol. */
			if(!(symbol = SymbolArrayNew(s->is_doc
				? &segment->doc : &segment->code))) break;
			symbol->token = t;
			symbol->from = s->token;
			symbol->to = s->cursor;
			/* @fixme
			 Different doc comments should definitely be paragraphed. */
			/* Create another segment next time. */
			if(!s->is_doc && (t == SEMI || t == RBRACE)) segment = 0;
		}
		if(t) break;
		is_done = 1;
	} while(0); if(!is_done) {
		perror(fn);
	} else {
		fputs("\n\n*****\n\n", stdout);
		segment = 0;
		while((segment = SegmentArrayNext(&text, segment))) {
			printf("Segment:\n\tdoc: %s.\n\tcode: %s.\n\n",
				SymbolArrayToString(&segment->doc),
				SymbolArrayToString(&segment->code));
			/*if(symbols->token == WHITESPACE) {
				fputs(" ,", stdout);
			} else {
				printf("<%.*s>,", (int)(symbols->to - symbols->from), symbols->from);
			}*/
		}
		fputc('\n', stdout);
	} {
		while((segment = SegmentArrayPop(&text)))
			SymbolArray_(&segment->doc), SymbolArray_(&segment->code);
		SegmentArray_(&text);
		Scanner_(&s);
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
