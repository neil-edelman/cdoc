#ifndef XMACRO_H /* <!-- !x */
#define XMACRO_H

/* X-Macros are great for debugging. */

#define PARAM(A) A
#define STRINGISE(A) #A

/*#define PARAM2A(A, B) A
#define PARAM2B(A, B) B
#define STRINGISE2A(A, B) #A*/

#define PARAM3A(A, B, C) A
#define PARAM3B(A, B, C) B
#define PARAM3C(A, B, C) C
#define STRINGISE3A(A, B, C) #A

#define PARAM6A(A, B, C, D, E, F) A
#define PARAM6B(A, B, C, D, E, F) B
#define PARAM6C(A, B, C, D, E, F) C
#define PARAM6D(A, B, C, D, E, F) D
#define PARAM6E(A, B, C, D, E, F) E
#define PARAM6F(A, B, C, D, E, F) F
#define STRINGISE6A(A, B, C, D, E, F) #A

#endif /* !x */
