/* @license 2020 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 @subtitle Contiguous trait

 Only one should be included per box. Requires contiguous data elements are
 stored in array `data` up to `size_t size` such that `memcpy` will work.

 @param[CG_]
 A one-argument macro producing a name that is responsible for the name of the
 functions. Should be something like `CG_(x) -> foo_widget_x`. The caller is
 responsible for undefining `CG_`.

 @std C89 */

#if !defined(BOX_) || !defined(BOX_CONTAINER) || !defined(BOX_CONTENTS) \
	|| !defined(CG_)
#error Unexpected preprocessor symbols.
#endif

#ifndef CONTIGUOUS_H /* <!-- idempotent */
#define CONTIGUOUS_H
#include <limits.h>
#if defined(CONTIGUOUS_CAT_) || defined(CONTIGUOUS_CAT) || defined(PCG_)
#error Unexpected defines.
#endif
/* <Kernighan and Ritchie, 1988, p. 231>. */
#define CONTIGUOUS_CAT_(n, m) n ## _ ## m
#define CONTIGUOUS_CAT(n, m) CONTIGUOUS_CAT_(n, m)
#define PCG_(n) CONTIGUOUS_CAT(contiguous, CG_(n))
#endif /* idempotent --> */

/** <contiguous.h>: an alias to the box. */
typedef BOX_CONTAINER PCG_(box);

/** <contiguous.h>: an alias to the individual type contained in the box. */
typedef BOX_CONTENTS PCG_(type);

/** Operates by side-effects on <typedef:<PCG>type>. */
typedef void (*PCG_(action_fn))(PCG_(type) *);

/** Returns a boolean given read-only <typedef:<PCG>type>. */
typedef int (*PCG_(predicate_fn))(const PCG_(type) *);

/** @param[x] A valid entry or null to start from the last.
 @return The previous valid entry from `box` or null if this was the first.
 @allow */
static PCG_(type) *CG_(previous)(const PCG_(box) *const box,
	const PCG_(type) *const x) {
	size_t i;
	assert(box);
	if(!box->data) return 0;
	if(!x) return box->size ? box->data + box->size - 1 : 0;
	return (i = (size_t)(x - box->data)) ? box->data + i - 1 : 0;
}

/** @param[x] A valid entry or null to start from the first.
 @return The next valid entry from `box` or null if this was the last. @allow */
static PCG_(type) *CG_(next)(const PCG_(box) *const box,
	const PCG_(type) *const x) {
	size_t i;
	assert(box);
	if(!box->data) return 0;
	if(!x) return box->size ? box->data + 0 : 0;
	return (i = (size_t)(x - box->data) + 1) < box->size ? box->data + i : 0;
}

/** @return Converts `i` to an index in <typedef:<PCG>box> `box` from
 [0, `a.size`]. Negative values are implicitly plus `box.size`.
 @order \Theta(1) @allow */
static size_t CG_(clip)(const PCG_(box) *const box, const long i) {
	/* `SIZE_MAX` is `C99`. */
	assert(box && ~((size_t)0) >= (size_t)LONG_MAX
		&& (unsigned long)((size_t)-1) >= LONG_MAX);
	return i < 0
		? (size_t)-i >= box->size ? 0 : box->size - (size_t)-i
		: (size_t)i > box->size ? box->size : (size_t)i;
}

/** For all elements of <typedef:<PCG>box> `b`, calls `copy`, and if true,
 lazily copies the elements to `a`. `a` and `b` can not be the same but `b` can
 be null, (in which case, it does nothing.)
 @order \O(`b.size` \times `copy`) @throws[ERANGE, realloc] @allow */
static int CG_(copy_if)(PCG_(box) *const a, const PCG_(predicate_fn) copy,
	const PCG_(box) *const b) {
	PCG_(type) *i, *fresh;
	const PCG_(type) *end, *rise = 0;
	size_t add;
	int difcpy = 0;
	assert(a && copy && a != b);
	if(!b) return 1;
	for(i = b->data, end = i + b->size; i < end; i++) {
		if(!(!!rise ^ (difcpy = copy(i)))) continue; /* Not falling/rising. */
		if(difcpy) { /* Rising edge. */
			assert(!rise);
			rise = i;
		} else { /* Falling edge. */
			assert(rise && !difcpy && rise < i);
			if(!(fresh = BOX_(append)(a, add = (size_t)(i - rise)))) return 0;
			memcpy(fresh, rise, sizeof *fresh * add);
			rise = 0;
		}
	}
	if(rise) { /* Delayed copy. */
		assert(!difcpy && rise < i);
		if(!(fresh = BOX_(append)(a, add = (size_t)(i - rise)))) return 0;
		memcpy(fresh, rise, sizeof *fresh * add);
	}
	return 1;
}

/** For all elements of <typedef:<PCG>box> `box`, calls `keep`, and if false,
 lazy deletes that item, calling `destruct` (if not-null).
 @order \O(`a.size` \times `keep` \times `destruct`) @allow */
static void CG_(keep_if)(PCG_(box) *const box,
	const PCG_(predicate_fn) keep, const PCG_(action_fn) destruct) {
	PCG_(type) *erase = 0, *t;
	const PCG_(type) *retain = 0, *end;
	int keep0 = 1, keep1 = 0;
	assert(box && keep);
	for(t = box->data, end = t + box->size; t < end; keep0 = keep1, t++) {
		if(!(keep1 = !!keep(t)) && destruct) destruct(t);
		if(!(keep0 ^ keep1)) continue; /* Not a falling/rising edge. */
		if(keep1) { /* Rising edge. */
			assert(erase && !retain);
			retain = t;
		} else if(erase) { /* Falling edge. */
			size_t n = (size_t)(t - retain);
			assert(erase < retain && retain < t);
			memmove(erase, retain, sizeof *t * n);
			erase += n;
			retain = 0;
		} else { /* Falling edge, (first time only.) */
			erase = t;
		}
	}
	if(!erase) return; /* All elements were kept. */
	if(keep1) { /* Delayed move when the iteration ended; repeat. */
		size_t n = (size_t)(t - retain);
		assert(retain && erase < retain && retain < t);
		memmove(erase, retain, sizeof *t * n);
		erase += n;
	}
	/* Adjust the size. */
	assert((size_t)(erase - box->data) <= box->size);
	box->size = (size_t)(erase - box->data);
}

/** Removes at either end of <typedef:<PCG>box> `box` of things that
 `predicate` returns true. @order \O(`box.size` \times `predicate`) @allow */
static void CG_(trim)(PCG_(box) *const box,
	const PCG_(predicate_fn) predicate) {
	size_t i;
	assert(box && predicate);
	while(box->size && predicate(box->data + box->size - 1)) box->size--;
	for(i = 0; i < box->size && predicate(box->data + i); i++);
	if(!i) return;
	assert(i < box->size);
	memmove(box->data, box->data + i, sizeof *box->data * i), box->size -= i;
}

/** Iterates through <typedef:<PCG>box> `box` and calls `action` on all the
 elements. The topology of the list should not change while in this function.
 @order \O(`box.size` \times `action`) @allow */
static void CG_(each)(PCG_(box) *const box, const PCG_(action_fn) action) {
	PCG_(type) *i, *end;
	assert(box && action);
	for(i = box->data, end = i + box->size; i < end; i++) action(i);
}

/** Iterates through <typedef:<PCG>box> `box` and calls `action` on all the
 elements for which `predicate` returns true. The topology of the list should
 not change while in this function.
 @order \O(`box.size` \times `predicate` \times `action`) @allow */
static void CG_(if_each)(PCG_(box) *const box,
	const PCG_(predicate_fn) predicate, const PCG_(action_fn) action) {
	PCG_(type) *i, *end;
	assert(box && predicate && action);
	for(i = box->data, end = i + box->size; i < end; i++)
		if(predicate(i)) action(i);
}

/** Iterates through <typedef:<PCG>box> `box` and calls `predicate` until it
 returns true.
 @return The first `predicate` that returned true, or, if the statement is
 false on all, null. @order \O(`box.size` \times `predicate`) @allow */
static const PCG_(type) *CG_(any)(const PCG_(box) *const box,
	const PCG_(predicate_fn) predicate) {
	PCG_(type) *i, *end;
	assert(box && predicate);
	for(i = box->data, end = i + box->size; i < end; i++)
		if(predicate(i)) return i;
	return 0;
}

static void PCG_(unused_function_coda)(void);
static void PCG_(unused_function)(void)
	{ CG_(previous)(0, 0); CG_(next)(0, 0); CG_(clip)(0, 0);
	CG_(copy_if)(0, 0, 0); CG_(keep_if)(0, 0, 0); CG_(trim)(0, 0);
	CG_(each)(0, 0); CG_(if_each)(0, 0, 0); CG_(any)(0, 0);
	PCG_(unused_function_coda)(); }
static void PCG_(unused_function_coda)(void) { PCG_(unused_function)(); }
