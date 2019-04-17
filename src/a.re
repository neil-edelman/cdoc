/** 2019 Neil Edelman, distributed under the terms of the MIT License;
 see readme.txt, or \url{ https://opensource.org/licenses/MIT }. */

#include <stdlib.h>
#include <stdio.h>  /* FILE */
#include <string.h> /* memmove memset */
#include <errno.h>
#include <assert.h> /* assert */

/* X-Marco. */
#define IDENT(A) A
#define STRINGISE(A) #A
/*#define TOKEN(X) \
	X(END), X(MACRO), X(LBRACE), X(RBRACE), X(LPAREN), X(RPAREN), \
	X(POINTER), X(ADDRESS), X(ID), X(SEMI), X(COMMA), X(COLON), \
	X(COMPARISON), X(EQUALS), X(STRUCT), X(TYPEDEF), \
	X(INT_CONSTANT), X(FLOAT_CONSTANT), X(STRING_LITERAL), X(CHAR_LITERAL), \
	X(PARAM), X(SEE)*/
#define TOKEN(X) \
	X(END), X(THING), X(LBRACE), X(RBRACE), X(LPAREN), X(RPAREN), X(ID), \
	X(COMMA), X(SEMI), X(COLON)
enum Token { TOKEN(IDENT) };
static const char *const tokens[] = { TOKEN(STRINGISE) };

/*!max:re2c*/
#define SIZE (64 * 1024)

struct Scanner {
	char buffer[SIZE + YYMAXFILL], *limit, *cursor, *marker, *token;
	int eof;
	const char *fn;
    FILE *fp;
	size_t line;
	enum Token t;
};

/** Fill the structure with default values. */
static void Scanner_zero(struct Scanner *const s) {
	assert(s);
	s->limit = s->cursor = s->marker = s->token = s->buffer;
	s->eof = 0;
	s->fn = 0;
	s->line = 0;
	s->t = END;
}

/** Uninitialises the address of {ps}. */
static void Scanner_(struct Scanner **const ps) {
	struct Scanner *const s = *ps;
	assert(ps);
	if(!s) return;
	if(s->fp) { if(fclose(s->fp) == EOF) perror(s->fn); s->fp = 0; }/* @fixme */
	Scanner_zero(s);
	free(s), *ps = 0;
}

/** @param s: An uninitialised scanner.
 @param fn: File name that the scanner reads and stores in memory.
 @return Success.
 @throws fopen fclose fseek ftell fread malloc
 @throws EDOM on empty file. */
static struct Scanner *Scanner(const char *const fn) {
	struct Scanner *s;
	assert(s && fn);
	if(!(s = malloc(sizeof *s))) return 0;
	Scanner_zero(s);
	s->fn = fn;
	if(!(s->fp = fopen(s->fn, "r"))) { Scanner_(&s); return 0; }
	s->limit = s->cursor = s->marker = s->token = s->buffer + SIZE;
	s->line = 1;
	return s;
}

static int Scanner_fill(struct Scanner *const s, const size_t need) {
	const size_t a = s->token - s->buffer;
	size_t read;
	assert(s);
	if(s->eof || a < need) return 0;
	memmove(s->buffer, s->token, s->limit - s->token);
	s->limit -= a;
	s->cursor -= a;
	s->marker -= a;
	s->token -= a;
	read = fread(s->limit, 1, a, s->fp);
	if(ferror(s->fp)) return 0;
	s->limit += read;
	if(s->limit < s->buffer + SIZE) {
		s->eof = 1;
		memset(s->limit, 0, YYMAXFILL);
		s->limit += YYMAXFILL;
	}
	return 1;
}

/** Lex. */
static int scan(struct Scanner *const s) {
	assert(s);

/*!re2c
	re2c:define:YYCTYPE = char;
	re2c:define:YYCURSOR = s->cursor;
	re2c:define:YYMARKER = s->marker;
	re2c:define:YYLIMIT = s->limit;
	re2c:yyfill:enable = 1;
	re2c:define:YYFILL = "if(!Scanner_fill(s, @@)) return 0;";
	re2c:define:YYFILL:naked = 1;
*/

	for( ; ; ) {
		s->token = s->cursor;
		printf("Starting.\n");
/*!re2c
		* { return 0; }

		end = "\x00";
		end { return s->limit - s->token == YYMAXFILL; }

		macro = ("#" | "%:") ([^\n] | "\\\n")* "\n";
		macro { printf("<macro>\n"); continue; }

		mcm = "/""*" ([^*] | ("*" [^/]))* "*""/";
		scm = "//" [^\n]* "\n";
		wsp = ([ \t\v\n\r] | scm | mcm)+;
		wsp { printf("~\n"); continue; }

		"L"? ['"] { printf("<string not implemented>\n"); continue; }
		"L"? "''" { return 0; }

		size_mod = "u"|"l"|"ul"|"lu";
		oct = "0" [0-7]* size_mod?;
		dec = "-"? [1-9][0-9]* size_mod?; /* What about: -- -74? */
		hex = '0x' [0-9a-fA-F]+ size_mod?;
		oct | dec | hex { printf("<int>\n"); continue; }

		frc = "-"? [0-9]* "." [0-9]+ | [0-9]+ ".";
		exp = 'e' [+-]? [0-9]+;
		flt = (frc exp? | [0-9]+ exp) [fFlL]?;
		flt { printf("<float>\n"); continue; }

		("{" | "<%") { printf("{"); continue; }
		("}" | "%>") { printf("}");      continue; }
		("[" | "<:")      { printf("[");      continue; }
		("]" | ":>")      { printf("]");      continue; }
		"("               { printf("(");      continue; }
		")"               { printf(")");      continue; }
		";"               { printf(";");      continue; }
		":"               { printf(":");      continue; }
		"..."             { printf("...");    continue; }

		id = [a-zA-Z_][a-zA-Z_0-9]*;
		id { printf("%.*s", (int)(s->cursor - s->token), s->token); return 1; }
*/
		printf("Got here.\n");
	}

	/*newline = "\r\n" | "\n" | "\r";
	newline { s->marker = s->cursor; s->line++; goto regular; }
	"{" { return LBRACE; }
	"}" { return RBRACE; }
	"(" { return LPAREN; }
	")" { return RPAREN; }
	identifier = ([a-zA-Z_][0-9a-zA-Z_]*);
	identifier { return ID; }
	"," { return COMMA; }
	";" { return SEMI; }
	":" { return COLON; }
	* { return THING; }*/

#if 0
/* !re2c

	"/**" { goto doc; }

	"/*" { goto comment; }

	"*" { return POINTER; }

	"&" { return ADDRESS; }

	"==" { return COMPARISON; }

	"=" { return EQUALS; }

	"," { return COMMA; }

	";" { return SEMI; }

	":" { return COLON; }

	"struct" { return STRUCT; }

	"typedef" { return TYPEDEF; }

	identifier = ([a-zA-Z_][0-9a-zA-Z_]*);
	identifier { return ID; }

	whitespace = [ \t\v\f]+;
	slc = "//" [^\n]* "\n";
	mlc = "/*" ([^*] | ("*" [^/]))* "*""/";
	slc mlc whitespace { goto regular; }

	macro = ("#" | "%:") ([^\n] | "\\\n")* "\n";
	macro { return MACRO; }

	//string = ("\"" [^"]* "\"") | ("\'" [^']* "\'");
	string_literal = "L"? "([^\\\"]|\\.)*";
	string_literal { return STRING_LITERAL; }

	char_literal = "\'" "(. | (\.))" "\'";
	char_literal { return CHAR_LITERAL; }

	oct = "0" [0-7]*;
	dec = "-"? [1-9][0-9]* ("L"|"l"|"U"|"u")*;
	hex = '0x' [0-9a-fA-F]+;
	oct dec hex { return INT_CONSTANT; }

	frc = [0-9]* "." [0-9]+ | [0-9]+ ".";
	exp = 'e' [+-]? [0-9]+;
	flt = (frc exp? | [0-9]+ exp) [fFlL]?;
	flt { return FLOAT_CONSTANT; }

	newline = "\r\n" | "\n" | "\r";
	newline { s->marker = s->cursor; s->line++; goto regular; }

	* { return WTF; }
*/

comment:
	if(s->cursor >= s->limit) return END;
/* !re2c
	"*/" { goto regular; }
	newline { s->line++; goto comment; }
	* { goto comment; }
*/

doc:
	if(s->cursor >= s->end) return END;
/* !re2c
	"{" { return LBRACE; }
	"}" { return RBRACE; }
	":" { return COLON; }
	"@param" { return PARAM; }
	"@see" { return SEE; }
	"*/" { goto regular; }
	* { goto doc; }
*/
#endif

}

int main(void) {
	const char *const fn = "x.c"; 
	struct Scanner *s = 0;
	int is_done = 0;
	do {
		if(!(s = Scanner(fn))) break;
		while(scan(s)) printf("%s on line %lu \"%.*s\"\n", tokens[s->t],
			(unsigned long)s->line, (int)(s->cursor - s->token), s->cursor);
		is_done = 1;
	} while(0); if(!is_done) {
		perror(fn);
	} {
		Scanner_(&s);
	}
	return is_done ? EXIT_SUCCESS : EXIT_FAILURE;
}
