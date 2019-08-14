/** Parses one line of code very fuzzily to determine where in the document it
 should be.
 @depend [Bison](https://www.gnu.org/software/bison/) */

%{ /* -- Prologue. -- */
#include <stdio.h>
#include "../src/Report.h"
#include "../src/Parser.h"

/* Include these for Bison. */
int yylex(void);
void yyerror(char const *);

%}

/* -- Bison declarations. -- */

%%
/* Grammer rules. */

program : expr(A). { printf("Result = %s.\n", symbols[A]); }

expr(A) : INTEGER(B). { A = B; }
//expr(A) ::= tag(B). { A = B; }
expr(A) : id(B). { A = B; }

/*program ::= id(A).   { printf("Result=%s\n", A); }
 program ::= id.*/

//tag ::= STRUCT | UNION | ENUM.
//name ::= ID | STATIC | VOID | STRUCT | UNION | ENUM.

id ::= ID.
id ::= ID_ONE_GENERIC LPAREN ID RPAREN.
/*| ID_TWO_GENERICS LPAREN name COMMA name RPAREN
 | ID_THREE_GENERICS LPAREN name COMMA name COMMA name RPAREN.*/

/*expr(A) ::= expr(B) MINUS  expr(C).   { A = B - C; }
 expr(A) ::= expr(B) PLUS  expr(C).   { A = B + C; }
 expr(A) ::= expr(B) TIMES  expr(C).   { A = B * C; }
 expr(A) ::= expr(B) DIVIDE expr(C).  {
 expr(A) ::= INTEGER(B). { A = B; }*/
%%

/* Epilog. */

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
	if(!symbol) { printf("END Parse.\n"); ReportDivision(DIV_FUNCTION); }
}
