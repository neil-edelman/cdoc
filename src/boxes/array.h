/** @license 2016 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 @abstract Header <../../src/array.h>; examples <../../test/test_array.c>.

 @subtitle Contiguous dynamic array

 ![Example of array.](../doc/array/array.png)

 <tag:<t>array> is a dynamic array that stores contiguous <typedef:<pT>type>.
 Resizing may be necessary when increasing the size of the array; this incurs
 amortised cost. As such, the contents are not stable.

 @param[ARRAY_NAME, ARRAY_TYPE]
 `<t>` that satisfies `C` naming conventions when mangled and a valid tag-type,
 <typedef:<pT>type>, associated therewith; required.

 @param[ARRAY_COMPARE, ARRAY_IS_EQUAL]
 Compare trait contained in <../../src/compare.h>. See <typedef:<pT>compare_fn>
 or <typedef:<pT>bipredicate_fn>, (but not both.)

 @param[ARRAY_TO_STRING]
 To string trait contained in <../../src/to_string.h>. See
 <typedef:<pT>to_string_fn>.

 @param[ARRAY_EXPECT_TRAIT, ARRAY_TRAIT]
 Named traits are obtained by including `array.h` multiple times with
 `ARRAY_EXPECT_TRAIT` and then subsequently including the name in `ARRAY_TRAIT`.

 @param[ARRAY_DECLARE_ONLY, ARRAY_NON_STATIC]
 For headers in different compilation units.

 @depend [box](../../src/box.h)
 @std C89 */

#if !defined ARRAY_NAME || !defined ARRAY_TYPE
#	error Name or tag type undefined.
#endif
#if !defined BOX_ENTRY1 && (defined ARRAY_TRAIT ^ defined BOX_MAJOR)
#	error Trait name must come after expect trait.
#endif
#if defined ARRAY_COMPARE && defined ARRAY_IS_EQUAL
#	error Only one can be defined at a time.
#endif
#if defined ARRAY_TEST && (!defined ARRAY_TRAIT && !defined ARRAY_TO_STRING \
	|| defined ARRAY_TRAIT && !defined ARRAY_HAS_TO_STRING \
	|| !defined HAS_GRAPH_H)
#	error Test requires to string and graph.
#endif
#if defined BOX_TRAIT && !defined ARRAY_TRAIT
#	error Unexpected flow.
#endif

#ifdef ARRAY_TRAIT
#	define BOX_TRAIT ARRAY_TRAIT /* Ifdef in <box.h>. */
#endif
#ifdef ARRAY_NON_STATIC
#	define BOX_NON_STATIC
#endif
#ifdef ARRAY_DECLARE_ONLY
#	define BOX_DECLARE_ONLY
#endif
#define BOX_START
#include "box.h"

#ifndef ARRAY_TRAIT /* Base code, necessarily first. */
#	include <stdlib.h>
#	include <string.h>
#	include <errno.h>
#	include <assert.h>

#	define BOX_MINOR ARRAY_NAME
#	define BOX_MAJOR array
#	define BOX_ACCESS /* fixme: Have all the functions individually. */
#	define BOX_CONTIGUOUS

#	ifndef ARRAY_MIN_CAPACITY
#		define ARRAY_MIN_CAPACITY 3 /* > 1 */
#	endif
#	if ARRAY_MIN_CAPACITY <= 1
#		error ARRAY_MIN_CAPACITY > 1
#	endif

/** A valid tag type set by `ARRAY_TYPE`. */
typedef ARRAY_TYPE pT_(type);

/** Manages the array field `data` which has `size` elements. The space is
 indexed up to `capacity`, which is at least `size`.

 ![States.](../doc/array/states.png) */
struct t_(array) { size_t size, capacity; pT_(type) *data; };
typedef struct t_(array) pT_(box);
/* !data -> !size, data -> capacity >= min && size <= capacity <= max */
struct T_(cursor) { struct t_(array) *a; size_t i; };

#	ifdef ARRAY_NON_STATIC /* Public functions. */
#		define static
struct T_(cursor) T_(begin)(const struct t_(array) *);
struct T_(cursor) T_(at)(const struct t_(array) *, size_t);
int T_(exists)(const struct T_(cursor) *);
pT_(type) *T_(entry)(struct T_(cursor) *);
void T_(next)(struct T_(cursor) *);
size_t T_(size)(const struct t_(array) *);
pT_(type) *T_(data_at)(const struct t_(array) *, size_t);
void T_(tell_size)(struct t_(array) *, size_t);
struct t_(array) t_(array)(void);
void t_(array_)(struct t_(array) *const);
int T_(reserve)(struct t_(array) *const, size_t);
pT_(type) *T_(buffer)(struct t_(array) *, size_t);
pT_(type) *T_(append)(struct t_(array) *, size_t);
pT_(type) *T_(insert)(struct t_(array) *, size_t, size_t);
pT_(type) *T_(new)(struct t_(array) *);
int T_(shrink)(struct t_(array) *);
void T_(remove)(struct t_(array) *, pT_(type) *);
void T_(lazy_remove)(struct t_(array) *const a, pT_(type) *);
void T_(clear)(struct t_(array) *);
pT_(type) *T_(peek)(const struct t_(array) *);
pT_(type) *T_(pop)(struct t_(array) *const a);
int T_(splice)(struct t_(array) *restrict, const struct t_(array) *restrict,
	size_t, size_t);
#	endif
#	ifndef ARRAY_DECLARE_ONLY /* Produce code. */

/** @return A cursor at the beginning of a valid `a`. */
static struct T_(cursor) T_(begin)(const struct t_(array) *const a) {
	/* Instead of discouraging the use of non-`const` data, we accept `const`
	 but store it in the same structure. The user will be responsible for
	 `const` at lookup. */
	union { const struct t_(array) *readonly; struct t_(array) *promise; } sly;
	struct T_(cursor) cur;
	cur.a = (sly.readonly = a, sly.promise), cur.i = 0;
	return cur;
}
/** @return A cursor in `a` at index `i`. */
static struct T_(cursor) T_(at)(const struct t_(array) *const a,
	const size_t i) {
	union { const struct t_(array) *readonly; struct t_(array) *promise; } sly;
	struct T_(cursor) cur;
	cur.a = (sly.readonly = a, sly.promise), cur.i = i;
	return cur;
}
/** @return Whether `cur` points to a valid entry. */
static int T_(exists)(const struct T_(cursor) *const cur)
	{ return cur && cur->a && cur->a->data && cur->i < cur->a->size; }
/** @return Pointer to a valid entry at `cur`. */
static pT_(type) *T_(entry)(struct T_(cursor) *const cur)
	{ return cur->a->data + cur->i; }
/** Move to the next of a valid `cur`. */
static void T_(next)(struct T_(cursor) *const cur)
	{ if(cur->i == (size_t)~0) cur->a = 0; else cur->i++; }

/* fixme: This is satisfying `compare.h` contracts; probably should re-think
 those. */
/** Size of `a`. @implements `size`. */
static size_t T_(size)(const struct t_(array) *const a) { return a->size; }
/** @return Element `idx` of `a`. @implements `at` */
static pT_(type) *T_(data_at)(const struct t_(array) *const a, const size_t idx)
	{ return a->data + idx; }
/** Writes `size` to `a`. @implements `tell_size` */
static void T_(tell_size)(struct t_(array) *a, const size_t size)
	{ a->size = size; }

/** Zeroed data (not all-bits-zero) is initialized, as well.
 @return An idle array. @order \Theta(1) @allow */
static struct t_(array) t_(array)(void)
	{ struct t_(array) a; a.data = 0, a.capacity = a.size = 0; return a; }

/** If `a` is not null, returns the idle zeroed state where it takes no dynamic
 memory. @order \Theta(1) @allow */
static void t_(array_)(struct t_(array) *const a)
	{ if(a) free(a->data), *a = t_(array)(); }

/** Ensures `min` capacity of `a`. Invalidates pointers in `a`. @param[min] If
 zero, does nothing. @return Success; otherwise, `errno` will be set.
 @throws[ERANGE] Tried allocating more then can fit in `size_t` or `realloc`
 doesn't follow POSIX. @throws[realloc] @allow */
static int T_(reserve)(struct t_(array) *const a, const size_t min) {
	size_t c0;
	pT_(type) *data;
	const size_t max_size = (size_t)~0 / sizeof *a->data;
	if(a->data) {
		assert(a->size <= a->capacity);
		if(min <= a->capacity) return 1;
		c0 = a->capacity < ARRAY_MIN_CAPACITY
			? ARRAY_MIN_CAPACITY : a->capacity;
	} else { /* Idle. */
		assert(!a->size && !a->capacity);
		if(!min) return 1;
		c0 = ARRAY_MIN_CAPACITY;
	}
	if(min > max_size) return errno = ERANGE, 0;
	/* `c_n = a1.625^n`, approximation golden ratio `\phi ~ 1.618`. */
	while(c0 < min) { /* \O(\log min), in practice, negligible. */
		size_t c1 = c0 + (c0 >> 1) + (c0 >> 3);
		if(c0 >= c1) { c0 = max_size; break; } /* Unlikely. */
		c0 = c1;
	}
	if(!(data = realloc(a->data, sizeof *a->data * c0)))
		{ if(!errno) errno = ERANGE; return 0; }
	a->data = data, a->capacity = c0;
	return 1;
}

/** The capacity of `a` will be increased to at least `n` elements beyond the
 size. Invalidates any pointers in `a`.
 @return The start of the buffered space at the back of the array. If `a` is
 idle and `buffer` is zero, a null pointer is returned, otherwise null
 indicates an error. @throws[realloc] @allow */
static pT_(type) *T_(buffer)(struct t_(array) *const a, const size_t n) {
	if(a->size > (size_t)~0 - n) { errno = ERANGE; return 0; }
	return T_(reserve)(a, a->size + n) && a->data ? a->data + a->size : 0;
}

/** Sets `a` to be empty. That is, the size of `a` will be zero, but if it was
 previously in an active non-idle state, it continues to be.
 @order \Theta(1) @allow */
static void T_(clear)(struct t_(array) *const a)
	{ assert(a), a->size = 0; }

/** Adds `n` elements to the back of `a`. It will invalidate pointers in `a` if
 `n` is greater than the buffer space.
 @implements `append` from `BOX_CONTIGUOUS`
 @return A pointer to the elements. If `a` is idle and `n` is zero, a null
 pointer will be returned, otherwise null indicates an error.
 @throws[realloc, ERANGE] @allow */
static pT_(type) *T_(append)(struct t_(array) *const a, const size_t n) {
	pT_(type) *b;
	if(!(b = T_(buffer)(a, n))) return 0;
	assert(n <= a->capacity && a->size <= a->capacity - n);
	return a->size += n, b;
}

/** Adds `n` un-initialised elements at position `at` in `a`. It will
 invalidate any pointers in `a` if the buffer holds too few elements.
 @param[at] A number smaller than or equal to `a.size`; if `a.size`, this
 function behaves as <fn:<T>append>.
 @return A pointer to the start of the new region, where there are `n`
 elements. @throws[realloc, ERANGE] @allow */
static pT_(type) *T_(insert)(struct t_(array) *const a,
	const size_t n, const size_t at) {
	/* Investigate `n` is better than `element`; all the other are element. But
	 also, when would I ever use this? */
	const size_t old_size = a->size;
	pT_(type) *const b = T_(append)(a, n);
	assert(a && at <= old_size);
	if(!b) return 0;
	memmove(a->data + at + n, a->data + at, sizeof *a->data * (old_size - at));
	return a->data + at;
}
/* fixme: All of this is stupid—why would I need it? */

/** @return Adds (push back) one new element of `a`. The buffer space holds at
 least one element, or it may invalidate pointers in `a`.
 @order amortised \O(1) @throws[realloc, ERANGE] @allow */
static pT_(type) *T_(new)(struct t_(array) *const a)
	{ return T_(append)(a, 1); }

/** Shrinks the capacity `a` to the size, freeing unused memory. If the size is
 zero, it will be in an idle state. Invalidates pointers in `a`.
 @return Success. @throws[ERANGE, realloc] (Unlikely) `realloc` error. */
static int T_(shrink)(struct t_(array) *const a) {
	pT_(type) *data;
	size_t c;
	assert(a && a->capacity >= a->size);
	if(!a->data) return assert(!a->size && !a->capacity), 1;
	c = a->size && a->size > ARRAY_MIN_CAPACITY ? a->size : ARRAY_MIN_CAPACITY;
	if(!(data = realloc(a->data, sizeof *a->data * c)))
		{ if(!errno) errno = ERANGE; return 0; }
	a->data = data, a->capacity = c;
	return 1;
}

/** Removes `element` from `a`. Do not attempt to remove an element that is not
 in `a`. @order \O(`a.size`). @allow */
static void T_(remove)(struct t_(array) *const a,
	pT_(type) *const element) {
	const size_t n = (size_t)(element - a->data);
	assert(a && element && element >= a->data && element < a->data + a->size);
	memmove(element, element + 1, sizeof *element * (--a->size - n));
}

/** Removes `datum` from `a` and replaces it with the tail. Do not attempt to
 remove an element that is not in `a`. @order \O(1). @allow */
static void T_(lazy_remove)(struct t_(array) *const a,
	pT_(type) *const datum) {
	size_t n = (size_t)(datum - a->data);
	assert(a && datum && datum >= a->data && datum < a->data + a->size);
	if(--a->size != n) memcpy(datum, a->data + a->size, sizeof *datum);
}

/** @return The last element or null if `a` is empty. @order \Theta(1) @allow */
static pT_(type) *T_(peek)(const struct t_(array) *const a)
	{ return assert(a), a->size ? a->data + a->size - 1 : 0; }

/** @return Value from the the top of `a` that is removed or null if the array
 is empty. @order \Theta(1) @allow */
static pT_(type) *T_(pop)(struct t_(array) *const a)
	{ return assert(a), a->size ? a->data + --a->size : 0; }

/** Indices [`i0`, `i1`) of `a` will be replaced with a copy of `b`.
 @param[b] Can be null, which acts as empty, but cannot overlap with `a`.
 @return Success. @throws[realloc, ERANGE] @allow */
static int T_(splice)(struct t_(array) *restrict const a,
	const struct t_(array) *restrict const b,
	const size_t i0, const size_t i1) {
	const size_t a_range = i1 - i0, b_range = b ? b->size : 0;
	assert(a && a != b && i0 <= i1 && i1 <= a->size);
	if(a_range < b_range) { /* The output is bigger. */
		const size_t diff = b_range - a_range;
		if(!T_(buffer)(a, diff)) return 0;
		memmove(a->data + i1 + diff, a->data + i1,
			(a->size - i1) * sizeof *a->data);
		a->size += diff;
	} else if(b_range < a_range) { /* The output is smaller. */
		memmove(a->data + i0 + b_range, a->data + i1,
			(a->size - i1) * sizeof *a->data);
		a->size -= a_range - b_range;
	}
	if(b) memcpy(a->data + i0, b->data, b->size * sizeof *a->data);
	return 1;
}

#		ifdef static /* Private functions. */
#			undef static
#		endif
static void pT_(unused_base_coda)(void);
static void pT_(unused_base)(void) {
	T_(begin)(0); T_(at)(0, 0); T_(exists)(0); T_(entry)(0); T_(next)(0);
	T_(size)(0); T_(data_at)(0, 0); T_(tell_size)(0, 0);
	t_(array)(); t_(array_)(0); T_(insert)(0, 0, 0); T_(new)(0); T_(shrink)(0);
	T_(remove)(0, 0); T_(lazy_remove)(0, 0); T_(clear)(0); T_(peek)(0);
	T_(pop)(0); T_(append)(0, 0); T_(splice)(0, 0, 0, 0);
	pT_(unused_base_coda)();
}
static void pT_(unused_base_coda)(void) { pT_(unused_base)(); }
#	endif /* Produce code. */
#	ifdef static /* Private functions. */
#		undef static
#	endif
#endif /* Base code. */

#if defined HAS_ITERATE_H && !defined ARRAY_TRAIT
#	include "iterate.h" /** \include */
#endif

#ifdef ARRAY_TO_STRING
#	undef ARRAY_TO_STRING
#	ifndef ARRAY_DECLARE_ONLY
#		ifndef ARRAY_TRAIT
/** The type of the required `<tr>to_string`. Responsible for turning the
 read-only argument into a 12-max-`char` output string. */
typedef void (*pT_(to_string_fn))(const pT_(type) *, char (*)[12]);
#		endif
/** Thunk(`cur`, `a`). One must implement `<tr>to_string`. */
static void pTR_(to_string)(const struct T_(cursor) *const cur,
	char (*const a)[12])
	{ tr_(to_string)((const void *)&cur->a->data[cur->i], a); }
#	endif
#	include "to_string.h" /** \include */
#	ifndef ARRAY_TRAIT
#		define ARRAY_HAS_TO_STRING /* Warning about tests. */
#	endif
#endif

#if defined HAS_GRAPH_H && defined ARRAY_HAS_TO_STRING && !defined ARRAY_TRAIT
#	include "graph.h" /** \include */
#endif

#if defined ARRAY_TEST && !defined ARRAY_TRAIT
#	include "../test/test_array.h"
#endif

#if defined ARRAY_COMPARE || defined ARRAY_IS_EQUAL
#	ifdef ARRAY_COMPARE
#		define COMPARE ARRAY_COMPARE
#	else
#		define COMPARE_IS_EQUAL ARRAY_IS_EQUAL
#	endif
#	include "compare.h" /** \include */
#	ifdef ARRAY_TEST
#		include "../test/test_array_compare.h"
#	endif
#	ifdef ARRAY_COMPARE
#		undef ARRAY_COMPARE
#	else
#		undef ARRAY_IS_EQUAL
#	endif
#endif


#ifdef ARRAY_EXPECT_TRAIT
#	undef ARRAY_EXPECT_TRAIT
#else
#	undef BOX_MINOR
#	undef BOX_MAJOR
#	undef BOX_ACCESS
#	undef BOX_CONTIGUOUS
#	undef ARRAY_NAME
#	undef ARRAY_TYPE
#	undef ARRAY_MIN_CAPACITY
#	ifdef ARRAY_HAS_TO_STRING
#		undef ARRAY_HAS_TO_STRING
#	endif
#	ifdef ARRAY_TEST
#		undef ARRAY_TEST
#	endif
#	ifdef ARRAY_DECLARE_ONLY
#		undef BOX_DECLARE_ONLY
#		undef ARRAY_DECLARE_ONLY
#	endif
#	ifdef ARRAY_NON_STATIC
#		undef BOX_NON_STATIC
#		undef ARRAY_NON_STATIC
#	endif
#	ifdef COMPARE_H
#		undef COMPARE_H /* More comparisons for later boxes. */
#	endif
#endif
#ifdef ARRAY_TRAIT
#	undef ARRAY_TRAIT
#	undef BOX_TRAIT
#endif
#define BOX_END
#include "box.h"
