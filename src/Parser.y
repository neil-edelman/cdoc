/** Parses one line of code very fuzzily to determine where in the document it
 should be.
 @depend [Lemon](http://www.hwaci.com/sw/lemon/) (included.) */

%token_type {enum Symbol}

%include {
#include <stdlib.h> /* malloc free */
#include <stdio.h>
#include "../src/Division.h"
#include "../src/Parser.h"

void ParseTrace(FILE *TraceFILE, char *zTracePrompt);
void ParseInit(void *yypRawParser);
void *ParseAlloc(void *(*)());
void ParseFinalize(void *p);
void ParseFree(void *p, void (*freeProc)(void*));
void Parse(void *yyp, int yymajor, enum Symbol yyminor);
int ParseFallback(int iToken);

}

%syntax_error {
	fprintf(stderr, "Syntax error!\n");
}

%parse_accept {
	printf("parsing complete!\n\n\n");
}

//%extra_argument
%left PLUS MINUS.

program ::= expr(A). { printf("Result = %s.\n", symbols[A]); }

expr(A) ::= INTEGER(B). { A = B; }

/*program ::= id(A).   { printf("Result=%s\n", A); }
program ::= id.

tag ::= STRUCT | UNION | ENUM.
name ::= ID | STATIC | VOID | STRUCT | UNION | ENUM.

id ::= ID
| ID_ONE_GENERIC LPAREN name RPAREN
| ID_TWO_GENERICS LPAREN name COMMA name RPAREN
| ID_THREE_GENERICS LPAREN name COMMA name COMMA name RPAREN.

/*expr(A) ::= expr(B) MINUS  expr(C).   { A = B - C; }
expr(A) ::= expr(B) PLUS  expr(C).   { A = B + C; }
expr(A) ::= expr(B) TIMES  expr(C).   { A = B * C; }
expr(A) ::= expr(B) DIVIDE expr(C).  {
expr(A) ::= INTEGER(B). { A = B; }*/

%code{
void *parser;
void Parser_(void) {
	if(!parser) return;
	ParseFree(parser, &free);
	parser = 0;
}
int Parser(void) {
	if(parser) return 1;
	if(!(parser = ParseAlloc(&malloc))) return 0;
	return 1;
}
void ParserSymbol(enum Symbol symbol) {
	if(!parser) return;
	Parse(parser, 0, symbol);
}
}
