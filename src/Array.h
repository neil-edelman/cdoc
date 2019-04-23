/** 2016 Neil Edelman, distributed under the terms of the MIT License;
 see readme.txt, or \url{ https://opensource.org/licenses/MIT }.

 {<T>Array} is a dynamic array that stores unordered {<T>}, which must be set
 using {ARRAY_TYPE}. The capacity is greater then or equal to the size;
 resizing incurs amortised cost. You cannot shrink the capacity, only cause it
 to grow.

 {<T>Array} is contiguous, and therefore unstable; that is, when adding new
 elements, it may change the memory location. Pointers to this memory become
 stale and unusable.

 {<T>Array} is not synchronised. Errors are returned with {errno}. The
 parameters are preprocessor macros, and are all undefined at the end of the
 file for convenience.

 @param ARRAY_NAME, ARRAY_TYPE
 The name that literally becomes {<T>}, and a valid type associated therewith,
 accessible to the compiler at the time of inclusion; should be conformant to
 naming and to the maximum available length of identifiers. Must each be
 present before including.

 @param ARRAY_STACK
 Doesn't define \see{<T>ArrayRemove}, making it a stack. Not compatible with
 {ARRAY_TAIL_DELETE}.

 @param ARRAY_TAIL_DELETE
 Instead of preserving order on removal, {O(n)}, this copies the tail element
 to the removed. One gives up order, but preserves contiguity in {O(1)}. Not
 compatible with {ARRAY_STACK}.

 @param ARRAY_TO_STRING
 Optional print function implementing {<T>ToString}; makes available
 \see{<T>ArrayToString}.

 @param ARRAY_TEST
 Unit testing framework using {<T>ArrayTest}, included in a separate header,
 {../test/ArrayTest.h}. Must be defined equal to a (random) filler function,
 satisfying {<T>Action}. If {NDEBUG} is not defined, turns on {assert} private
 function integrity testing. Requires {ARRAY_TO_STRING}.

 @title		Array.h
 @std		C89
 @author	Neil
 @version	2019-03 Renamed {Pool} to {Array}. Took out migrate.
 @since		2018-04 Merged {Stack} into {Pool} again to eliminate duplication;
			2018-03 Why have an extra level of indirection?
			2018-02 Errno instead of custom errors.
			2017-12 Introduced {POOL_PARENT} for type-safety.
			2017-11 Forked {Stack} from {Pool}.
			2017-10 Replaced {PoolIsEmpty} by {PoolElement}, much more useful.
			2017-10 Renamed Pool; made migrate automatic.
			2017-07 Made migrate simpler.
			2017-05 Split {List} from {Pool}; much simpler.
			2017-01 Almost-redundant functions simplified.
			2016-11 Multi-index.
			2016-08 Permute. */



#include <stddef.h>	/* ptrdiff_t offset_of */
#include <stdlib.h>	/* realloc free */
#include <assert.h>	/* assert */
#include <string.h>	/* memcpy memmove (strerror strcpy memcmp in ArrayTest.h) */
#include <errno.h>	/* errno */
#ifdef ARRAY_TO_STRING /* <-- print */
#include <stdio.h>	/* sprintf */
#endif /* print --> */



/* Check defines. */
#ifndef ARRAY_NAME /* <-- error */
#error Generic ARRAY_NAME undefined.
#endif /* error --> */
#ifndef ARRAY_TYPE /* <-- error */
#error Generic ARRAY_TYPE undefined.
#endif /* --> */
#if defined(ARRAY_STACK) && defined(ARRAY_TAIL_DELETE) /* <-- error */
#error ARRAY_STACK and ARRAY_TAIL_DELETE are mutually exclusive.
#endif /* error --> */
#if defined(ARRAY_TEST) && !defined(ARRAY_TO_STRING) /* <-- error */
#error ARRAY_TEST requires ARRAY_TO_STRING.
#endif /* error --> */
#if !defined(ARRAY_TEST) && !defined(NDEBUG) /* <-- ndebug */
#define ARRAY_NDEBUG
#define NDEBUG
#endif /* ndebug --> */



/* Generics using the preprocessor;
 \url{ http://stackoverflow.com/questions/16522341/pseudo-generics-in-c }. */
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
#define QUOTE_(name) #name
#define QUOTE(name) QUOTE_(name)
#define T_(thing) CAT(ARRAY_NAME, thing)
#define PT_(thing) PCAT(array, PCAT(ARRAY_NAME, thing))
#define T_NAME QUOTE(ARRAY_NAME)

/* Troubles with this line? check to ensure that {ARRAY_TYPE} is a valid type,
 whose definition is placed above {#include "Array.h"}. */
typedef ARRAY_TYPE PT_(Type);
#define T PT_(Type)



#ifdef ARRAY_TO_STRING /* <-- string */
/** Responsible for turning {<T>} (the first argument) into a 12 {char}
 null-terminated output string (the second.) Used for {ARRAY_TO_STRING}. */
typedef void (*PT_(ToString))(const T *, char (*const)[12]);
/* Check that {ARRAY_TO_STRING} is a function implementing {<PT>ToString}, whose
 definition is placed above {#include "Array.h"}. */
static const PT_(ToString) PT_(to_string) = (ARRAY_TO_STRING);
#endif /* string --> */

/* Operates by side-effects only. */
typedef void (*PT_(Action))(T *const data);



/** The array. To instantiate, see \see{<T>Array}. */
struct T_(Array);
struct T_(Array) {
	T *data;
	/* {nodes} -> {capacity} -> {c[0] < c[1] || c[0] == c[1] == max_size}.
	 Fibonacci, [0] is the capacity, [1] is next. */
	size_t capacity[2];
	/* {nodes} ? {size <= capacity[0]} : {size == 0}. Including removed. */
	size_t size;
};



/** Ensures capacity.
 @return Success; otherwise, {errno} may be set.
 @throws ERANGE: Tried allocating more then can fit in {size_t}.
 @throws {realloc} errors: {IEEE Std 1003.1-2001}. */
static int PT_(reserve)(struct T_(Array) *const a,
	const size_t min_capacity, T **const update_ptr) {
	size_t c0, c1;
	T *data;
	const size_t max_size = (size_t)-1 / sizeof *data;
	assert(a && a->size <= a->capacity[0]
		&& (a->capacity[0] < a->capacity[1] || !a->data
		|| (a->capacity[0] == a->capacity[1]) == max_size)
		&& a->capacity[1] <= max_size);
	if(a->capacity[0] >= min_capacity) return 1;
	if(min_capacity > max_size) return errno = ERANGE, 0;
	if(!a->data) {
		c0 = 8 /* fibonacci 6 */;
		c1 = 13 /* fibonacci 7 */;
	} else {
		c0 = a->capacity[0];
		c1 = a->capacity[1];
	}
	while(c0 < min_capacity) {
		/* c0 ^= c1, c1 ^= c0, c0 ^= c1, c1 += c0; */
		size_t temp = c0 + c1; c0 = c1; c1 = temp;
		if(c1 > max_size || c1 <= c0) c1 = max_size;
	}
	if(!(data = realloc(a->data, c0 * sizeof *a->data))) return 0;
	if(update_ptr && a->data != data) {
		/* Migrate data; violates pedantic strict-ANSI? */
		const void *begin = a->data,
			*end = (const char *)a->data + a->size * sizeof *data;
		ptrdiff_t delta = (const char *)data - (const char *)a->data;
		const void *const u = *update_ptr;
		if(u >= begin && u < end) *(char **const)update_ptr += delta;
	}
	a->data = data;
	a->capacity[0] = c0;
	a->capacity[1] = c1;
	return 1;
}

/** Zeros {a} except for {ARRAY_MIGRATE_ALL} which is initialised in the
 containing function, and not {!ARRAY_FREE_LIST}, which is initialised in
 \see{<PT>_reserve}. */
static void PT_(array)(struct T_(Array) *const a) {
	assert(a);
	a->data        = 0;
	a->capacity[0] = 0;
	a->capacity[1] = 0;
	a->size        = 0;
}

/** Destructor for {a}. All the {a} contents should not be accessed
 anymore and the {a} takes no memory.
 @param a: If null, does nothing.
 @order \Theta(1)
 @allow */
static void T_(Array_)(struct T_(Array) *const a) {
	if(!a) return;
	free(a->data);
	PT_(array)(a);
}

/** Initialises {a} to be empty. If it is {static} data then it is
 initialised by default and one doesn't have to call this.
 @order \Theta(1)
 @allow */
static void T_(Array)(struct T_(Array) *const a) {
	if(!a) return;
	PT_(array)(a);
}

/** @return The size.
 @order O(1)
 @allow */
static size_t T_(ArraySize)(const struct T_(Array) *const a) {
	if(!a) return 0;
	return a->size;
}

#ifndef ARRAY_STACK /* <-- !stack */
/** Removes {data} from {a}. Only if not {ARRAY_STACK}.
 @param a, data: If null, returns false.
 @param data: Will be removed; data will remain the same but be updated to the
 next element, (ARRAY_TAIL_DELETE causes the next element to be the tail,) or
 if this was the last element, the pointer will be past the end.
 @return Success, otherwise {errno} will be set for valid input.
 @throws EDOM: {data} is not part of {a}.
 @order Amortised O(1) if {ARRAY_FREE_LIST} is defined, otherwise, O(n).
 @fixme Test on stack.
 @allow */
static int T_(ArrayRemove)(struct T_(Array) *const a, T *const data) {
	size_t n;
	if(!a || !data) return 0;
	if(data < a->data
		|| (n = data - a->data) >= a->size) return errno = EDOM, 0;
#ifdef ARRAY_TAIL_DELETE /* <-- tail */
	if(--a->size != n) memcpy(data, a->data + a->size, sizeof *data);
#else /* tail -->< !tail */
	memmove(data, data + 1, sizeof *data * (--a->size - n));
#endif /* !tail --> */
	return 1;
}
#endif /* !stack --> */

/** Removes all from {a}, but leaves the {a} memory alone; if one wants
 to remove memory, see \see{Array_}.
 @param a: If null, does nothing.
 @order \Theta(1)
 @allow */
static void T_(ArrayClear)(struct T_(Array) *const a) {
	if(!a) return;
	a->size = 0;
}

/** Gets an existing element by index. Causing something to be added to the
 {<T>Array} may invalidate this pointer, see \see{<T>ArrayUpdateNew}.
 @param a: If null, returns null.
 @param idx: Index.
 @return If failed, returns a null pointer.
 @order \Theta(1)
 @allow */
static T *T_(ArrayGet)(const struct T_(Array) *const a, const size_t idx) {
	return a ? idx < a->size ? a->data + idx : 0 : 0;
}

/** Gets an index given {data}.
 @param data: If the element is not part of the {Array}, behaviour is undefined.
 @return An index.
 @order \Theta(1)
 @fixme Untested.
 @allow */
static size_t T_(ArrayIndex)(const struct T_(Array) *const a,
	const T *const data) {
	return data - a->data;
}

/** @param a: If null, returns null.
 @return The last element or null if the a is empty. Causing something to be
 added to the {array} may invalidate this pointer.
 @order \Theta(1)
 @fixme Untested.
 @allow */
static T *T_(ArrayPeek)(const struct T_(Array) *const a) {
	if(!a || !a->size) return 0;
	return a->data + a->size - 1;
}

/** The same value as \see{<T>ArrayPeek}.
 @param a: If null, returns null.
 @return Value from the the top of the {a} that is removed or null if the
 stack is empty. Causing something to be added to the {a} may invalidate
 this pointer. See \see{<T>ArrayUpdateNew}.
 @order \Theta(1)
 @allow */
static T *T_(ArrayPop)(struct T_(Array) *const a) {
	if(!a || !a->size) return 0;
	return a->data + --a->size;
}

/** Provides a way to iterate through the {a}. It is safe to add using
 \see{<T>ArrayUpdateNew} with the return value as {update}. Removing an element
 causes the pointer to go to the next element, if it exists.
 @param a: If null, returns null. If {prev} is not from this {a} and not
 null, returns null.
 @param prev: Set it to null to start the iteration.
 @return A pointer to the next element or null if there are no more.
 @order \Theta(1)
 @allow */
static T *T_(ArrayNext)(const struct T_(Array) *const a, T *const prev) {
	T *data;
	size_t idx;
	if(!a) return 0;
	if(!prev) {
		data = a->data;
		idx = 0;
	} else {
		data = prev + 1;
		idx = (size_t)(data - a->data);
	}
	return idx < a->size ? data : 0;
}

/** Called from \see{<T>ArrayNew} and \see{<T>ArrayUpdateNew}. */
static T *PT_(new)(struct T_(Array) *const a, T **const update_ptr) {
	assert(a);
	if(!PT_(reserve)(a, a->size + 1, update_ptr)) return 0;
	return a->data + a->size++;
}

/** Gets an uninitialised new element. May move the {Array} to a new memory
 location to fit the new size.
 @param a: If is null, returns null.
 @return A new, un-initialised, element, or null and {errno} may be set.
 @throws ERANGE: Tried allocating more then can fit in {size_t} objects.
 @throws {realloc} errors: {IEEE Std 1003.1-2001}.
 @order amortised O(1)
 @allow */
static T *T_(ArrayNew)(struct T_(Array) *const a) {
	if(!a) return 0;
	return PT_(new)(a, 0);
}

/** Gets an uninitialised new element and updates the {update_ptr} if it is
 within the memory region that was changed to accomidate new space. For
 example, when iterating a pointer and new element is needed that could change
 the pointer.
 @param a: If null, returns null.
 @param update_ptr: Pointer to update on memory move.
 @return A new, un-initialised, element, or null and {errno} may be set.
 @throws ERANGE: Tried allocating more then can fit in {size_t}.
 @throws {realloc} errors: {IEEE Std 1003.1-2001}.
 @order amortised O(1)
 @fixme Untested.
 @allow */
static T *T_(ArrayUpdateNew)(struct T_(Array) *const a,
	T **const update_ptr) {
	if(!a) return 0;
	return PT_(new)(a, update_ptr);
}

/** Ensures that {a} array is {buffer} capacity beyond the elements in the
 array.
 @param a: If is null, returns null.
 @param buffer: If this is zero, returns null.
 @return One past the end of the array, or null and {errno} may be set.
 @throws ERANGE
 @throws realloc
 @order amortised O({buffer})
 @fixme Test.
 @allow */
static T *T_(ArrayBuffer)(struct T_(Array) *const a, const size_t buffer) {
	if(!a || !buffer || !PT_(reserve)(a, a->size + buffer, 0)) return 0;
	return a->data + a->size;
}

/** Adds {add} to the size in {a}.
 @return Success.
 @throws ERANGE: The size added is greater than the capacity. To avoid this,
 call \see{<T>ArrayBuffer} before.
 @order O(1)
 @fixme Test.
 @allow */
static int T_(ArrayAddSize)(struct T_(Array) *const a, const size_t add) {
	if(!a) return 0;
	if(add > a->capacity[0] || a->size > a->capacity[0] - add)
		return errno = ERANGE, 0;
	a->size += add;
	return 1;
}

/** Iterates though {a} from the bottom and calls {action} on all the
 elements. The topology of the list can not change while in this function.
 That is, don't call \see{<T>ArrayNew}, \see{<T>ArrayRemove}, {etc} in
 {action}.
 @param stack, action: If null, does nothing.
 @order O({size} \times {action})
 @fixme Untested.
 @fixme Sequence interface.
 @allow */
static void T_(ArrayForEach)(struct T_(Array) *const a,
	const PT_(Action) action) {
	T *t, *end;
	if(!a || !action) return;
	for(t = a->data, end = t + a->size; t < end; t++) action(t);
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
static void array_super_cat_init(struct Array_SuperCat *const cat,
	char *const print, const size_t print_size) {
	cat->print = cat->cursor = print;
	cat->left = print_size;
	cat->is_truncated = 0;
	print[0] = '\0';
}
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
 {ARRAY_TO_STRING} to a function implementing {<T>ToString} to get this
 functionality.
 @return Prints {a} in a static buffer.
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

#ifdef ARRAY_TEST /* <-- test */
#include "../test/TestArray.h" /* Need this file if one is going to run tests.*/
#endif /* test --> */

/* Prototype. */
static void PT_(unused_coda)(void);
/** This silences unused function warnings from the pre-processor, but allows
 optimisation, (hopefully.)
 \url{ http://stackoverflow.com/questions/43841780/silencing-unused-static-function-warnings-for-a-section-of-code } */
static void PT_(unused_set)(void) {
	T_(Array_)(0);
	T_(Array)(0);
	T_(ArraySize)(0);
#ifndef ARRAY_STACK /* <-- !stack */
	T_(ArrayRemove)(0, 0);
#endif /* !stack --> */
	T_(ArrayClear)(0);
	T_(ArrayGet)(0, 0);
	T_(ArrayIndex)(0, 0);
	T_(ArrayPeek)(0);
	T_(ArrayPop)(0);
	T_(ArrayNext)(0, 0);
	T_(ArrayNew)(0);
	T_(ArrayUpdateNew)(0, 0);
	T_(ArrayBuffer)(0, 0);
	T_(ArrayAddSize)(0, 0);
	T_(ArrayForEach)(0, 0);
#ifdef ARRAY_TO_STRING
	T_(ArrayToString)(0);
#endif
	PT_(unused_coda)();
}
/** {clang}'s pre-processor is not fooled if you have one function. */
static void PT_(unused_coda)(void) { PT_(unused_set)(); }



/* Un-define all macros. */
#undef ARRAY_NAME
#undef ARRAY_TYPE
/* Undocumented; allows nestled inclusion so long as: {CAT_}, {CAT}, {PCAT},
 {PCAT_} conform, and {T} is not used. */
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
#ifdef ARRAY_STACK
#undef ARRAY_STACK
#endif
#ifdef ARRAY_TAIL_DELETE
#undef ARRAY_TAIL_DELETE
#endif
#ifdef ARRAY_TO_STRING
#undef ARRAY_TO_STRING
#endif
#ifdef ARRAY_TEST
#undef ARRAY_TEST
#endif
#ifdef ARRAY_NDEBUG
#undef ARRAY_NDEBUG
#undef NDEBUG
#endif
