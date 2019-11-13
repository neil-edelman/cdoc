/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT). */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "Path.h"

static const char *const twodots = "..", *const dot = ".", dirsep = '/',
	*const notallowed = "//";

#define ARRAY_NAME Path
#define ARRAY_TYPE const char *
#include "Array.h"

#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "Array.h"

static void print_path(const struct PathArray *const path, FILE *const fp) {
	const char **p;
	assert(fp);
	if(!path || !(p = PathArrayNext(path, 0))) return;
	for( ; ; ) {
		fputs(*p, fp);
		if(!(p = PathArrayNext(path, p))) break;
		fputc(dirsep, fp);
	}
}

/** Renders the `path` to `str`.
 @param[str] The path container.
 @param[path] The path.
 @return The string representing the path.
 @throws[malloc] */
static const char *path_to_string(struct CharArray *const str,
	const struct PathArray *const path) {
	const char **p = 0;
	char *buf;
	size_t len;
	assert(str && path);
	CharArrayClear(str);
	if(!(p = PathArrayNext(path, 0))) goto terminate;
	for( ; ; ) {
		/* Print the next part. */
		if((len = strlen(*p))) {
			if(!(buf = CharArrayBuffer(str, len))) return 0;
			memcpy(buf, *p, len), CharArrayExpand(str, len);
		}
		/* Is it at the end. */
		if(!(p = PathArrayNext(path, p))) break;
		/* Otherwise stick a `dirsep`. */
		if(!(buf = CharArrayNew(str))) return 0;
		*buf = dirsep;
	}
terminate:
	if(!(buf = CharArrayNew(str))) return 0;
	*buf = '\0';
	return CharArrayGet(str);
}

static int relative_path(const char *string) {
	int is_last_slash = 1;
	assert(string);
	while(*string != '\0') {
		/* Must be something between slashes. */
		if(*string == dirsep) {
			if(is_last_slash) return 0;
			is_last_slash = 1;
		} else {
			is_last_slash = 0;
		}
		string++;
	}
	return 1;
}

static int looks_like_path(const char *string) {
	assert(string);
	return !strstr(string, notallowed);
}

/** Appends `path` split on `dirsep` to `args`.
 @param[string] Modified and referenced; if it goes out-of-scope or changes,
 undefined behaviour will result.
 @return Success. */
static int sep_path(struct PathArray *const path, char *string) {
	const char **arg = 0;
	char *p = string;
	assert(path);
	if(!string) return 1;
	for( ; ; ) {
		if(!(arg = PathArrayNew(path))) return 0;
		*arg = p;
		if(!(p = strchr(p, dirsep))) break;
		*p++ = '\0';
	}
	return 1;
}

/** "<path>/[<file>]" -> "<path>"; really, it pops the file, so just use it
 once. */
static void strip_path(struct PathArray *const args) {
	assert(args);
	PathArrayPop(args);
}

static int cat_path(struct PathArray *const path,
	const struct PathArray *const cat) {
	assert(path && cat);
	return PathArraySplice(path, 0, 0, cat);
}

static void simplify_path(struct PathArray *const path) {
	const char **p0 = 0, **p1, **p2;
	assert(path);
	while((p1 = PathArrayNext(path, p0))) {
		/* "./" -> "" */
		if(strcmp(dot, *p1) == 0) { PathArrayRemove(path, p1); continue; }
		/* "<dir>/../" -> "" */
		if((p2 = PathArrayNext(path, p1)) && strcmp(twodots, *p1) != 0
			&& strcmp(twodots, *p2) == 0) { PathArraySplice(path, p1, 2, 0);
			continue; }
		p0 = p1;
	}
}

/** Should do `strip_path` as needed, `simplify_path` first. */
static int inverse_path(struct PathArray *const path,
	const struct PathArray *const inv) {
	const char **p = 0;
	size_t inv_size = PathArraySize(inv);
	assert(path && inv);
	PathArrayClear(path);
	/* The ".." is not an invertable operation; we may be lazy and require "."
	 to not be there, too, then we can just count. */
	while((p = PathArrayNext(inv, p))) if(strcmp(dot, *p) == 0
		|| strcmp(twodots, *p) == 0) return fprintf(stderr,
		"inverse_path: \"..\" is not surjective.\n"), 0;
	if(!PathArrayBuffer(path, inv_size)) return 0;
	while((inv_size)) p = PathArrayNew(path), *p = twodots, inv_size--;
	return 1;
}

/* Includes it's own buffer. */
struct PathExtra {
	struct CharArray buffer;
	struct PathArray path;
};

static struct {
	struct PathExtra input, output, working;
	struct PathArray outinv;
	struct CharArray result;
} paths;

static void clear_paths(void) {
	PathArray_(&paths.input.path), CharArray_(&paths.input.buffer);
	PathArray_(&paths.output.path), CharArray_(&paths.output.buffer);
	PathArray_(&paths.working.path), CharArray_(&paths.working.buffer);
	PathArray_(&paths.outinv);
	CharArray_(&paths.result);
}

/** Helper for <fn:Path>. Puts `string` into `extra`. */
static int extra_path(struct PathExtra *const extra, const char *const string) {
	size_t string_size;
	assert(extra);
	PathArrayClear(&extra->path), CharArrayClear(&extra->buffer);
	if(!string) return 1;
	if(!looks_like_path(string)) return fprintf(stderr,
		"%s: does not appear to be a path.\n", string), 1;
	string_size = strlen(string) + 1;
	if(!CharArrayBuffer(&extra->buffer, string_size)) return 0;
	memcpy(CharArrayGet(&extra->buffer), string, string_size);
	CharArrayExpand(&extra->buffer, string_size);
	if(!sep_path(&extra->path, CharArrayGet(&extra->buffer))) return 0;
	strip_path(&extra->path);
	simplify_path(&extra->path);
	fputs("Path from extra_path: ", stderr), print_path(&extra->path, stderr), fputc('\n', stderr);
	return 1;
}

/** Clears all the data. */
void Paths_(void) {
	fprintf(stderr, "Paths_()\n");
	clear_paths();
}

/** Sets up `in_fn` and `out_fn` as directories.
 @return Success. */
int Paths(const char *const in_fn, const char *const out_fn) {
	fprintf(stderr, "Paths(\"%s\", \"%s\")\n", in_fn, out_fn);
	if(!extra_path(&paths.input, in_fn) || !extra_path(&paths.output, out_fn)
		|| (!inverse_path(&paths.outinv, &paths.output.path) && errno))
		return clear_paths(), 0;
	fputs("Input: ", stderr), print_path(&paths.input.path, stderr),
		fputc('\n', stderr);
	fputs("Output: ", stderr), print_path(&paths.output.path, stderr),
		fputc('\n', stderr);
	fputs("OutInv: ", stderr), print_path(&paths.outinv, stderr),
		fputc('\n', stderr);
	return 1;
}

/** Appends output directory to `fn`:`fn_name`, (if it exists.)
 @return A temporary path, invalid on calling any path function.
 @throws[malloc] */
const char *PathsFromHere(const size_t fn_len, const char *const fn) {
	CharArrayClear(&paths.working.buffer), PathArrayClear(&paths.working.path);
	cat_path(&paths.working.path, &paths.input.path);
	if(fn) {
		char *workfn;
		/* Copy the argument. */
		if(!CharArrayBuffer(&paths.working.buffer, fn_len + 1)) return 0;
		workfn = CharArrayGet(&paths.working.buffer);
		memcpy(workfn, fn, fn_len), workfn[fn_len] = '\0';
		/* Break it up and stick it on the path. */
		if(!relative_path(workfn) || !sep_path(&paths.working.path, workfn))
			return 0;
	}
	simplify_path(&paths.working.path);
	return path_to_string(&paths.result, &paths.working.path);
}
