/** Parses one line of code very fuzzily to determine where in the document it
 should be.
 @depend [Lemon](http://www.hwaci.com/sw/lemon/) (included.) */

/* Copyright (GPL) 2004 mchirico@users.sourceforge.net or mchirico@comcast.net
 
 Simple lemon parser  example.
 
 
 $ ./lemon example1.y                          
 
 The above statement will create example1.c.
 
 The command below  adds  main and the
 necessary "Parse" calls to the
 end of this example1.c.
 
 
 $ cat <<EOF >>example1.c                      
 int main()                                    
 {                                             
 void* pParser = ParseAlloc (malloc);        
 Parse (pParser, INTEGER, 1);                
 Parse (pParser, PLUS, 0);                   
 Parse (pParser, INTEGER, 2);                
 Parse (pParser, 0, 0);                      
 ParseFree(pParser, free );                  
 }                                            
 EOF                                           
 
 Downloads:
 http://prdownloads.sourceforge.net/souptonuts/lemon_examples.tar.gz?download
 
 */

%token_type {int}  

%left PLUS MINUS.   
%left DIVIDE TIMES.  

%include {   
#include <stdio.h>
#include "../src/Division.h"
#include "../src/Parse.h"
}

%syntax_error {  
	fprintf(stderr, "Syntax error!\n");  
}   
	
program ::= expr(A).   { printf("Result=%d\n", A); }  
	
expr(A) ::= expr(B) MINUS  expr(C).   { A = B - C; }  
expr(A) ::= expr(B) PLUS  expr(C).   { A = B + C; }  
expr(A) ::= expr(B) TIMES  expr(C).   { A = B * C; }  
expr(A) ::= expr(B) DIVIDE expr(C).  { 
		
	if(C != 0){
		A = B / C;
	}else{
		printf("divide by zero\n");
	}
}  /* end of DIVIDE */
	
expr(A) ::= INTEGER(B). { A = B; }
