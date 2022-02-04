/** @license 2016 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 @subtitle Contiguous dynamic array

 ![Example of array.](../web/array.png)

 <tag:<A>array> is a dynamic array that stores contiguous <typedef:<PA>type>.
 Resizing may be necessary when increasing the size of the array; this incurs
 amortised cost, and any pointers to this memory may become stale.

 @param[ARRAY_NAME, ARRAY_TYPE]
 `<A>` that satisfies `C` naming conventions when mangled and a valid tag-type,
 <typedef:<PA>type>, associated therewith; required. `<PA>` is private, whose
 names are prefixed in a manner to avoid collisions.

 @param[ARRAY_CONTIGUOUS]
 Include the singleton trait contained in <contiguous.h> that takes no options.

 @param[ARRAY_MIN_CAPACITY]
 Default is 3; optional number in `[2, SIZE_MAX]` that the capacity can not go
 below.

 @param[ARRAY_EXPECT_TRAIT]
 Do not un-define certain variables for subsequent inclusion in a parameterized
 trait.

 @param[ARRAY_COMPARE_NAME, ARRAY_COMPARE, ARRAY_IS_EQUAL]
 Compare trait contained in <compare.h>. An optional mangled name for
 uniqueness and a function implementing either <typedef:<PCM>compare_fn> or
 <typedef:<PCM>bipredicate_fn>.

 @param[ARRAY_TO_STRING_NAME, ARRAY_TO_STRING]
 To string trait contained in <to_string.h>. An optional mangled name for
 uniqueness and function implementing <typedef:<PSZ>to_string_fn>.

 @std C89 */

#if !defined(ARRAY_NAME) || !defined(ARRAY_TYPE)
#error Name ARRAY_NAME or tag type ARRAY_TYPE undefined.
#endif
#if defined(ARRAY_TO_STRING_NAME) || defined(ARRAY_TO_STRING)
#define ARRAY_TO_STRING_TRAIT 1
#else
#define ARRAY_TO_STRING_TRAIT 0
#endif
#if defined(ARRAY_COMPARE_NAME) || defined(ARRAY_COMPARE) \
	|| defined(ARRAY_IS_EQUAL)
#define ARRAY_COMPARE_TRAIT 1
#else
#define ARRAY_COMPARE_TRAIT 0
#endif
#define ARRAY_TRAITS ARRAY_TO_STRING_TRAIT + ARRAY_COMPARE_TRAIT
#if ARRAY_TRAITS > 1
#error Only one trait per include is allowed; use ARRAY_EXPECT_TRAIT.
#endif
#if defined(ARRAY_TO_STRING_NAME) && !defined(ARRAY_TO_STRING)
#error ARRAY_TO_STRING_NAME requires ARRAY_TO_STRING.
#endif
#if defined(ARRAY_COMPARE_NAME) \
	&& (!(!defined(ARRAY_COMPARE) ^ !defined(ARRAY_IS_EQUAL)))
#error ARRAY_COMPARE_NAME requires ARRAY_COMPARE or ARRAY_IS_EQUAL not both.
#endif

#ifndef ARRAY_H /* <!-- idempotent */
#define ARRAY_H
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#if defined(ARRAY_CAT_) || defined(ARRAY_CAT) || defined(A_) || defined(PA_) \
	|| defined(ARRAY_IDLE)
#error Unexpected defines.
#endif
/* <Kernighan and Ritchie, 1988, p. 231>. */
#define ARRAY_CAT_(n, m) n ## _ ## m
#define ARRAY_CAT(n, m) ARRAY_CAT_(n, m)
#define A_(n) ARRAY_CAT(ARRAY_NAME, n)
#define PA_(n) ARRAY_CAT(array, A_(n))
#define ARRAY_IDLE { 0, 0, 0 }
#endif /* idempotent --> */


#if ARRAY_TRAITS == 0 /* <!-- base code */


#ifndef ARRAY_MIN_CAPACITY /* <!-- !min; */
#define ARRAY_MIN_CAPACITY 3 /* > 1 */
#endif /* !min --> */

/** A valid tag type set by `ARRAY_TYPE`. */
typedef ARRAY_TYPE PA_(type);

/** Manages the array field `data` which has `size` elements. The space is
 indexed up to `capacity`, which is at least `size`. To initialize it to an
 idle state, see <fn:<A>array>, `ARRAY_IDLE`, `{0}` (`C99`,) or being `static`.
 The fields should be treated as read-only; any modification is liable to cause
 the array to go into an invalid state.

 ![States.](../web/states.png) */
struct A_(array) { PA_(type) *data; size_t size, capacity; };
/* !data -> !size, data -> capacity >= min && size <= capacity <= max */

/** Initialises `a` to idle. @order \Theta(1) @allow */
static void A_(array)(struct A_(array) *const a)
	{ assert(a), a->data = 0, a->capacity = a->size = 0; }

/** Destroys `a` and returns it to idle. @allow */
static void A_(array_)(struct A_(array) *const a)
	{ assert(a), free(a->data), A_(array)(a); }

/** Ensures `min` capacity of `a`. Invalidates pointers in `a`. @param[min] If
 zero, does nothing. @return Success; otherwise, `errno` will be set.
 @throws[ERANGE] Tried allocating more then can fit in `size_t` or `realloc`
 doesn't follow POSIX. @throws[realloc] @allow */
static int A_(array_reserve)(struct A_(array) *const a, const size_t min) {
	size_t c0;
	PA_(type) *data;
	const size_t max_size = (size_t)-1 / sizeof *a->data;
	assert(a);
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
	while(c0 < min) {
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
 indicates an error. @throws[realloc, ERANGE] @allow */
static PA_(type) *A_(array_buffer)(struct A_(array) *const a, const size_t n) {
	assert(a);
	if(a->size > (size_t)-1 - n) { errno = ERANGE; return 0; }
	return A_(array_reserve)(a, a->size + n) && a->data ? a->data + a->size : 0;
}

/** Adds `n` elements to the back of `a`. It will invalidate pointers in `a` if
 `n` is greater than the buffer space.
 @return A pointer to the elements. If `a` is idle and `n` is zero, a null
 pointer will be returned, otherwise null indicates an error.
 @throws[realloc, ERANGE] @allow */
static PA_(type) *A_(array_append)(struct A_(array) *const a, const size_t n) {
	PA_(type) *b;
	assert(a);
	if(!(b = A_(array_buffer)(a, n))) return 0;
	assert(n <= a->capacity && a->size <= a->capacity - n);
	return a->size += n, b;
}

/** Adds `n` un-initialised elements at position `at` in `a`. The buffer holds
 enough elements or it will invalidate pointers in `a`.
 @param[at] A number smaller than or equal to `a.size`; if `a.size`, this
 function behaves as <fn:<A>array_append>.
 @return A pointer to the start of the new region, where there are `n`
 elements. @throws[realloc, ERANGE] @allow */
static PA_(type) *A_(array_insert)(struct A_(array) *const a,
	const size_t n, const size_t at) {
	const size_t old_size = a->size;
	PA_(type) *const b = A_(array_append)(a, n);
	assert(a && at <= old_size);
	if(!b) return 0;
	memmove(a->data + at + n, a->data + at, sizeof *a->data * (old_size - at));
	return a->data + at;
}

/** @return Adds (push back) one new element of `a`. The buffer holds an
 element or it will invalidate pointers in `a`.
 @order amortised \O(1) @throws[realloc, ERANGE] @allow */
static PA_(type) *A_(array_new)(struct A_(array) *const a)
	{ return A_(array_append)(a, 1); }

/** Shrinks the capacity `a` to the size, freeing unused memory. If the size is
 zero, it will be in an idle state. Invalidates pointers in `a`.
 @return Success. @throws[ERANGE, realloc] Unlikely `realloc` error. */
static int A_(array_shrink)(struct A_(array) *const a) {
	PA_(type) *data;
	size_t c;
	assert(a && a->capacity >= a->size);
	if(!a->data) return assert(!a->size && !a->capacity), 1;
	c = a->size && a->size > ARRAY_MIN_CAPACITY ? a->size : ARRAY_MIN_CAPACITY;
	if(!(data = realloc(a->data, sizeof *a->data * c)))
		{ if(!errno) errno = ERANGE; return 0; }
	a->data = data, a->capacity = c;
	return 1;
}

/** Removes `datum` from `a`. @order \O(`a.size`). @allow */
static void A_(array_remove)(struct A_(array) *const a,
	PA_(type) *const datum) {
	const size_t n = (size_t)(datum - a->data);
	assert(a && datum && datum >= a->data && datum < a->data + a->size);
	memmove(datum, datum + 1, sizeof *datum * (--a->size - n));
}

/** Removes `datum` from `a` and replaces it with the tail.
 @order \O(1). @allow */
static void A_(array_lazy_remove)(struct A_(array) *const a,
	PA_(type) *const datum) {
	size_t n = (size_t)(datum - a->data);
	assert(a && datum && datum >= a->data && datum < a->data + a->size);
	if(--a->size != n) memcpy(datum, a->data + a->size, sizeof *datum);
}

/** Sets `a` to be empty. That is, the size of `a` will be zero, but if it was
 previously in an active non-idle state, it continues to be.
 @order \Theta(1) @allow */
static void A_(array_clear)(struct A_(array) *const a)
	{ assert(a), a->size = 0; }

/** @return The last element or null if `a` is empty. @order \Theta(1) @allow */
static PA_(type) *A_(array_peek)(const struct A_(array) *const a)
	{ return assert(a), a->size ? a->data + a->size - 1 : 0; }

/** @return Value from the the top of `a` that is removed or null if the array
 is empty. @order \Theta(1) @allow */
static PA_(type) *A_(array_pop)(struct A_(array) *const a)
	{ return assert(a), a->size ? a->data + --a->size : 0; }

/** `a` indices [`i0`, `i1`) will be replaced with a copy of `b`.
 @param[b] Can be null, which acts as empty.
 @return Success. @throws[realloc, ERANGE] */
static int A_(array_splice)(struct A_(array) *const a, const size_t i0,
	const size_t i1, const struct A_(array) *const b) {
	const size_t a_range = i1 - i0, b_range = b ? b->size : 0;
	assert(a && a != b && i0 <= i1 && i1 <= a->size);
	if(a_range < b_range) { /* The output is bigger. */
		const size_t diff = b_range - a_range;
		if(!A_(array_buffer)(a, diff)) return 0;
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

/** Copies `b`, which can be null, to the back of `a`.
 @return Success. @throws[realloc, ERANGE]
 @fixme Untested. */
static int A_(array_affix)(struct A_(array) *const a,
	const struct A_(array) *const b)
	{ return A_(array_splice)(a, a->size, a->size, b); }

/** Appends `n` items on the back of `a`.
 @fixme It should be the other way around. */
static PA_(type) *PA_(append)(struct A_(array) *const a, const size_t n)
	{ return A_(array_append)(a, n); }

/* <!-- iterate interface */

/* Contains all iteration parameters. */
struct PA_(iterator) { const struct A_(array) *a; size_t i; };

/** Loads `a` into `it`. @implements begin */
static void PA_(begin)(struct PA_(iterator) *const it,
	const struct A_(array) *const a) { assert(it && a), it->a = a, it->i = 0; }

/** Advances `it`. @implements next */
static PA_(type) *PA_(next)(struct PA_(iterator) *const it) {
	return assert(it && it->a), it->i < it->a->size ? it->a->data + it->i++ : 0;
}

/* iterate --> */

/* <!-- box (multiple traits) */
#define BOX_ PA_
#define BOX_CONTAINER struct A_(array)
#define BOX_CONTENTS PA_(type)

#ifdef ARRAY_CONTIGUOUS /* <!-- contiguous */
#define CG_(n) ARRAY_CAT(A_(array), n)
#include "contiguous.h" /** \include */
#endif /* contiguous --> */

#ifdef ARRAY_TEST /* <!-- test */
/* Forward-declare. */
static void (*PA_(to_string))(const PA_(type) *, char (*)[12]);
static const char *(*PA_(array_to_string))(const struct A_(array) *);
#include "../test/test_array.h" /* (this will needlessly confuse) \include */
#endif /* test --> */

static void PA_(unused_base_coda)(void);
static void PA_(unused_base)(void) {
	A_(array_)(0); A_(array_insert)(0, 0, 0); A_(array_new)(0);
	A_(array_shrink)(0); A_(array_remove)(0, 0); A_(array_lazy_remove)(0, 0);
	A_(array_clear)(0); A_(array_peek)(0); A_(array_pop)(0);
	A_(array_splice)(0, 0, 0, 0); A_(array_affix)(0, 0); PA_(begin)(0, 0);
	PA_(next)(0); PA_(append)(0, 0);
	PA_(unused_base_coda)();
}
static void PA_(unused_base_coda)(void) { PA_(unused_base)(); }


#elif defined(ARRAY_TO_STRING) /* base code --><!-- to string trait */


#ifdef ARRAY_TO_STRING_NAME
#define SZ_(n) ARRAY_CAT(A_(array), ARRAY_CAT(ARRAY_TO_STRING_NAME, n))
#else
#define SZ_(n) ARRAY_CAT(A_(array), n)
#endif
#define TO_STRING ARRAY_TO_STRING
#include "to_string.h" /** \include */
#ifdef ARRAY_TEST /* <!-- expect: greedy satisfy forward-declared. */
#undef ARRAY_TEST
static PSZ_(to_string_fn) PA_(to_string) = PSZ_(to_string);
static const char *(*PA_(array_to_string))(const struct A_(array) *)
	= &SZ_(to_string);
#endif /* expect --> */
#undef SZ_
#undef ARRAY_TO_STRING
#ifdef ARRAY_TO_STRING_NAME
#undef ARRAY_TO_STRING_NAME
#endif


#else /* to string trait --><!-- compare trait */


#ifdef ARRAY_COMPARE_NAME /* <!-- name */
#define CM_(n) ARRAY_CAT(A_(array), ARRAY_CAT(ARRAY_COMPARE_NAME, n))
#else /* name --><!-- !name */
#define CM_(n) ARRAY_CAT(A_(array), n)
#endif /* !name --> */
#ifdef ARRAY_COMPARE /* <!-- cmp */
#define BOX_COMPARE ARRAY_COMPARE
#else /* cmp --><!-- eq */
#define BOX_IS_EQUAL ARRAY_IS_EQUAL
#endif /* eq --> */
#include "compare.h" /** \include */
#ifdef ARRAY_TEST /* <!-- test: this detects and outputs compare test. */
#include "../test/test_array.h"
#endif /* test --> */
#undef CM_
#ifdef ARRAY_COMPARE_NAME
#undef ARRAY_COMPARE_NAME
#endif
#ifdef ARRAY_COMPARE
#undef ARRAY_COMPARE
#endif
#ifdef ARRAY_IS_EQUAL
#undef ARRAY_IS_EQUAL
#endif


#endif /* traits --> */


#ifdef ARRAY_EXPECT_TRAIT /* <!-- trait */
#undef ARRAY_EXPECT_TRAIT
#else /* trait --><!-- !trait */
#ifdef ARRAY_TEST
#error No ARRAY_TO_STRING traits defined for ARRAY_TEST.
#endif
#undef ARRAY_NAME
#undef ARRAY_TYPE
#undef BOX_
#undef BOX_CONTAINER
#undef BOX_CONTENTS
/* box (multiple traits) --> */
#endif /* !trait --> */
#undef ARRAY_TO_STRING_TRAIT
#undef ARRAY_COMPARE_TRAIT
#undef ARRAY_TRAITS
