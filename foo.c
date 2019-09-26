/** [This is
text
](http://yo.c) Header. Yo <http://foo.org/> [MIT](https://mit.edu/)

Paragraph @ @a \,~~

 \* This
 \* is
 \* bah.

 @title Foo `Bar`
 @depend C89
 
 Escapes. \\ \` \@ \_ \*  /// \\\`
 lala	preformated <http://foo.com/> */

#include <assert.h>

#ifdef CAT
#undef CAT
#endif
#ifdef CAT_
#undef CAT_
#endif
#ifdef PCAT
#undef PCAT
#endif
#ifdef PCAT_
#undef PCAT_
#endif
#ifdef T
#undef T
#endif
#ifdef T_
#undef T_
#endif
#ifdef PT_
#undef PT_
#endif
#define CAT_(x, y) x ## y
#define CAT(x, y) CAT_(x, y)
#define PCAT_(x, y) x ## _ ## y
#define PCAT(x, y) PCAT_(x, y)
#define T_(thing) CAT(array, thing)
#define PT_(thing) PCAT(array, PCAT(int, thing))
#define A_B_(a, b) CAT(CAT(array, a), b)
#define A_B_C_(a, b, c) CAT(CAT(CAT(array, a), b), c)

/** Troubles with this line? check to ensure that `ARRAY_TYPE` is a valid type,
 whose definition is placed above `#include "Array.h"`. */
typedef int PT_(Type);
#define T PT_(Type)

/* Tag. */
struct T_(Array) {
	int i;
} t, x;

/** Decl. */
static struct T_(Array) (*const a)(int, int a, int (*b)(const int a));

/** Typedef. */
typedef int (*A_B_(Foo, Bar))(int);

/** Decl. */
static int (*A_B_C_(Foo, Bar  ,  Baz))(void) = 0;

/** declare x as array 3 of pointer to function returning pointer to array 5 of
 char */
char (*(*y[3])())[5];

/** declare foo as pointer to function (void) returning pointer to array 3 of
 int */
int (*(*foo)(void ))[3];

/** ? */
int (*c);

/** Function declare x as function (int) returning pointer to function (int)
 returning pointer to function (pointer to function (int) returning int)
 returning int
 @param[arg1_1] Yes. */
int (*(*fun1(const int arg1_1))(int ))(int (*)(int )) {
	(void)arg1_1;
	return 0;
}

/** Typedef. */
typedef void (*PT_(ToString))(const T *, char (*const)[12]);

/** Decl. */
static const PT_(ToString) PT_(to_string) = 0;

/** Typedef. Operates by side-effects on {data} only. */
typedef void (*PT_(Action))(T *const data);

/** Typedef. Given constant {data}, returns a boolean. */
typedef int (*PT_(Predicate))(const T *const data);

/****************/

/** Tag */
struct Scanner;

/** Tag */
enum Token { END };

/** Returns eof.
 @param[arg1_2] Arg 1/2.
 @param[arg2_2] Arg 2/2.
 @fixme
 @return END.
 @implements ScannerFn
 @allow */
static enum Token fun2_scan_eof(struct Scanner *const arg1_2, int arg2_2) {
	(void)arg1_2, (void)arg2_2; return END;
}

/**
 * Header. That also goes in the header.
 *
 *
 * This is a
 * kernel-style comment.
 * <http://www.`@.com>
 * <Yo2019> ?<>&!
 * <fn:a>
 *
 *
 */

/*********************
 * Header.           *
 * This is an ascii  *
 * art comment. Eww. *
 *********************/

/** Function.
 @implements ScannerFn
 @allow */
static enum Token fun3_scan_comment(struct Scanner *const arg1_1) {
	goto comment;
comment:
	(void)arg1_1;
	return END;
}

typedef int Foo;

/** Function `arg1_2`.
 @return Function of `arg2_2`. */
T fun4(int (*arg1_2)(int a, int (*fn)(void)), Foo arg2_2) /** yo */ {
	/** @param[a, fn] [] Doesn't do anything. */
	return arg1_2(arg2_2, 0);
}

int main(void) {
	arrayFooBarBaz = 0;
	fun3_scan_comment(0);
	fun2_scan_eof(0, 0);
	return 0;
}

/** @title title! <http://f.o/> */
