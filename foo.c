/** Yo \url{ http://foo.org/ }

Paragraph

@title Foo
@depend C89
*/



/** A `a`. */
static T_(Array) a;

/** B */
struct T_U_(Array, Sort) b;

/** C */
int T_U_V_(Foo, Bar  ,  Baz)(void);

/** Escapes. \\ \` \@ \_ \*  /// \\\ \*/

/** Returns eof.
 @implements ScannerFn
 @allow */
static enum Token scan_eof(struct Scanner *const s) { (void)s; return END; }

/**
 * This is a
 * kernel-style comment.
 * \url{ http://   www.  `@.com }
 * \cite{Yo2019 Foo} ?<>&!
 *
 * Aha!
 */

/** C style comments. Actively ignore.
 @implements ScannerFn
 @allow */
static enum Token scan_comment(struct Scanner *const s) {
	assert(s && state_look(s) == COMMENT);
comment:
}


T a(int (*ptr)(int a, int (*fn)(void))) /** yo */ {
	/** @param{a, fn} Doesn't do anything `x`. */
	a = a;
}
