/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT). */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h> /* tolower */
#include "Cdoc.h"
#include "Path.h"

#define ARRAY_NAME Path
#define ARRAY_TYPE const char *
#include "Array.h"

#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "Array.h"

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
		*buf = *path_dirsep;
	}
terminate:
	if(!(buf = CharArrayNew(str))) return 0;
	*buf = '\0';
	return CharArrayGet(str);
}

/** Checks for not "//", which is not a path, except after '?#'. */
static int looks_like_path(const char *const string) {
	const char *s, *q;
	assert(string);
	if(!(s = strstr(string, path_notallowed))) return 1;
	if(!(q = strpbrk(string, path_fragment))) return 0;
	return s < q ? 0 : 1;
}

/** Checks for not "//" as well as not "/" at the beginning. */
static int looks_like_relative_path(const char *const string) {
	assert(string);
	return string[0] == '\0'
		|| (string[0] != *path_dirsep && looks_like_path(string));
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
		/* This searches for '/' until hitting a '?#'. */
		if(!(p = strpbrk(p, path_searchdirsep)) || strchr(path_fragment, *p))
			break;
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

/** I think we can only do this, but paths will be, in `foo`, `../foo/bar`, but
 that is probably using more information? */
static void simplify_path(struct PathArray *const path) {
	const char **p0 = 0, **p1, **p2;
	assert(path);
	while((p1 = PathArrayNext(path, p0))) {
		/* "./" -> "" */
		if(strcmp(path_dot, *p1) == 0) { PathArrayRemove(path, p1); continue; }
		/* "<dir>/../" -> "" */
		if((p2 = PathArrayNext(path, p1)) && strcmp(path_twodots, *p1) != 0
			&& strcmp(path_twodots, *p2) == 0)
			{ PathArraySplice(path, p1, 2, 0); continue; }
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
	while((p = PathArrayNext(inv, p))) if(strcmp(path_dot, *p) == 0
		|| strcmp(path_twodots, *p) == 0) return fprintf(stderr,
		"inverse_path: \"..\" is not surjective.\n"), 0;
	if(!PathArrayBuffer(path, inv_size)) return 0;
	while((inv_size)) p = PathArrayNew(path), *p = path_twodots, inv_size--;
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
	return 1;
}

/** Clears all the data. */
void Paths_(void) { clear_paths(); }

/** Sets up `in_fn` and `out_fn` as directories.
 @return Success. */
int Paths(const char *const in_fn, const char *const out_fn) {
	if(!extra_path(&paths.input, in_fn) || !extra_path(&paths.output, out_fn)
		|| (!inverse_path(&paths.outinv, &paths.output.path) && errno))
		return clear_paths(), 0;
	return 1;
}

/** Helper; clobbers the working path buffer.
 @return It may not set `errno` and return 0 if the path is messed. */
static int append_working_path(const size_t fn_len, const char *const fn) {
	char *workfn;
	assert(fn);
	CharArrayClear(&paths.working.buffer);
	if(!(workfn = CharArrayBuffer(&paths.working.buffer, fn_len + 1))) return 0;
	memcpy(workfn, fn, fn_len), workfn[fn_len] = '\0';
	CharArrayExpand(&paths.working.buffer, fn_len + 1);
	if(!looks_like_relative_path(workfn)
		|| !sep_path(&paths.working.path, workfn)) return 0;
	return 1;
}

/* ?# are special url encoding things, so make sure they're not included in
 the filename if we are going to open it. */
static size_t strip_query_fragment(const size_t uri_len, const char *const uri)
{
	const char *u = uri;
	size_t without = 0;
	assert(uri);
	while(without < uri_len) {
		if(!*u) { assert(0); break; } /* Cannot happen on well-formed input. */
		if(strchr(path_fragment, *u)) break;
		u++, without++;
	}
	return without;
}

/** @return True if the first character in `fn`:`fn_len` is `?` or `#`. */
static int looks_like_fragment(const size_t fn_len, const char *const fn) {
	if(!fn_len) return 0;
	return !!strchr(path_fragment, fn[0]);
}

/** Appends output directory to `fn`:`fn_name`, (if it exists.) For opening.
 @return A temporary path, invalid on calling any path function.
 @throws[malloc] */
const char *PathsFromHere(const size_t fn_len, const char *const fn) {
	if(looks_like_fragment(fn_len, fn)) return 0;
	PathArrayClear(&paths.working.path);
	if(!cat_path(&paths.working.path, &paths.input.path)
		|| (fn && !append_working_path(strip_query_fragment(fn_len, fn), fn)))
		return 0;
	simplify_path(&paths.working.path);
	return path_to_string(&paths.result, &paths.working.path);
}

/** Appends inverse output directory and input directory to `fn`:`fn_name`, (if
 it exists.) It may return 0 if the path is weird, set `errno` before.
 @return A temporary path, invalid on calling any path function.
 @throws[malloc] */
const char *PathsFromOutput(const size_t fn_len, const char *const fn) {
	if(looks_like_fragment(fn_len, fn)) return 0;
	PathArrayClear(&paths.working.path);
	if(!cat_path(&paths.working.path, &paths.outinv)
		|| !cat_path(&paths.working.path, &paths.input.path)
		|| (fn && !append_working_path(fn_len, fn))) return 0;
	simplify_path(&paths.working.path);
	return path_to_string(&paths.result, &paths.working.path);
}

/** This adds on to the buffer in `paths.working.buffer` a safe anchor string
 (for GitHub.)
 @return Success; if not, error and the string may be in an intermediate state
 and not null-terminated.

 * This uses a small part of GitHub's rendering engine contained in
 * `html.c:rndr_header_anchor`. Why do they do they break html like this? Ask
 * GitHub.
 *
 * @license
 * Copyright (c) 2009, Natacha Porté
 * Copyright (c) 2015, Vicent Marti
 *
 * @licence
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * @license
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * @license
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
static int github_anchor_working_cat(const char *const a) {
	static const char *strip = " -&+$,/:;=?@\"#{}|^~[]`\\*()%.!'";
	struct CharArray *buffer = &paths.working.buffer;
	const size_t size = strlen(a);
	size_t i = 0;
	int stripped = 0, inserted = 0;
	{ /* If it's null-terminated, overwrite null. */
		char *zero = CharArrayPeek(buffer);
		if(zero && *zero == '\0') CharArrayPop(buffer);
	}
	/* Upper bound on the size; we don't need to worry about new. */
	if(!CharArrayBuffer(buffer, size + 1)) return 0;
	for( ; i < size; ++i) {
		if(a[i] == '<') {
			/* Skip html tags. */
			while(i < size && a[i] != '>') i++;
		} else if(a[i] == '&') {
			/* Skip html entities. */
			while (i < size && a[i] != ';') i++;
		} else if(!isascii(a[i]) || strchr(strip, a[i])) {
			/* Replace non-ascii or invalid characters with dashes. */
			if(inserted && !stripped) *CharArrayNew(buffer) = '-';
			/* And do it only once. */
			stripped = 1;
		} else {
			*CharArrayNew(buffer) = tolower(a[i]);
			stripped = 0;
			inserted++;
		}
	}
	/* Replace the last dash if there was anything added. */
	if(stripped && inserted) CharArrayPop(buffer);
	/* If anchor found empty, use djb2 hash for it. */
	if(!inserted && size) {
		char z[256];
		size_t z_len;
		unsigned long hash = 5381;
		/* h * 33 + c */
		for (i = 0; i < size; ++i) hash = ((hash << 5) + hash) + a[i];
		/* We are too lazy to implement `CharArrayPrint`, just use an
		 intermediary. fixme: That would be cool to add in `Array.h`. */
		sprintf(z, "part-%lx", hash);
		z_len = strlen(z);
		if(!CharArrayBuffer(buffer, z_len + 1)) return 0;
		memcpy(CharArrayEnd(buffer), z, z_len);
		CharArrayExpand(buffer, z_len);
	}
	*CharArrayNew(buffer) = '\0';
	return 1;
}

/*const char *PathsBuildAnchor(, const int is_href) {
}*/

#if 0
int PathsAnchorClear(void) {
	char *c;
	CharArrayClear(&paths.working.buffer);
	if(!(c = CharArrayNew(&paths.working.buffer))) return 0;
	*c = '#';
}
int PathsAnchorCat(const char *str) {
	const size_t len = strlen(str);
	if(!CharArrayBuffer(&paths.working.buffer, len)) return 0;
	memcpy(paths.working.buffer, str, len);
	CharArrayExpand(&paths.working.buffer, len);
}

/** Safe fragment name for whatever platform, (_aka_ GitHub.) This is a
 surjection, so don't rely on non-letter symbols to make the name unique.
 Doesn't include the `#`.
 @return A temporary name fragment made up from `fn`:`fn_len`, invalid on
 calling any path function. It does include the `#`
 @throws[malloc] */
const char *PathsSafeAnchor(const size_t fn_len, const char *const fn) {
	CharArrayBuffer(&paths.working.buffer, fn_len + 1); /* Approximate. */
	if(!CdocGetGithub()) return fn;
	fprintf(stderr, "fixme: invert %.*s.\n", (int)fn_len, fn);
	return 0;
}

const char *PathsSafeFragment(const size_t fn_len, const char *const fn) {
	return 0;
}
#endif
