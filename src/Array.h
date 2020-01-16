/** @license 2016 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 @subtitle Parameterised Contiguous Dynamic Array (Vector)

 ![Example of Array](../web/array.png)

 <tag:<T>Array> is a dynamic array that stores contiguous `<T>`, which must be
 set using `ARRAY_TYPE`. To ensure that the capacity is greater then or equal
 to the size, resizing may be necessary and incurs amortised cost. When adding
 new elements, the elements may change memory location to fit. It is therefore
 unstable; any pointers to this memory may become stale and unusable.

 `<T>Array` is not synchronised. Errors are returned with `errno`. The
 parameters are preprocessor macros, and are all undefined at the end of the
 file for convenience. `assert.h` is included in this file; to stop the
 debug assertions, use `#define NDEBUG` before `assert.h`.

 @param[ARRAY_NAME, ARRAY_TYPE]
 `<T>` that satisfies `C` naming conventions when mangled and a valid tag-type
 associated therewith; required. `<PT>` is private, whose names are prefixed in
 a manner to avoid collisions; any should be re-defined prior to use elsewhere.

 @param[ARRAY_STACK]
 Doesn't define removal functions except <fn:<T>ArrayPop>, making it a stack.

 @param[ARRAY_TO_STRING]
 Optional print function implementing <typedef:<PT>ToString>; makes available
 <fn:<T>ArrayToString>.

 @param[ARRAY_TEST]
 Unit testing framework <fn:<T>ArrayTest>, included in a separate header,
 <../test/ArrayTest.h>. Must be defined equal to a (random) filler function,
 satisfying <typedef:<PT>Action>. Requires `ARRAY_TO_STRING` and not `NDEBUG`.

 @std C89
 @cf [Heap](https://github.com/neil-edelman/Heap)
 @cf [List](https://github.com/neil-edelman/List)
 @cf [Orcish](https://github.com/neil-edelman/Orcish)
 @cf [Pool](https://github.com/neil-edelman/Pool)
 @cf [Set](https://github.com/neil-edelman/Set) */

#include <stddef.h> /* offset_of */
#include <stdlib.h> /* realloc free */
#include <assert.h> /* assert */
#include <string.h> /* memcpy memmove (strlen) (strerror strcpy memcmp) */
#include <errno.h>  /* errno */

/* Check defines. */
#ifndef ARRAY_NAME /* <!-- error */
#error Generic ARRAY_NAME undefined.
#endif /* error --> */
#ifndef ARRAY_TYPE /* <!-- error */
#error Generic ARRAY_TYPE undefined.
#endif /* --> */
#if defined(ARRAY_TEST) && !defined(ARRAY_TO_STRING) /* <!-- error */
#error ARRAY_TEST requires ARRAY_TO_STRING.
#endif /* error --> */
#if defined(ARRAY_CHILD) && (defined(ARRAY_STACK) || defined(ARRAY_TO_STRING) \
	|| defined(ARRAY_TEST)) /* <!-- error */
#error With ARRAY_CHILD, defining public interface functions is useless.
#endif /* error --> */
#if defined(T) || defined(T_) || defined(PT_) /* <!-- error */
#error T, T_, and PT_ cannot be defined.
#endif /* error --> */

/* <Kernighan and Ritchie, 1988, p. 231>. */
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
#define CAT_(x, y) x ## y
#define CAT(x, y) CAT_(x, y)
#define PCAT_(x, y) x ## _ ## y
#define PCAT(x, y) PCAT_(x, y)
#define T_(thing) CAT(ARRAY_NAME, thing)
#define PT_(thing) PCAT(array, PCAT(ARRAY_NAME, thing))

/** A valid tag type set by `ARRAY_TYPE`. This becomes `T`. */
typedef ARRAY_TYPE PT_(Type);
#define T PT_(Type)

/** Operates by side-effects. */
typedef void (*PT_(Action))(T *);

/** Given constant `data`, returns a boolean. */
typedef int (*PT_(Predicate))(const T *data);

/** To initialise it to an idle state, see <fn:<T>Array>, `ARRAY_IDLE`, `{0}`
 (`C99`), or being `static`.

 ![States.](../web/states.png) */
struct T_(Array);
struct T_(Array) {
	T *data;
	/* Fibonacci; data -> (c0 < c1 || c0 == c1 == max_size). */
	size_t capacity, next_capacity;
	/* !data -> !size, data -> size <= capacity */
	size_t size;
};
/* `{0}` is `C99`. */
#ifndef ARRAY_IDLE /* <!-- !zero */
#define ARRAY_IDLE { 0, 0, 0, 0 }
#endif /* !zero --> */

/** Ensures `min_capacity` of `a`.
 @param[min_capacity] If zero, allocates anyway.
 @param[update_ptr] Must be in the array or null, it updates this value.
 @return Success; otherwise, `errno` will be set.
 @throws[ERANGE] Tried allocating more then can fit in `size_t` or `realloc`
 doesn't follow [IEEE Std 1003.1-2001
 ](https://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html).
 @throws[realloc] */
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
	/* Fibonacci: c0 ^= c1, c1 ^= c0, c0 ^= c1, c1 += c0. Technically, this
	 calculation takes `\O(log (min_capacity - capacity))`, but we expect that
	 to be very small using a transdichotomous model, <Fredman, Willard, 1993>,
	 and much less than the time it takes to re-allocate. */
	while(c0 < min_capacity) {
		size_t temp = c0 + c1; c0 = c1; c1 = temp;
		if(c1 > max_size || c1 < c0) c1 = max_size;
	}
	if(!(data = realloc(a->data, c0 * sizeof *a->data)))
		{ if(!errno) errno = ERANGE; return 0; }
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

/** With `a`, and optional `update_ptr`, adds one to the size. Called from
 <fn:<T>ArrayNew> and <fn:<T>ArrayUpdateNew>. */
static T *PT_(new)(struct T_(Array) *const a, T **const update_ptr) {
	assert(a);
	if(a->size >= (size_t)-1) { errno = ERANGE; return 0; } /* Not likely. */
	if(!PT_(reserve)(a, a->size + 1, update_ptr)) return 0;
	return a->data + a->size++;
}

/** Zeros `a`. */
static void PT_(array)(struct T_(Array) *const a) {
	assert(a);
	a->data          = 0;
	a->capacity      = 0;
	a->next_capacity = 0;
	a->size          = 0;
}

#ifndef ARRAY_CHILD /* <!-- !sub-type */

/** Returns `a` to the idle state where it takes no dynamic memory.
 @param[a] If null, does nothing.
 @order \Theta(1)
 @allow */
static void T_(Array_)(struct T_(Array) *const a) {
	if(!a) return;
	free(a->data);
	PT_(array)(a);
}

/** Initialises `a` to be idle.
 @order \Theta(1)
 @allow */
static void T_(Array)(struct T_(Array) *const a) {
	if(!a) return;
	PT_(array)(a);
}

/** @param[a] If null, returns zero.
 @return The size of `a`.
 @order \O(1)
 @allow */
static size_t T_(ArraySize)(const struct T_(Array) *const a) {
	if(!a) return 0;
	return a->size;
}

#ifndef ARRAY_STACK /* <!-- !stack */

/** Removes `data` from `a`. Only defined if not `ARRAY_STACK`.
 @param[a, data] If null, returns false.
 @param[data] Will be removed; data will remain the same but be updated to the
 next element, or if this was the last element, the pointer will be past the
 end.
 @return Success, otherwise `errno` will be set for valid input.
 @throws[EDOM] `data` is not part of `a`.
 @order \O(n).
 @allow */
static int T_(ArrayRemove)(struct T_(Array) *const a, T *const data) {
	size_t n;
	if(!a || !data) return 0;
	if(data < a->data
		|| (n = data - a->data) >= a->size) return errno = EDOM, 0;
	memmove(data, data + 1, sizeof *data * (--a->size - n));
	return 1;
}

/** Removes `data` from `a` and replaces it with the tail. Only defined if not
 `ARRAY_STACK`.
 @param[a, data] If null, returns false.
 @param[data] Will be removed; data will remain the same but be updated to the
 last element, or if this was the last element, the pointer will be past the
 end.
 @return Success, otherwise `errno` will be set for valid input.
 @throws[EDOM] `data` is not part of `a`.
 @order \O(1).
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

/** Sets `a` to be empty. That is, the size of `a` will be zero, but if it was
 previously in an active non-idle state, it continues to be. Compare
 <fn:<T>Array_>.
 @param[a] If null, does nothing.
 @order \Theta(1)
 @allow */
static void T_(ArrayClear)(struct T_(Array) *const a) {
	if(!a) return;
	a->size = 0;
}

/** As long as the size doesn't go up, see <fn:<T>ArrayUpdateNew>.
 @param[a] If null, returns null.
 @return A pointer to the `a`'s data, indexable up to the `a`'s size.
 @order \Theta(1)
 @allow */
static T *T_(ArrayGet)(const struct T_(Array) *const a) {
	return a ? a->data : 0;
}

/** Gets an index given `data`.
 @param[a] Must be a valid object that stores `data`.
 @param[data] If the element is not part of the `a`, behaviour is undefined.
 @return An index.
 @order \Theta(1)
 @allow */
static size_t T_(ArrayIndex)(const struct T_(Array) *const a,
	const T *const data) {
	return data - a->data;
}

/** @param[a] If null or idle, returns null.
 @return One past the end of the array.
 @order \Theta(1)
 @allow */
static T *T_(ArrayEnd)(const struct T_(Array) *const a) {
	return a && a->data ? a->data + a->size : 0;
}

/** @param[a] If null, returns null.
 @return The last element or null if the a is empty.
 @order \Theta(1)
 @allow */
static T *T_(ArrayPeek)(const struct T_(Array) *const a) {
	return a && a->size ? a->data + a->size - 1 : 0;
}

/** The same value as <fn:<T>ArrayPeek>.
 @param[a] If null, returns null.
 @return Value from the the top of the `a` that is removed or null if the
 stack is empty.
 @order \Theta(1)
 @allow */
static T *T_(ArrayPop)(struct T_(Array) *const a) {
	return a && a->size ? a->data + --a->size : 0;
}

/** Iterate through `a` backwards.
 @param[a] The array; if null, returns null.
 @param[here] Set it to the current element; when null, it will be last
 element, if it exists.
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

/** @param[a] If is null, returns null.
 @return A new, un-initialised, element at the back of `a`, or null and `errno`
 will be set.
 @throws[ERANGE] Tried allocating more then can fit in `size_t` or `realloc`
 error and doesn't follow [IEEE Std 1003.1-2001
 ](https://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html).
 @throws[realloc]
 @order Amortised \O(1).
 @allow */
static T *T_(ArrayNew)(struct T_(Array) *const a) {
	if(!a) return 0;
	return PT_(new)(a, 0);
}

/** @param[a] If null, returns null.
 @param[update_ptr] Pointer to update on memory move if it is within the memory
 region that was changed to accommodate new space.
 @return A new, un-initialised, element at the back of `a`, or null and `errno`
 will be set.
 @throws[ERANGE] Tried allocating more then can fit in `size_t` or `realloc`
 error and doesn't follow [IEEE Std 1003.1-2001
 ](https://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html).
 @throws[realloc]
 @order Amortised \O(1).
 @allow */
static T *T_(ArrayUpdateNew)(struct T_(Array) *const a,
	T **const update_ptr) {
	if(!a) return 0;
	return PT_(new)(a, update_ptr);
}

/** Ensures that `a` is `reserve` capacity beyond the elements in the array,
 but doesn't add to the size.
 @param[a] If null, returns false.
 @param[reserve] If zero, returns true.
 @return The <fn:<T>ArrayEnd> of the `a`, where are `reserve` elements, or null
 and `errno` will be set. Writing on this memory space is safe, but one will
 have to increase the size manually, (see <fn:<T>ArrayBuffer>.)
 @throws[ERANGE] Tried allocating more then can fit in `size_t` or `realloc`
 error and doesn't follow [IEEE Std 1003.1-2001
 ](https://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html).
 @throws[realloc]
 @order Amortised \O(`reserve`).
 @allow */
static T *T_(ArrayReserve)(struct T_(Array) *const a, const size_t reserve) {
	if(!a) return 0;
	if(!reserve) return a->data ? a->data + a->size : 0;
	if(a->size > (size_t)-1 - reserve) { errno = ERANGE; return 0; }
	if(!PT_(reserve)(a, a->size + reserve, 0)) return 0;
	assert(a->data);
	return a->data + a->size;
}

/** Adds `add` elements to `a`.
 @param[a] If null, returns null.
 @param[add] If zero, returns null.
 @return The start of a new sub-array of `add` elements at the previous end of
 `a`, or null and `errno` will be set.
 @throws[ERANGE] Tried allocating more then can fit in `size_t` or `realloc`
 error and doesn't follow [IEEE Std 1003.1-2001
 ](https://pubs.opengroup.org/onlinepubs/009695399/functions/realloc.html). If
 <fn:<T>ArrayReserve> has been successful in reserving at least `add` elements,
 one is guaranteed success.
 @throws[realloc]
 @order Amortised \O(`add`).
 @allow */
static T *T_(ArrayBuffer)(struct T_(Array) *const a, const size_t add) {
	size_t prev_size;
	if(!a || !add) return 0;
	if(a->size > (size_t)-1 - add) { errno = ERANGE; return 0; }
	if(!PT_(reserve)(a, a->size + add, 0)) return 0;
	prev_size = a->size;
	a->size += add;
	return a->data + prev_size;
}

/** Iterates through `a` and calls `action` on all the elements. The topology
 of the list can not change while in this function. That is, don't call
 <fn:<T>ArrayNew>, <fn:<T>ArrayRemove>, _etc_ in `action`.
 @param[a, action] If null, does nothing.
 @order \O(`size` \times `action`)
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
 @order \O(`size` \times `action`)
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
 @order \O(`size` \times `predicate`)
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
 value is false, lazy deletes that item, calling `destruct` if not-null.
 @param[a, keep] If null, does nothing.
 @order \O(`size`)
 @allow */
static void T_(ArrayKeepIf)(struct T_(Array) *const a,
	const PT_(Predicate) keep, const PT_(Action) destruct) {
	T *erase = 0, *t;
	const T *retain = 0, *end;
	int keep0 = 1, keep1 = 0;
	if(!a || !keep) return;
	for(t = a->data, end = a->data + a->size; t < end; keep0 = keep1, t++) {
		if(!(keep1 = !!keep(t)) && destruct) destruct(t);
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
 @order \O(`size`)
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
 @throws[realloc]
 @order \Theta(`b.size`) if the elements have the same size, otherwise,
 amortised \O(`a.size` + `b.size`).
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
 @throws[realloc]
 @order \Theta(`b.size`) if the elements have the same size, otherwise,
 amortised \O(`a.size` + `b.size`).
 @allow */
static int T_(ArrayIndexSplice)(struct T_(Array) *const a, const size_t i0,
	const size_t i1, const struct T_(Array) *const b) {
	if(!a) return 0;
	if(a == b || i0 > i1 || i1 > a->size) return errno = EDOM, 0;
	return PT_(replace)(a, i0, i1, b);
}

#ifdef ARRAY_TO_STRING /* <!-- string */

/** Responsible for turning the first argument into a 12-`char` null-terminated
 output string. Used for `ARRAY_TO_STRING`. */
typedef void (*PT_(ToString))(const T *, char (*)[12]);
/* Check that `ARRAY_TO_STRING` is a function implementing
 <typedef:<PT>ToString>. */
static const PT_(ToString) PT_(to_string) = (ARRAY_TO_STRING);
	
/** Can print 4 things at once before it overwrites. One must a
 `ARRAY_TO_STRING` to a function implementing <typedef:<PT>ToString> to get
 this functionality.
 @return Prints `a` in a static buffer.
 @order \Theta(1); it has a 255 character limit; every element takes some of it.
 @allow */
static const char *T_(ArrayToString)(const struct T_(Array) *const a) {
	static char buffers[4][256];
	static size_t buffer_i;
	char *const buffer = buffers[buffer_i++], *b = buffer;
	const size_t buffers_no = sizeof buffers / sizeof *buffers,
		buffer_size = sizeof *buffers / sizeof **buffers;
	const char start = '(', comma = ',', space = ' ', end = ')',
		*const ellipsis_end = ",…)", *const null = "null",
		*const idle = "idle";
	const size_t ellipsis_end_len = strlen(ellipsis_end),
		null_len = strlen(null), idle_len = strlen(idle);
	size_t i;
	PT_(Type) *e, *e_end;
	int is_first = 1;
	assert(!(buffers_no & (buffers_no - 1)) && ellipsis_end_len >= 1
		&& buffer_size >= 1 + 11 + ellipsis_end_len + 1
		&& buffer_size >= null_len + 1
		&& buffer_size >= idle_len + 1);
	/* Advance the buffer for next time. */
	buffer_i &= buffers_no - 1;
	if(!a) { memcpy(b, null, null_len), b += null_len; goto terminate; }
	if(!a->data) { memcpy(b, idle, idle_len), b += idle_len; goto terminate; }
	*b++ = start;
	for(e = a->data, e_end = a->data + a->size; ; ) {
		if(!is_first) *b++ = comma, *b++ = space;
		else is_first = 0;
		PT_(to_string)(e, (char (*)[12])b);
		for(i = 0; *b != '\0' && i < 12; b++, i++);
		if(++e >= e_end) break;
		if((size_t)(b - buffer) > buffer_size - 2 - 11 - ellipsis_end_len - 1)
			goto ellipsis;
	}
	*b++ = end;
	goto terminate;
ellipsis:
	memcpy(b, ellipsis_end, ellipsis_end_len), b += ellipsis_end_len;
terminate:
	*b++ = '\0';
	assert(b <= buffer + buffer_size);
	return buffer;
}
#endif /* string --> */

#ifdef ARRAY_TEST /* <!-- test: need this file. */
#include "../test/TestArray.h" /** \include */
#endif /* test --> */

static void PT_(unused_coda)(void);
/** This silences unused function warnings from the pre-processor, but allows
 optimisation
 <http://stackoverflow.com/questions/43841780/silencing-unused-static-function-warnings-for-a-section-of-code>. */
static void PT_(unused_set)(void) {
	T_(Array_)(0);
	T_(Array)(0);
	T_(ArraySize)(0);
#ifndef ARRAY_STACK /* <!-- !stack */
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
	T_(ArrayReserve)(0, 0);
	T_(ArrayBuffer)(0, 0);
	T_(ArrayEach)(0, 0);
	T_(ArrayIfEach)(0, 0, 0);
	T_(ArrayAny)(0, 0);
	T_(ArrayKeepIf)(0, 0, 0);
	T_(ArrayTrim)(0, 0);
	T_(ArraySplice)(0, 0, 0, 0);
	T_(ArrayIndexSplice)(0, 0, 0, 0);
#ifdef ARRAY_TO_STRING
	T_(ArrayToString)(0);
#endif
	PT_(unused_coda)();
}
/** Some newer compilers are smart. */
static void PT_(unused_coda)(void) { PT_(unused_set)(); }

/* Un-define all macros. */
#undef CAT
#undef CAT_
#undef PCAT
#undef PCAT_
#else /* !sub-type --><!-- sub-type */
#undef ARRAY_CHILD
static void PT_(unused_coda)(void);
/** This is a subtype of another, more specialised type. `CAT`, _etc_, have to
 have the same meanings; they will be replaced with these, and `T` cannot be
 used. */
static void PT_(unused_set)(void) {
	/* <fn:<PT>reserve>, <fn:<PT>new>, and <fn:<PT>array> are integral; we want
	 to be notified when these are not called. Other stuff, not really. */
	PT_(range)(0, 0, 0, 0, 0);
	PT_(replace)(0, 0, 0, 0);
	PT_(unused_coda)();
}
static void PT_(unused_coda)(void) { PT_(unused_set)(); }
#endif /* sub-type --> */
#undef T
#undef T_
#undef PT_
#undef ARRAY_NAME
#undef ARRAY_TYPE
#ifdef ARRAY_STACK
#undef ARRAY_STACK
#endif
#ifdef ARRAY_TO_STRING
#undef ARRAY_TO_STRING
#endif
#ifdef ARRAY_TEST
#undef ARRAY_TEST
#endif
