/** Yo <http://foo.org/> [MIT](https://foo.org/)

Paragraph @ @a \,~~

 -* This
 -* is
 -* bah.

@title Foo
@depend C89
lala	lala? <http://foo.com/>
*/



/** A `a`. */
static T_(Array) a;

/** B */
struct T_U_(Array, Sort) b;

/** C */
int T_U_V_(Foo, Bar  ,  Baz)(void);

/** Escapes. \\ \` \@ \_ \*  /// \\\`*/

/****************/

/** Returns eof.
 @implements ScannerFn
 @allow */
static enum Token scan_eof(struct Scanner *const s) { (void)s; return END; }

/**
 *
 *
 * This is a
 * kernel-style comment.
 * <http://www.`@.com>
 * <Yo2019> ?<>&!
 * <fn:a>
 *
 *
 * Aha!
 */

/*********************
 * This is an ascii  *
 * art comment. Eww. *
 ********************/

/** C style comments. Actively ignore.
 @implements ScannerFn
 @allow */
static enum Token scan_comment(struct Scanner *const s) {
	assert(s && state_look(s) == COMMENT);
comment:
}


T a(int (*ptr)(int a, int (*fn)(void))) /** yo */ {
	/** @param[a, fn] Doesn't do anything `x`. */
	a = a;
}
