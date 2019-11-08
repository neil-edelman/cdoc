#ifndef XMACRO_H /* <-- !x */
#define XMACRO_H

/* X-Macros are great for debugging. */

#define PARAM(A) A
#define STRINGISE(A) #A

#define PARAM2A(A, B) A
#define PARAM2B(A, B) B
#define STRINGISE2A(A, B) #A

#define PARAM7A(A, B, C, D, E, F, G) A
#define PARAM7B(A, B, C, D, E, F, G) B
#define PARAM7C(A, B, C, D, E, F, G) C
#define PARAM7D(A, B, C, D, E, F, G) D
#define PARAM7E(A, B, C, D, E, F, G) E
#define PARAM7F(A, B, C, D, E, F, G) F
#define PARAM7G(A, B, C, D, E, F, G) G
#define STRINGISE7A(A, B, C, D, E, F, G) #A

#endif /* !x */
