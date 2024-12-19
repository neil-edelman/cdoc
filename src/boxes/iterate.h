/* @license 2020 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 @subtitle Iterate trait

 Interface defined by box. Singleton. C++17 "__has_include" is used to include
 this; one may have to edit the source to include it on older compilers.

 @std C89 */

#define BOX_ALL /* Sanity check. */
#include "box.h"

#include <stddef.h>
#include <limits.h>

/** <../../src/iterate.h>: Operates by side-effects. */
typedef void (*pT_(action_fn))(pT_(type) *);
/** <../../src/iterate.h>: Returns a boolean given read-only. */
typedef int (*pT_(predicate_fn))(const pT_(type) *);

#ifdef BOX_NON_STATIC
#	define static
pT_(type) *TR_(any)(const pT_(box) *, pTR_(predicate_fn));
void TR_(each)(pT_(box) *, pTR_(action_fn));
void TR_(if_each)(pT_(box) *, pTR_(predicate_fn), pTR_(action_fn));
#	ifdef BOX_CONTIGUOUS
int TR_(copy_if)(pT_(box) *restrict,
	const pTR_(box) *restrict, pTR_(predicate_fn));
static void TR_(keep_if)(pT_(box) *, pTR_(predicate_fn), pTR_(action_fn));
static void TR_(trim)(pT_(box) *, pTR_(predicate_fn));
#	endif
#endif
#ifndef BOX_DECLARE_ONLY

/** <../../src/iterate.h>: Iterates through `box` and calls `predicate` until it
 returns true. @return The first `predicate` that returned true, or, if the
 statement is false on all, null.
 @order \O(`box.size`) \times \O(`predicate`) @allow */
static pT_(type) *TR_(any)(const pT_(box) *const box,
	const pTR_(predicate_fn) predicate) {
	union { const pT_(box) *readonly; pT_(box) *promise; } slybox;
	struct T_(cursor) it;
	assert(box && predicate);
	for(slybox.readonly = box, it = T_(begin)(slybox.promise);
		T_(exists)(&it); T_(next)(&it)) {
		pT_(type) *i = T_(look)(&it);
		if(predicate(i)) return i;
	}
	return 0;
}

/** <../../src/iterate.h>: Iterates through `box` and calls `action` on all the
 elements. Differs calling `action` until the iterator is one-ahead, so can
 delete elements as long as it doesn't affect the next, (specifically, a
 linked-list.) @order \O(|`box`|) \times \O(`action`) @allow */
static void TR_(each)(pT_(box) *const box, const pTR_(action_fn) action) {
	struct T_(cursor) it;
	assert(box && action);
	for(it = T_(begin)(box); T_(exists)(&it); T_(next)(&it))
		action(T_(look)(&it));
}

/** <../../src/iterate.h>: Iterates through `box` and calls `action` on all the
 elements for which `predicate` returns true.
 @order \O(`box.size`) \times (\O(`predicate`) + \O(`action`)) @allow */
static void TR_(if_each)(pT_(box) *const box,
	const pTR_(predicate_fn) predicate, const pTR_(action_fn) action) {
	struct T_(cursor) it;
	assert(box && predicate && action);
	/* fixme: Could I to remove `i` from the list? */
	/* 2024-11-25: it depends what container… but yes, inefficiently. */
	for(it = T_(begin)(box); T_(exists)(&it); T_(next)(&it)) {
		pT_(type) *v = T_(look)(&it);
		if(predicate(v)) action(v);
	}
}

#	ifdef BOX_CONTIGUOUS /* <!-- contiguous */

/** <../../src/iterate.h>, `BOX_CONTIGUOUS`: For all elements of `src`, calls
 `copy`, and if true, lazily copies the elements to `dst`. `dst` and `src` can
 not be the same but `src` can be null, (in which case, it does nothing.)
 @order \O(|`src`|) \times \O(`copy`) @throws[realloc] @allow */
static int TR_(copy_if)(pT_(box) *restrict const dst,
	const pTR_(box) *restrict const src, const pTR_(predicate_fn) copy) {
	pT_(type) *v, *fresh, *end, *rise = 0;
	size_t add;
	int difcpy = 0;
	assert(dst && copy && dst != src);
	if(!src) return 1;
	for(v = T_(at)(src, 0), end = v + T_(size)(src); v < end; v++) {
		/* Not falling/rising. */
		if(!(!!rise ^ (difcpy = copy(v)))) continue;
		if(difcpy) { /* Rising edge. */
			assert(!rise);
			rise = v;
		} else { /* Falling edge. */
			assert(rise && !difcpy && rise < v);
			if(!(fresh = T_(append)(dst, add = (size_t)(v - rise)))) return 0;
			memcpy(fresh, rise, sizeof *fresh * add);
			rise = 0;
		}
	}
	if(rise) { /* Delayed copy. */
		assert(!difcpy && rise < v);
		if(!(fresh = T_(append)(dst, add = (size_t)(v - rise)))) return 0;
		memcpy(fresh, rise, sizeof *fresh * add);
	}
	return 1;
}

/** <../../src/iterate.h> `BOX_CONTIGUOUS`: For all elements of `box`, calls
 `keep`, and if false, if contiguous, lazy deletes that item, if not, eagerly.
 Calls `destruct` if not-null before deleting.
 @order \O(|`box`|) (\times O(`keep`) + O(`destruct`)) @allow */
static void TR_(keep_if)(pT_(box) *const box,
	const pTR_(predicate_fn) keep, const pTR_(action_fn) destruct) {
	pT_(type) *erase = 0, *v, *retain = 0, *end;
	int keep0 = 1, keep1 = 0;
	assert(box && keep);
	for(v = T_(at)(box, 0), end = v + T_(size)(box); v < end;
		keep0 = keep1, v++) {
		if(!(keep1 = !!keep(v)) && destruct)
			destruct(v);
		if(!(keep0 ^ keep1)) continue; /* Not a falling/rising edge. */
		if(keep1) { /* Rising edge. */
			assert(erase && !retain);
			retain = v;
		} else if(erase) { /* Falling edge. */
			size_t n = (size_t)(v - retain);
			assert(erase < retain && retain < v);
			memmove(erase, retain, sizeof *v * n);
			erase += n;
			retain = 0;
		} else { /* Falling edge, (first time only.) */
			erase = v;
		}
	}
	if(!erase) return; /* All elements were kept. */
	if(keep1) { /* Delayed move when the iteration ended; repeat. */
		size_t n = (size_t)(v - retain);
		assert(retain && erase < retain && retain < v);
		memmove(erase, retain, sizeof *v * n);
		erase += n;
	}
	/* Adjust the size. */
	assert((size_t)(erase - T_(at)(box, 0)) <= T_(size)(box));
	T_(tell_size)(box, (size_t)(erase - T_(at)(box, 0)));
}

/** <../../src/iterate.h>, `BOX_CONTIGUOUS`: Removes at either end of `box` the
 things that `predicate`, if it exists, returns true.
 @order \O(`box.size`) \times \O(`predicate`) @allow */
static void TR_(trim)(pT_(box) *const box,
	const pTR_(predicate_fn) predicate) {
	size_t right, left;
	pT_(type) *first;
	assert(box);
	if(!predicate) return;
	right = T_(size)(box);
	first = T_(at)(box, 0);
	while(right && predicate(first + right - 1))
		right--;
	for(left = 0; left < right
		&& predicate(first + left); left++);
	if(right == T_(size)(box) && !left) return; /* No change. */
	assert(left <= right);
	if(left) memmove(first, first + left, sizeof *first * (right - left));
	T_(tell_size)(box, right - left);
}

#	endif /* contiguous --> */

#	ifdef static
#		undef static
#	endif

static void pTR_(unused_iterate_coda)(void);
static void pTR_(unused_function)(void) {
	TR_(any)(0, 0); TR_(each)(0, 0); TR_(if_each)(0, 0, 0);
#	ifdef BOX_CONTIGUOUS
	TR_(copy_if)(0, 0, 0); TR_(trim)(0, 0); TR_(keep_if)(0, 0, 0);
#	endif
	pTR_(unused_iterate_coda)(); }
static void pTR_(unused_iterate_coda)(void) { pTR_(unused_function)(); }

#endif
