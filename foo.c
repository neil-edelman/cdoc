/** Header. Yo <http://foo.org/> [MIT](https://foo.org/)

Paragraph @ @a \,~~

 -* This
 -* is
 -* bah.

 Escapes. \\ \` \@ \_ \*  /// \\\`

 @title Foo `Bar`
 @depend C89
 lala	lala? <http://foo.com/> */

/** Data. */
static struct T_(Array) (*const a)(int, int a, int (*a)(const int a));

/** Typedef. */
typedef A_B_(Foo, Bar) int (*)(int);

/** Data. */
static int T_U_V_(Foo, Bar  ,  Baz)(void) = 0;

/** declare x as array 3 of pointer to function returning pointer to array 5 of
 char */
char (*(*x[3])())[5];

/** declare foo as pointer to function (void) returning pointer to array 3 of
 int */
int (*(*foo)(void ))[3];


/** declare x as function (int) returning pointer to function (int) returning
 pointer to function (pointer to function (int) returning int) returning int */
int (*(*x(const int a))(int ))(int (*)(int )) {
}

/****************/

/** Returns eof.
 @implements ScannerFn
 @allow */
static enum Token scan_eof(struct Scanner *const s) { (void)s; return END; }

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
static enum Token scan_comment(struct Scanner *const s) {
	assert(s && state_look(s) == COMMENT);
comment:
}

/** Function. */
T a(int (*ptr)(int a, int (*fn)(void))) /** yo */ {
	/** @param[a, fn] [] Doesn't do anything. */
	a = a;
}

/** Foo. */
(foo)];
