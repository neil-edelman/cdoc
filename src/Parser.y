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

%defines

%token P_ID P_CONSTANT
%token P_OPERATOR P_ASSIGNEMENT
%token P_TYPEDEF P_STATIC
%token P_VOID
%token P_STRUCT P_UNION P_ENUM P_ELLIPSIS

%token P_LBRACK P_RBRACK P_LBRACE P_RBRACE P_LPAREN P_RPAREN
%token P_ASSIGNMENT P_SEMI P_COMMA

%start translation_unit

%%
/* Grammer rules. */

primary_expression
: P_ID
| P_CONSTANT
| P_LPAREN expression P_RPAREN
;

postfix_expression
: primary_expression
| postfix_expression P_LBRACK expression P_RBRACK
| postfix_expression P_LPAREN P_RPAREN
| postfix_expression P_LPAREN argument_expression_list P_RPAREN
| postfix_expression P_OPERATOR P_ID
| postfix_expression P_OPERATOR P_ID
| postfix_expression P_OPERATOR
;

argument_expression_list
: assignment_expression
| argument_expression_list P_COMMA assignment_expression
;

unary_expression
: postfix_expression
| P_OPERATOR unary_expression
| P_OPERATOR cast_expression
| P_ID unary_expression
| P_ID P_LPAREN type_name P_RPAREN
;

cast_expression
: unary_expression
| P_LPAREN type_name P_RPAREN cast_expression
;

abstract_expression
: cast_expression
| abstract_expression P_OPERATOR abstract_expression
| abstract_expression P_OPERATOR cast_expression
;

constant_expression
: abstract_expression
| abstract_expression P_OPERATOR expression P_OPERATOR constant_expression
;

assignment_expression
: constant_expression
| unary_expression assignment_operator assignment_expression
;

assignment_operator
: P_ASSIGNMENT
| P_OPERATOR
;

expression
: assignment_expression
| expression P_COMMA assignment_expression
;

declaration
: declaration_specifiers P_SEMI
| declaration_specifiers init_declarator_list P_SEMI
;

declaration_specifiers
: storage_class_specifier
| storage_class_specifier declaration_specifiers
| type_specifier
| type_specifier declaration_specifiers
| P_ID
| P_ID declaration_specifiers
;

init_declarator_list
: init_declarator
| init_declarator_list P_COMMA init_declarator
;

init_declarator
: declarator
| declarator P_ASSIGNMENT initializer
;

storage_class_specifier
: P_TYPEDEF
| P_ID
| P_STATIC
;

type_specifier
: P_VOID
| P_ID
| struct_or_union_specifier
| enum_specifier
;

struct_or_union_specifier
: struct_or_union P_ID P_LBRACE struct_declaration_list P_RBRACE
| struct_or_union P_LBRACE struct_declaration_list P_RBRACE
| struct_or_union P_ID
;

struct_or_union
: P_STRUCT
| P_UNION
;

struct_declaration_list
: struct_declaration
| struct_declaration_list struct_declaration
;

struct_declaration
: specifier_qualifier_list struct_declarator_list P_SEMI
;

specifier_qualifier_list
: type_specifier specifier_qualifier_list
| type_specifier
| P_ID specifier_qualifier_list
| P_ID
;

struct_declarator_list
: struct_declarator
| struct_declarator_list P_COMMA struct_declarator
;

struct_declarator
: declarator
| P_OPERATOR constant_expression
| declarator P_OPERATOR constant_expression
;

enum_specifier
: P_ENUM P_LBRACE enumerator_list P_RBRACE
| P_ENUM P_ID P_LBRACE enumerator_list P_RBRACE
| P_ENUM P_ID
;

enumerator_list
: enumerator
| enumerator_list P_COMMA enumerator
;

enumerator
: P_ID
| P_ID P_ASSIGNMENT constant_expression
;

declarator
: pointer direct_declarator
| direct_declarator
;

direct_declarator
: P_ID
| P_LPAREN declarator P_RPAREN
| direct_declarator P_LBRACK constant_expression P_RBRACK
| direct_declarator P_LBRACK P_RBRACK
| direct_declarator P_LPAREN parameter_type_list P_RPAREN
| direct_declarator P_LPAREN identifier_list P_RPAREN
| direct_declarator P_LPAREN P_RPAREN
;

pointer
: P_OPERATOR
| P_OPERATOR type_qualifier_list
| P_OPERATOR pointer
| P_OPERATOR type_qualifier_list pointer
;

type_qualifier_list
: P_ID
| type_qualifier_list P_ID
;


parameter_type_list
: parameter_list
| parameter_list P_COMMA P_ELLIPSIS
;

parameter_list
: parameter_declaration
| parameter_list P_COMMA parameter_declaration
;

parameter_declaration
: declaration_specifiers declarator
| declaration_specifiers abstract_declarator
| declaration_specifiers
;

identifier_list
: P_ID
| identifier_list P_COMMA P_ID
;

type_name
: specifier_qualifier_list
| specifier_qualifier_list abstract_declarator
;

abstract_declarator
: pointer
| direct_abstract_declarator
| pointer direct_abstract_declarator
;

direct_abstract_declarator
: P_LPAREN abstract_declarator P_RPAREN
| P_LBRACK P_RBRACK
| P_LBRACK constant_expression P_RBRACK
| direct_abstract_declarator P_LBRACK P_RBRACK
| direct_abstract_declarator P_LBRACK constant_expression P_RBRACK
| P_LPAREN P_RPAREN
| P_LPAREN parameter_type_list P_RPAREN
| direct_abstract_declarator P_LPAREN P_RPAREN
| direct_abstract_declarator P_LPAREN parameter_type_list P_RPAREN
;

initializer
: assignment_expression
| P_LBRACE initializer_list P_RBRACE
| P_LBRACE initializer_list P_COMMA P_RBRACE
;

initializer_list
: initializer
| initializer_list P_COMMA initializer
;

declaration_list
: declaration
| declaration_list declaration
;

translation_unit
: external_declaration
| translation_unit external_declaration
;

external_declaration
: function_definition
| declaration
;

function_definition
: declaration_specifiers declarator declaration_list //compound_statement
| declaration_specifiers declarator //compound_statement
| declarator declaration_list //compound_statement
| declarator //compound_statement
;


//program : expr(A). { printf("Result = %s.\n", symbols[A]); }

//expr(A) : INTEGER(B). { A = B; }
////expr(A) ::= tag(B). { A = B; }
//expr(A) : id(B). { A = B; }

/*program ::= id(A).   { printf("Result=%s\n", A); }
 program ::= id.*/

////tag ::= P_STRUCT | P_UNION | P_ENUM.
////name ::= P_ID | P_STATIC | P_VOID | P_STRUCT | P_UNION | P_ENUM.

//id ::= P_ID.
//id ::= ID_ONE_GENERIC P_LPAREN P_ID P_RPAREN.
/*| ID_TWO_GENERICS P_LPAREN name P_COMMA name P_RPAREN
 | ID_THREE_GENERICS P_LPAREN name P_COMMA name P_COMMA name P_RPAREN.*/

/*expr(A) ::= expr(B) MINUS  expr(C).   { A = B - C; }
 expr(A) ::= expr(B) PLUS  expr(C).   { A = B + C; }
 expr(A) ::= expr(B) TIMES  expr(C).   { A = B * C; }
 expr(A) ::= expr(B) DIVIDE expr(C).  {
 expr(A) ::= INTEGER(B). { A = B; }*/
%%

/* Epilog. */

void ParserToken(const struct Token *const token) {
	/*Parse(parser, 0, symbol);
	if(!symbol) { printf("END Parse.\n"); ReportDivision(DIV_FUNCTION); }*/
}
