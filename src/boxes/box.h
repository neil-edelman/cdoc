/* Multiplex shared code. Pre-processor commands can't be a part of a macro.

 This used to be in every file. Each file was an independent project. Which was
 appealing for inclusion of one box. The rational for this file is it's
 repeated code and difficult in development of boxes to keep updated. Changing
 it back for convenience is also valid. */

#if defined(BOX_NONE) + defined(BOX_ALL) \
	+ defined(BOX_START) + defined(BOX_END) \
	+ defined(BOX_NAME_MISSING) + defined(BOX_NAME_PRESENT) != 1
#	error Request one.
#endif

#ifdef BOX_NONE
#	undef BOX_NONE
#	if defined(BOX_CAT_) || defined(BOX_CAT) \
		|| defined(t_) || defined(T_) || defined(pT_) \
		|| defined(tr_) || defined(TR_) || defined(pTR_) \
		/* We know that these are not defined outside, though they can be
		 undefined inside. */ \
		|| defined(BOX_RESTRICT) || defined(BOX_ENTRY1) || defined(BOX_ENTRY2)
#		error Unexpected preprocessor symbols.
#	endif
#endif

#ifdef BOX_ALL
#	undef BOX_ALL
#	if !defined(BOX_CAT_) || !defined(BOX_CAT) \
		|| !defined(t_) || !defined(T_) || !defined(pT_) \
		|| !defined(tr_) || !defined(TR_) || !defined(pTR_) \
		|| !defined(BOX_ENTRY1)
#		error Missing preprocessor symbols.
#	endif
#endif

#ifdef BOX_NAME_MISSING
#	undef BOX_NAME_MISSING
#	if defined(BOX_MINOR) || defined(BOX_MAJOR)
#		error Unexpected box name.
#	endif
#endif

#ifdef BOX_NAME_PRESENT
#	undef BOX_NAME_PRESENT
#	if !defined(BOX_MINOR) || !defined(BOX_MAJOR)
#		error Missing box name.
#	endif
#endif

#ifdef BOX_START
#	undef BOX_START
/* This is C++17. One may need to define this manually. */
#	if defined(__has_include) && !defined(HAS_ITERATE_H)
#		if __has_include("iterate.h")
#			define HAVE_ITERATE_H
#		endif
#	endif
#	ifdef BOX_ENTRY1
#		ifdef BOX_ENTRY2
#			error Three levels of recursion is not supported (yet?)
#		endif
#		define BOX_ENTRY2
#		define BOX_ALL
#		include "box.h"
#	else
#		define BOX_NONE
#		include "box.h"
#		define BOX_ENTRY1
/* <Kernighan and Ritchie, 1988, p. 231>. Allow two levels for containers
 including sub-containers—this is not real recursion. */
#		define BOX_CAT_(n, m) n ## _ ## m
#		define BOX_CAT(n, m) BOX_CAT_(n, m)
#		define t_(n) BOX_CAT(BOX_MINOR, n)
#		define T_(n) t_(BOX_CAT(BOX_MAJOR, n))
#		define pT_(n) BOX_CAT(private, T_(n))
#		ifdef BOX_TRAIT
#			define tr_(n) t_(BOX_CAT(BOX_TRAIT, n))
#			define TR_(n) T_(BOX_CAT(BOX_TRAIT, n))
#			define T_R_(n, m) t_(BOX_CAT(n, BOX_CAT(BOX_TRAIT, m)))
#			define pTR_(n) pT_(BOX_CAT(BOX_TRAIT, n))
#		else /* Anonymous trait. */
#			define tr_(n) t_(n)
#			define TR_(n) T_(n)
#			define T_R_(n, m) t_(BOX_CAT(n, m))
#			define pTR_(n) pT_(n)
#		endif
/* Attribute in C99+; ignore otherwise. */
#		if !defined(restrict) && (!defined(__STDC__) \
			|| !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)
#			define BOX_RESTRICT
#			define restrict
#		endif
#		define BOX_ALL
#		include "box.h"
#	endif
#endif

#ifdef BOX_END
#	undef BOX_END
#	define BOX_ALL
#	include "box.h"
#	ifdef BOX_ENTRY2
#		undef BOX_ENTRY2
#	else
#		undef BOX_ENTRY1
#		undef BOX_CAT_
#		undef BOX_CAT
#		undef t_
#		undef T_
#		undef pT_
#		undef tr_
#		undef TR_
#		undef T_R_
#		undef pTR_
#		ifdef BOX_RESTRICT
#			undef BOX_RESTRICT
#			undef restrict
#		endif
#		define BOX_NONE
#		include "box.h"
#	endif
#endif
