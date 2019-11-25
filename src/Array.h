/** @license 2016 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 `<T>Array` is a dynamic contiguous array that stores `<T>`, which must be set
 using `ARRAY_TYPE`. To ensure that the capacity is greater then or equal to
 the size, resizing may be necessary and incurs amortised cost. When adding new
 elements, the elements may change memory location to fit. It is therefore
 unstable; any pointers to this memory may become stale and unusable when
 expanding.

 `<T>Array` is not synchronised. Errors are returned with `errno`. The
 parameters are preprocessor macros, and are all undefined at the end of the
 file for convenience. `assert.h` is included in this file; to stop the
 debug assertions, use `#define NDEBUG` before inclusion.

 ![States](../web/states.png)

 @param[ARRAY_NAME, ARRAY_TYPE]
 The name that literally becomes `<T>`, and a valid type associated therewith,
 accessible to the compiler at the time of inclusion; should be conformant to
 naming and to the maximum available length of identifiers. Must each be
 present before including.

 @param[ARRAY_STACK]
 Doesn't define removal functions except <fn:<T>ArrayPop>, making it a stack.

 @param[ARRAY_TO_STRING]
 Optional print function implementing <typedef:<PT>ToString>; makes available
 <fn:<T>ArrayToString>.

 @param[ARRAY_PRINTF]
 This defines <fn:<T>ArrayCatPrintf>, but only makes sense if `ARRAY_TYPE` is
 `char`. Uses `vsnprintf`, which may not be in `C89` libraries.

 @param[ARRAY_TEST]
 Unit testing framework using `<T>ArrayTest`, included in a separate header,
 `../test/ArrayTest.h`. Must be defined equal to a (random) filler function,
 satisfying <typedef:<PT>Action>. Requires `ARRAY_TO_STRING` and not `NDEBUG`.

 @title Contiguous Dynamic Parameterised Array
 @std C89
 @author Neil */



#include <stddef.h>	/* ptrdiff_t offset_of */
#include <stdlib.h>	/* realloc free */
#include <assert.h>	/* assert */
#include <string.h>	/* memcpy memmove (strerror strcpy memcmp in ArrayTest.h) */
#include <errno.h>	/* errno */
#include <limits.h> /* LONG_MAX */
#ifdef ARRAY_TO_STRING /* <-- print */
#include <stdio.h>	/* sprintf */
#endif /* print --> */
#ifdef ARRAY_PRINTF /* <-- printf */
#include <stdarg.h> /* va_* */
/* This function was standardised in `C99`. If one is getting a linker error,
 download any of the `vsnprintf` implementations and link it. */
/*fixme!!: int vsnprintf(char *s, size_t n, const char *format, va_list ap);*/
#endif /* printf --> */



/* Check defines. */
#ifndef ARRAY_NAME /* <-- error */
#error Generic ARRAY_NAME undefined.
#endif /* error --> */
#ifndef ARRAY_TYPE /* <-- error */
#error Generic ARRAY_TYPE undefined.
#endif /* --> */
#if defined(ARRAY_TEST) && !defined(ARRAY_TO_STRING) /* <-- error */
#error ARRAY_TEST requires ARRAY_TO_STRING.
#endif /* error --> */



/* Generics using the preprocessor;
 <http://stackoverflow.com/questions/16522341/pseudo-generics-in-c>. */
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
#define T_(thing) CAT(ARRAY_NAME, thing)
#define PT_(thing) PCAT(array, PCAT(ARRAY_NAME, thing))

/* Troubles with this line? check to ensure that `ARRAY_TYPE` is a valid type,
 whose definition is placed above `#include "Array.h"`. */
typedef ARRAY_TYPE PT_(Type);
#define T PT_(Type)



#ifdef ARRAY_TO_STRING /* <-- string */
/** Responsible for turning `<T>` (the first argument) into a 12 `char`
 null-terminated output string (the second.) Private; must re-declare. Used for
 `ARRAY_TO_STRING`. */
typedef void (*PT_(ToString))(const T *, char (*const)[12]);
/* Check that `ARRAY_TO_STRING` is a function implementing
 <typedef:<PT>ToString>, whose definition is placed above
 `#include "Array.h"`. */
static const PT_(ToString) PT_(to_string) = (ARRAY_TO_STRING);
#endif /* string --> */

/** Operates by side-effects on `data` only. Private; must re-declare. */
typedef void (*PT_(Action))(T *const data);

/** Given constant `data`, returns a boolean. Private; must re-declare. */
typedef int (*PT_(Predicate))(const T *const data);



/** The array. Zeroed data is a valid state. To instantiate explicitly, see
 <fn:<T>Array> or initialise it with `ARRAY_INIT` or `{0}` (C99.) */
struct T_(Array);
struct T_(Array) {
	T *data;
	/* Fibonacci; data -> (c0 < c1 || c0 == c1 == max_size). */
	size_t capacity, next_capacity;
	/* !data -> !size, data -> size <= capacity */
	size_t size;
};

/* `{0}` is `C99`. */
#ifndef ARRAY_ZERO /* <-- !zero */
#define ARRAY_ZERO { 0, 0, 0, 0 }
#endif /* !zero --> */



/** Ensures `min_capacity` of `a`.
 @param[update_ptr] Must be in the array or null, it updates this value.
 @return Success; otherwise, `errno` may be set.
 @throws[ERANGE] Tried allocating more then can fit in `size_t`.
 @throws[realloc] [IEEE Std 1003.1-2001
 ](https://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html). */
static int PT_(reserve)(struct T_(Array) *const a,
	const size_t min_capacity, T **const update_ptr) {
	size_t c0, c1;
	T *data;
	const size_t max_size = (size_t)-1 / sizeof(T *);
	assert(a);
	if(!a->data) {
		if(!min_capacity) return 1;
		c0 = 8;
		c1 = 13;
	} else {
		if(min_capacity <= a->capacity) return 1;
		c0 = a->capacity;
		c1 = a->next_capacity;
	}
	if(min_capacity > max_size) return errno = ERANGE, 0;
	assert(c0 < c1);
	/* Fibonacci: c0 ^= c1, c1 ^= c0, c0 ^= c1, c1 += c0; */
	while(c0 < min_capacity) {
		size_t temp = c0 + c1; c0 = c1; c1 = temp;
		if(c1 > max_size || c1 < c0) c1 = max_size;
	}
	if(!(data = realloc(a->data, c0 * sizeof *a->data))) return 0;
	if(update_ptr && a->data != data)
		*update_ptr = data + (*update_ptr - a->data);
	a->data = data;
	a->capacity = c0;
	a->next_capacity = c1;
	return 1;
}

/** In `a`, converts `anchor` and `range` à la Python and stores them in the
 pointers `p0` and `p1` _st_ `*p0, *p1 \in [0, a.size], *p0 <= *p1`.
 @param[anchor] An element in the array or null to indicate past the end.
 @return Success.
 @throws[ERANGE] `anchor` is not null and not in `a`.
 @throws[ERANGE] `range` is greater then +/-65534.
 @throws[ERANGE] `size_t` overflow. */
static int PT_(range)(const struct T_(Array) *const a, const T *anchor,
	const long range, size_t *const p0, size_t *const p1) {
	size_t i0, i1;
	assert(a && p0 && p1);
	if((anchor && (anchor < a->data || anchor >= a->data + a->size))
		|| range > 65534l || range < -65534l) return errno = ERANGE, 0;
	i0 = anchor ? (size_t)(anchor - a->data) : a->size;
	if(range < 0) {
		i1 = (size_t)(-range) > a->size ? 0 : a->size - (size_t)(-range) + 1;
		if(i0 > i1) i1 = i0;
	} else {
		i1 = i0 + (size_t)range;
		if(i0 > i1) return errno = ERANGE, 0;
		if(i1 > a->size) i1 = a->size;
	}
	*p0 = i0, *p1 = i1;
	return 1;
}

/** Replace: does the work. With `a`, array indices `i0` (inclusive) to `i1`
 (exclusive) will be replaced with `b`. */
static int PT_(replace)(struct T_(Array) *const a, const size_t i0,
	const size_t i1, const struct T_(Array) *const b) {
	const size_t a_range = i1 - i0, b_range = b ? b->size : 0;
	assert(a && a != b && i0 <= i1 && i1 <= a->size);
	if(a_range < b_range) { /* The output is bigger. */
		const size_t diff = b_range - a_range;
		if(a->size > (size_t)-1 - diff) return errno = ERANGE, 0;
			if(!PT_(reserve)(a, a->size + diff, 0)) return 0;
		memmove(a->data + i1 + diff, a->data + i1,(a->size-i1)*sizeof *a->data);
		a->size += diff;
	} else if(b_range < a_range) { /* The output is smaller. */
		memmove(a->data + i0 + b_range, a->data+i1,(a->size-i1)*sizeof*a->data);
		a->size -= a_range - b_range;
	}
	if(b) memcpy(a->data + i0, b->data, b->size * sizeof *a->data);
	return 1;
}

/** Zeros `a` except for `ARRAY_MIGRATE_ALL` which is initialised in the
 containing function, and not `!ARRAY_FREE_LIST`, which is initialised in
 <fn:<PT>reserve>. */
static void PT_(array)(struct T_(Array) *const a) {
	assert(a);
	a->data          = 0;
	a->capacity      = 0;
	a->next_capacity = 0;
	a->size          = 0;
}

/** Destructor for `a`; returns an initialised `a` to the empty state where it
 takes no dynamic memory.
 @param[a] If null, does nothing.
 @order \Theta(1)
 @allow */
static void T_(Array_)(struct T_(Array) *const a) {
	if(!a) return;
	free(a->data);
	PT_(array)(a);
}

/** Initialises `a` to be empty.
 @order \Theta(1)
 @allow */
static void T_(Array)(struct T_(Array) *const a) {
	if(!a) return;
	PT_(array)(a);
}

/** @param[a] If null, returns zero.
 @return The size of `a`.
 @order O(1)
 @allow */
static size_t T_(ArraySize)(const struct T_(Array) *const a) {
	if(!a) return 0;
	return a->size;
}

#ifndef ARRAY_STACK /* <-- !stack */

/** Removes `data` from `a`.
 @param[a, data] If null, returns false.
 @param[data] Will be removed; data will remain the same but be updated to the
 next element, or if this was the last element, the pointer will be past the
 end.
 @return Success, otherwise `errno` will be set for valid input.
 @throws[EDOM] `data` is not part of `a`.
 @order O(n).
 @allow */
static int T_(ArrayRemove)(struct T_(Array) *const a, T *const data) {
	size_t n;
	if(!a || !data) return 0;
	if(data < a->data
		|| (n = data - a->data) >= a->size) return errno = EDOM, 0;
	memmove(data, data + 1, sizeof *data * (--a->size - n));
	return 1;
}

/** Removes `data` from `a` and replaces the spot it was in with the tail.
 @param[a, data] If null, returns false.
 @param[data] Will be removed; data will remain the same but be updated to the
 last element, or if this was the last element, the pointer will be past the
 end.
 @return Success, otherwise `errno` will be set for valid input.
 @throws[EDOM] `data` is not part of `a`.
 @order O(1).
 @allow */
static int T_(ArrayLazyRemove)(struct T_(Array) *const a, T *const data) {
	size_t n;
	if(!a || !data) return 0;
	if(data < a->data
	   || (n = data - a->data) >= a->size) return errno = EDOM, 0;
	if(--a->size != n) memcpy(data, a->data + a->size, sizeof *data);
	return 1;
}

#endif /* !stack --> */

/** Sets the size of `a` to zero, effectively removing all the elements, but
 leaves the capacity alone, (the only thing that will free memory allocation
 is <fn:<T>Array_>.)
 @param[a] If null, does nothing.
 @order \Theta(1)
 @allow */
static void T_(ArrayClear)(struct T_(Array) *const a) {
	if(!a) return;
	a->size = 0;
}

/** Causing something to be added to the `<T>Array` may invalidate this
 pointer, see <fn:<T>ArrayUpdateNew>.
 @param[a] If null, returns null.
 @return A pointer to the `a`'s data, indexable up to the `a`'s size.
 @order \Theta(1)
 @allow */
static T *T_(ArrayGet)(const struct T_(Array) *const a) {
	return a ? a->data : 0;
}

/** Causing something to be added to the `<T>Array` may invalidate this
 pointer, see <fn:<T>ArrayUpdateNew>.
 @param[a] If null, returns null.
 @return One past the end of the array; take care when dereferencing, as it is
 not part of the array.
 @order \Theta(1)
 @allow */
static T *T_(ArrayEnd)(const struct T_(Array) *const a) {
	return a ? a->data + a->size : 0;
}

/** Gets an index given `data`.
 @param[a] Must be a valid object that stores `data`.
 @param[data] If the element is not part of the `a`, behaviour is undefined.
 @return An index.
 @order \Theta(1)
 @fixme Untested.
 @allow */
static size_t T_(ArrayIndex)(const struct T_(Array) *const a,
	const T *const data) {
	return data - a->data;
}

/** @param[a] If null, returns null.
 @return The last element or null if the a is empty. Causing something to be
 added to the `a` may invalidate this pointer.
 @order \Theta(1)
 @fixme Untested.
 @allow */
static T *T_(ArrayPeek)(const struct T_(Array) *const a) {
	if(!a || !a->size) return 0;
	return a->data + a->size - 1;
}

/** The same value as <fn:<T>ArrayPeek>.
 @param[a] If null, returns null.
 @return Value from the the top of the `a` that is removed or null if the
 stack is empty. Causing something to be added to the `a` may invalidate
 this pointer. See <fn:<T>ArrayUpdateNew>.
 @order \Theta(1)
 @allow */
static T *T_(ArrayPop)(struct T_(Array) *const a) {
	if(!a || !a->size) return 0;
	return a->data + --a->size;
}

/** Iterate through `a` backwards.
 @param[a] The array; if null, returns null.
 @param[here] Set it to null to get the last element, if it exists.
 @return A pointer to the previous element or null if it does not exist.
 @order \Theta(1)
 @allow */
static T *T_(ArrayBack)(const struct T_(Array) *const a, const T *const here) {
	size_t idx;
	if(!a) return 0;
	if(!here) {
		if(!a->size) return 0;
		idx = a->size;
	} else {
		idx = (size_t)(here - a->data);
		if(!idx) return 0;
	}
	return a->data + idx - 1;
}

/** Iterate through `a`. It is safe to add using <fn:<T>ArrayUpdateNew> with
 the return value as `update`. Removing an element causes the pointer to go to
 the next element, if it exists.
 @param[a] The array; if null, returns null.
 @param[here] Set it to null to get the first element, if it exists.
 @return A pointer to the next element or null if there are no more.
 @order \Theta(1)
 @allow */
static T *T_(ArrayNext)(const struct T_(Array) *const a, const T *const here) {
	size_t idx;
	if(!a) return 0;
	idx = here ? (size_t)(here - a->data + 1) : 0;
	return idx < a->size ? a->data + idx : 0;
}

/** With `a`, and optional `update_ptr`, adds one to the size. Called from
 <fn:<T>ArrayNew> and <fn:<T>ArrayUpdateNew>. */
static T *PT_(new)(struct T_(Array) *const a, T **const update_ptr) {
	assert(a);
	if(!PT_(reserve)(a, a->size + 1, update_ptr)) return 0;
	return a->data + a->size++;
}

/** Gets an uninitialised new element. May move the `a` to a new memory
 location to fit the new size.
 @param[a] If is null, returns null.
 @return A new, un-initialised, element, or null and `errno` may be set.
 @throws[ERANGE] Tried allocating more then can fit in `size_t` objects.
 @throws[realloc] [IEEE Std 1003.1-2001
 ](https://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html).
 @order Amortised O(1).
 @allow */
static T *T_(ArrayNew)(struct T_(Array) *const a) {
	if(!a) return 0;
	return PT_(new)(a, 0);
}

/** Gets an uninitialised new element in `a` and updates the `update_ptr` if it
 is within the memory region that was changed to accomodate new space.
 @param[a] If null, returns null.
 @param[update_ptr] Pointer to update on memory move. If null, does nothing.
 @return A new, un-initialised, element, or null and `errno` may be set.
 @throws[ERANGE] Tried allocating more then can fit in `size_t`.
 @throws[realloc] [IEEE Std 1003.1-2001
 ](https://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html).
 @order amortised O(1)
 @fixme Untested.
 @allow */
static T *T_(ArrayUpdateNew)(struct T_(Array) *const a,
	T **const update_ptr) {
	if(!a) return 0;
	return PT_(new)(a, update_ptr);
}

/** Ensures that `a` array is `buffer` capacity beyond the elements in the
 array.
 @param[a] If is null, returns null.
 @param[buffer] If this is zero, returns null.
 @return One past the end of the array, or null and `errno` may be set.
 @throws[ERANGE] Tried allocating more then can fit in `size_t`.
 @throws[realloc] [IEEE Std 1003.1-2001
 ](https://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html).
 @order Amortised O(`buffer`).
 @fixme Test.
 @allow */
static T *T_(ArrayBuffer)(struct T_(Array) *const a, const size_t buffer) {
	if(!a || !buffer || !PT_(reserve)(a, a->size + buffer, 0)) return 0;
	return a->data + a->size;
}

/** Adds `add` to the size in `a`.
 @return Success.
 @throws[ERANGE] The size added is greater than the capacity. To avoid this,
 call <fn:<T>ArrayBuffer> before.
 @order O(1)
 @fixme Test.
 @allow */
static int T_(ArrayExpand)(struct T_(Array) *const a, const size_t add) {
	if(!a) return 0;
	if(add > a->capacity || a->size > a->capacity - add)
		return errno = ERANGE, 0;
	a->size += add;
	return 1;
}

/** Iterates through `a` and calls `action` on all the elements. The topology of
 the list can not change while in this function. That is, don't call
 <fn:<T>ArrayNew>, <fn:<T>ArrayRemove>, _etc_ in `action`.
 @param[a, action] If null, does nothing.
 @order O(`size` \times `action`)
 @fixme Untested.
 @fixme Sequence interface.
 @allow */
static void T_(ArrayEach)(struct T_(Array) *const a,
	const PT_(Action) action) {
	T *t, *end;
	if(!a || !action) return;
	for(t = a->data, end = t + a->size; t < end; t++) action(t);
}

/** Iterates through `a` and calls `action` on all the elements for which
 `predicate` returns true. The topology of the list can not change while in
 this function.
 @param[a, predicate, action] If null, does nothing.
 @order O(`size` \times `action`)
 @fixme Untested.
 @fixme Sequence interface.
 @allow */
static void T_(ArrayIfEach)(struct T_(Array) *const a,
	const PT_(Predicate) predicate, const PT_(Action) action) {
	T *t, *end;
	if(!a || !action || !predicate) return;
	for(t = a->data, end = t + a->size; t < end; t++)
		if(predicate(t)) action(t);
}

/** Iterates through `a` and calls `predicate` until it returns true.
 @param[a, predicate] If null, returns null.
 @return The first `predicate` that returned true, or, if the statement is
 false on all, null.
 @order O(`size` \times `action`)
 @fixme Untested.
 @fixme Sequence interface.
 @allow */
static T *T_(ArrayAny)(const struct T_(Array) *const a,
	const PT_(Predicate) predicate) {
	T *t, *end;
	if(!a || !predicate) return 0;
	for(t = a->data, end = t + a->size; t < end; t++)
		if(predicate(t)) return t;
	return 0;
}

/** For all elements of `a`, calls `keep`, and for each element, if the return
 value is false, lazy deletes that item.
 @param[a, keep] If null, does nothing.
 @order O(`size`)
 @allow */
static void T_(ArrayKeepIf)(struct T_(Array) *const a,
	const PT_(Predicate) keep) {
	T *erase = 0, *t;
	const T *retain = 0, *end;
	int keep0 = 1, keep1 = 0;
	if(!a || !keep) return;
	for(t = a->data, end = a->data + a->size; t < end; keep0 = keep1, t++) {
		keep1 = !!keep(t);
		if(!(keep0 ^ keep1)) continue; /* Not a falling/rising edge. */
		if(keep1) { /* Rising edge. */
			assert(erase && !retain);
			retain = t;
		} else if(erase) { /* Falling edge. */
			size_t n = t - retain;
			assert(erase < retain && retain < t);
			memmove(erase, retain, n * sizeof *t);
			erase += n;
			retain = 0;
		} else { /* Falling edge, (first time only.) */
			erase = t;
		}
	}
	if(!erase) return; /* All elements were kept. */
	if(keep1) { /* Delayed move when the iteration ended; repeat. */
		size_t n = t - retain;
		assert(retain && erase < retain && retain < t);
		memmove(erase, retain, n * sizeof *t);
		erase += n;
	}
	/* Adjust the size. */
	assert((size_t)(erase - a->data) <= a->size);
	a->size = erase - a->data;
}

/** Removes at either end of `a` of things that `predicate` returns true.
 @param[a, predicate] If null, does nothing.
 @order O(`size`)
 @allow */
static void T_(ArrayTrim)(struct T_(Array) *const a,
	const PT_(Predicate) predicate) {
	size_t i;
	if(!a || !predicate) return;
	while(a->size && predicate(a->data + a->size - 1)) a->size--;
	for(i = 0; i < a->size && predicate(a->data + i); i++);
	if(!i) return;
	assert(i < a->size);
	memmove(a->data, a->data + i, sizeof *a->data * i), a->size -= i;
}

/** In `a`, replaces the elements from `anchor` up to `range` with a copy of
 `b`.
 @param[a] If null, returns zero.
 @param[anchor] Beginning of the replaced value, inclusive. If null, appends to
 the end.
 @param[range] How many replaced values in the original; negative values are
 implicitly plus the length of the array; clamped at the minimum and maximum.
 @param[b] The replacement array. If null, deletes without replacing. It is
 more efficient than individual <fn:<T>ArrayRemove> to delete several
 consecutive values.
 @return Success.
 @throws[EDOM] `a` and `b` are not null and the same.
 @throws[ERANGE] `anchor` is not null and not in `a`.
 @throws[ERANGE] `range` is greater then 65535 or smaller then -65534.
 @throws[ERANGE] `b` would cause the array to overflow.
 @throws[realloc] [IEEE Std 1003.1-2001
 ](https://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html).
 @order \Theta(`b.size`) if the elements have the same size, otherwise,
 amortised O(`a.size` + `b.size`).
 @allow */
static int T_(ArraySplice)(struct T_(Array) *const a, const T *anchor,
	const long range, const struct T_(Array) *const b) {
	size_t i0, i1;
	if(!a) return 0;
	if(a == b) return errno = EDOM, 0;
	if(!PT_(range)(a, anchor, range, &i0, &i1)) return 0;
	return PT_(replace)(a, i0, i1, b);
}

/** In `a`, replaces the elements from indices `i0` (inclusive) to `i1`
 (exclusive) with a copy of `b`.
 @param[a] If null, returns zero.
 @param[i0, i1] The replacement indices, `[i0, i1)`, such that
 `0 <= i0 <= i1 <= a.size`.
 @param[b] The replacement array. If null, deletes without replacing.
 @return Success.
 @throws[EDOM] `a` and `b` are not null and the same.
 @throws[EDOM] `i0` or `i1` are out-of-bounds or `i0 > i1`.
 @throws[ERANGE] `b` would cause the array to overflow.
 @throws[realloc] [IEEE Std 1003.1-2001
 ](https://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html).
 @order \Theta(`b.size`) if the elements have the same size, otherwise,
 amortised O(`a.size` + `b.size`).
 @allow */
static int T_(ArrayIndexSplice)(struct T_(Array) *const a, const size_t i0,
	const size_t i1, const struct T_(Array) *const b) {
	if(!a) return 0;
	if(a == b || i0 > i1 || i1 > a->size) return errno = EDOM, 0;
	return PT_(replace)(a, i0, i1, b);
}

#ifdef ARRAY_TO_STRING /* <-- print */

#ifndef ARRAY_PRINT_THINGS /* <-- once inside translation unit */
#define ARRAY_PRINT_THINGS

static const char *const array_cat_start     = "[";
static const char *const array_cat_end       = "]";
static const char *const array_cat_alter_end = "...]";
static const char *const array_cat_sep       = ", ";
static const char *const array_cat_star      = "*";
static const char *const array_cat_null      = "null";

struct Array_SuperCat {
	char *print, *cursor;
	size_t left;
	int is_truncated;
};
/** Initialises `cat` to a static array `print` and `print_size`. */
static void array_super_cat_init(struct Array_SuperCat *const cat,
	char *const print, const size_t print_size) {
	cat->print = cat->cursor = print;
	cat->left = print_size;
	cat->is_truncated = 0;
	print[0] = '\0';
}
/** `append`s the string to `cat` */
static void array_super_cat(struct Array_SuperCat *const cat,
	const char *const append) {
	size_t lu_took; int took;
	if(cat->is_truncated) return;
	took = sprintf(cat->cursor, "%.*s", (int)cat->left, append);
	if(took < 0)  { cat->is_truncated = -1; return; } /*implementation defined*/
	if(took == 0) { return; }
	if((lu_took = (size_t)took) >= cat->left)
		cat->is_truncated = -1, lu_took = cat->left - 1;
	cat->cursor += lu_took, cat->left -= lu_took;
}
#endif /* once --> */

/** Can print 4 things at once before it overwrites. One must a
 `ARRAY_TO_STRING` to a function implementing <typedef:<PT>ToString> to get
 this functionality.
 @return Prints `a` in a static buffer.
 @order \Theta(1); it has a 255 character limit; every element takes some of it.
 @fixme ToString interface.
 @allow */
static const char *T_(ArrayToString)(const struct T_(Array) *const a) {
	static char buffer[4][256];
	static unsigned buffer_i;
	struct Array_SuperCat cat;
	int is_first = 1;
	char scratch[12];
	size_t i;
	assert(strlen(array_cat_alter_end) >= strlen(array_cat_end));
	assert(sizeof buffer > strlen(array_cat_alter_end));
	array_super_cat_init(&cat, buffer[buffer_i],
		sizeof *buffer / sizeof **buffer - strlen(array_cat_alter_end));
	buffer_i++, buffer_i &= 3;
	if(!a) {
		array_super_cat(&cat, array_cat_null);
		return cat.print;
	}
	array_super_cat(&cat, array_cat_start);
	for(i = 0; i < a->size; i++) {
		if(!is_first) array_super_cat(&cat, array_cat_sep); else is_first = 0;
		PT_(to_string)(a->data + i, &scratch),
		scratch[sizeof scratch - 1] = '\0';
		array_super_cat(&cat, scratch);
		if(cat.is_truncated) break;
	}
	sprintf(cat.cursor, "%s",
		cat.is_truncated ? array_cat_alter_end : array_cat_end);
	return cat.print; /* Static buffer. */
}

#endif /* print --> */

#ifdef ARRAY_PRINTF /* <-- printf */

/** Concatenates a [printf
 ](http://pubs.opengroup.org/onlinepubs/007908799/xsh/fprintf.html)-style
 format string. If it sees a '\0' as the last entry, it overwrites it; on
 success, it is always null-terminated. One must have `ARRAY_PRINTF` and
 `ARRAY_TYPE` must be `char`.
 @std C89 but C99 libraries required
 @return Success.
 @throws[malloc, vsnprintf]
 @allow */
static int T_(ArrayCatPrintf)(struct T_(Array) *const a,
	const char *const fmt, ...) {
	va_list argp;
	char garbage;
	int length;
	int is_null_term = 0;
	size_t total_size;
	if(!a || !fmt) return 0;
	printf("Str: starting with <%s> appending <%s>\n", a->data, fmt);
	/* Check the length first by printing to garbage. */
	va_start(argp, fmt);
	length = vsnprintf(&garbage, 0ul, fmt, argp);
	va_end(argp);
	if(length < 0) return 0; /* `vsnprintf` error. */
	/* Expand the buffer. If one has troubles with this line, ensure
	 `ARRAY_TYPE` is `char`. */
	is_null_term = a->size ? a->data[a->size - 1] == '\0' : 0;
	total_size = a->size + length + !is_null_term;
	printf("is_null_term %d\ntotal_size %lu\n", is_null_term, total_size);
	if(total_size < a->size) { errno = ERANGE; return 0; } /* Overflow. */
	if(!PT_(reserve)(a, total_size, 0)) return 0;
	/* Now that we have enough space, do the actual printing. */
	va_start(argp, fmt);
	length = vsnprintf(a->data + a->size - is_null_term, length + 1, fmt, argp);
	va_end(argp);
	if(length < 0) return 0; /* `vsnprintf` error; unlikely. */
	a->size += length + !is_null_term;
	printf("Str: <%s>:%lu\n", a->data, a->size);
	return 1;
}
#endif /* printf --> */

#ifdef ARRAY_TEST /* <-- test */
#include "../test/TestArray.h" /* Need this file if one is going to run tests.*/
#endif /* test --> */

/* Prototype. */
static void PT_(unused_coda)(void);
/** This silences unused function warnings from the pre-processor, but allows
 optimisation, (hopefully.)
 <http://stackoverflow.com/questions/43841780/silencing-unused-static-function-warnings-for-a-section-of-code> */
static void PT_(unused_set)(void) {
	T_(Array_)(0);
	T_(Array)(0);
	T_(ArraySize)(0);
#ifndef ARRAY_STACK /* <-- !stack */
	T_(ArrayRemove)(0, 0);
	T_(ArrayLazyRemove)(0, 0);
#endif /* !stack --> */
	T_(ArrayClear)(0);
	T_(ArrayGet)(0);
	T_(ArrayEnd)(0);
	T_(ArrayIndex)(0, 0);
	T_(ArrayPeek)(0);
	T_(ArrayPop)(0);
	T_(ArrayBack)(0, 0);
	T_(ArrayNext)(0, 0);
	T_(ArrayNew)(0);
	T_(ArrayUpdateNew)(0, 0);
	T_(ArrayBuffer)(0, 0);
	T_(ArrayExpand)(0, 0);
	T_(ArrayEach)(0, 0);
	T_(ArrayIfEach)(0, 0, 0);
	T_(ArrayAny)(0, 0);
	T_(ArrayKeepIf)(0, 0);
	T_(ArrayTrim)(0, 0);
	T_(ArraySplice)(0, 0, 0, 0);
	T_(ArrayIndexSplice)(0, 0, 0, 0);
#ifdef ARRAY_TO_STRING
	T_(ArrayToString)(0);
#endif
#ifdef ARRAY_PRINTF
	T_(ArrayCatPrintf)(0, 0);
#endif
	PT_(unused_coda)();
}
/** `clang`'s pre-processor is not fooled if you have one function. */
static void PT_(unused_coda)(void) { PT_(unused_set)(); }



/* Un-define all macros. */
#undef ARRAY_NAME
#undef ARRAY_TYPE
/* Undocumented; allows nestled inclusion so long as: `CAT_`, `CAT`, `PCAT`,
 `PCAT_` conform, and `T` is not used. */
#ifdef ARRAY_SUBTYPE /* <-- sub */
#undef ARRAY_SUBTYPE
#else /* sub --><-- !sub */
#undef CAT
#undef CAT_
#undef PCAT
#undef PCAT_
#endif /* !sub --> */
#undef T
#undef T_
#undef PT_
#ifdef ARRAY_PRINTF
#undef ARRAY_PRINTF
#endif
#ifdef ARRAY_STACK
#undef ARRAY_STACK
#endif
#ifdef ARRAY_TO_STRING
#undef ARRAY_TO_STRING
#endif
#ifdef ARRAY_TEST
#undef ARRAY_TEST
#endif
