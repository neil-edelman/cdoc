/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 Singleton temporary strings for working with urls. The constructor <fn:url>,
 destructor <fn:url_>. @fixme Should be a child of text. */

#include "url.h"
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define ARRAY_NAME char
#define ARRAY_TYPE char
#include "array.h"

#define ARRAY_NAME string
#define ARRAY_TYPE char *
#include "array.h"

/** @return A temporary string representing the `url` inside `str`.
 @throws[malloc] */
static const char *url_to_string(struct char_array *const str,
	const struct string_array *const url) {
	size_t i;
	char *buf;
	size_t len;
	assert(str && url);
	char_array_clear(str);
	if(!url->size) goto finally;
	for(i = 0; ; ) {
		char **s = url->data + i;
		/* Print the next part. */
		if((len = strlen(*s))) {
			if(!(buf = char_array_buffer(str, len))) return 0;
			memcpy(buf, *s, len);
		}
		/* Is it at the end. */
		if(++i == url->size) break;
		/* Otherwise stick a `dirsep`. */
		if(!(buf = char_array_new(str))) return 0;
		*buf = *url_dirsep;
	}
finally:
	if(!(buf = char_array_new(str))) return 0;
	*buf = '\0';
	return str->data;
}

/** Checks for not "//", which is not a url, except after '?#'. */
static int looks_like_url(const char *const string) {
	const char *s, *q;
	assert(string);
	if(!(s = strstr(string, url_relnotallowed))) return 1;
	if(!(q = strpbrk(string, url_subordinate))) return 0;
	return s < q ? 0 : 1;
}

/** Checks for not "//" as well as not "/" at the beginning. */
static int looks_like_relative_url(const char *const string) {
	assert(string);
	return string[0] == '\0'
		|| (string[0] != *url_dirsep && looks_like_url(string));
}

/** Appends `url` split on `dirsep` to `args`.
 @param[string] Modified and referenced; if it goes out-of-scope or changes,
 undefined behaviour will result.
 @return Success. */
static int sep_url(struct string_array *const url, char *string) {
	char **arg = 0;
	char *p = string;
	assert(url);
	if(!string) return 1;
	for( ; ; ) {
		if(!(arg = string_array_new(url))) return 0;
		*arg = p;
		/* This searches for '/' until hitting a '?#'. */
		if(!(p = strpbrk(p, url_searchdirsep)) || strchr(url_subordinate, *p))
			break;
		*p++ = '\0';
	}
	return 1;
}

/** "<url>/[<file>]" -> "<url>"; really, it pops the file, so just use it
 once. */
static void strip_url(struct string_array *const args) {
	assert(args);
	string_array_pop(args);
}

/** Cat `url`(`cat`). */
static int cat_url(struct string_array *const url,
	const struct string_array *const cat) {
	assert(url && cat);
	return string_array_splice(url, 0, 0, cat);
}

/** I think we can only do this, but urls will be, in `foo`, `../foo/bar`, but
 that is probably using more information? */
static void simplify_url(struct string_array *const url) {
	char **p1, **p2;
	size_t i;
	assert(url);
	if(url->size < 2) return;
	for(i = 1; ; i++) {
		p1 = url->data + i - 1, p2 = url->data + i;
		/* "./" -> "" */
		if(strcmp(url_dot, *p1) == 0)
			{ string_array_remove(url, p1); continue; }
		if(i + 1 == url->size) break;
		/* "<dir>/../" -> "" */
		if(strcmp(url_twodots, *p1) != 0
			&& strcmp(url_twodots, p2[1]) == 0)
			string_array_splice(url, i - 1, 2, 0);
	}
}

/** Should do `strip_url` as needed, `simplify_url` first. */
static int inverse_url(struct string_array *const url,
	const struct string_array *const inv) {
	char **p;
	size_t i;
	assert(url && inv);
	string_array_clear(url);
	/* The ".." is not an invertible operation; we may be lazy and require "."
	 to not be there, too, then we can just count. */
	for(i = 0; i < inv->size; i++) {
		p = inv->data + i;
		if(strcmp(url_dot, *p) == 0
		|| strcmp(url_twodots, *p) == 0) return fprintf(stderr,
		"inverse_url: \"..\" is not surjective.\n"), 0;
	}
	if(!string_array_buffer(url, i = inv->size)) return 0;
	while(i) p = string_array_new(url), *p = url_twodots, i--;
	return 1;
}

/* Includes it's own buffer. */
struct url_extra {
	struct char_array buffer;
	struct string_array url;
};

static struct {
	struct url_extra input, output, working;
	struct string_array outinv;
	struct char_array result;
} urls;

static void clear_urls(void) {
	string_array_(&urls.input.url), char_array_(&urls.input.buffer);
	string_array_(&urls.output.url), char_array_(&urls.output.buffer);
	string_array_(&urls.working.url), char_array_(&urls.working.buffer);
	string_array_(&urls.outinv);
	char_array_(&urls.result);
}

/** Helper for <fn:url>. Puts `string` into `extra`. */
static int extra_url(struct url_extra *const extra, const char *const string) {
	size_t string_size;
	char *buf;
	assert(extra);
	string_array_clear(&extra->url), char_array_clear(&extra->buffer);
	if(!string) return 1;
	if(!looks_like_url(string)) return fprintf(stderr,
		"%s: does not appear to be a url.\n", string), 1;
	assert(strlen(string) < (size_t)-1);
	string_size = strlen(string) + 1;
	if(!(buf = char_array_buffer(&extra->buffer, string_size))) return 0;
	memcpy(buf, string, string_size);
	assert(string[string_size - 1] == '\0');
	if(!sep_url(&extra->url, buf)) return 0;
	strip_url(&extra->url);
	simplify_url(&extra->url);
	return 1;
}

/** Clears all the data. */
void url_(void) { clear_urls(); }

/** Sets up `in_fn` and `out_fn` as directories for the url.
 @return Success. */
int url(const char *const in_fn, const char *const out_fn) {
	if(!extra_url(&urls.input, in_fn) || !extra_url(&urls.output, out_fn)
		|| (!inverse_url(&urls.outinv, &urls.output.url) && errno))
		return clear_urls(), 0;
	return 1;
}

/** Helper; clobbers the working url buffer.
 @return It may not set `errno` and return 0 if the url is messed. */
static int append_working_url(const size_t fn_len, const char *const fn) {
	char *workfn;
	assert(fn);
	char_array_clear(&urls.working.buffer);
	if(!(workfn = char_array_buffer(&urls.working.buffer, fn_len + 1))) return 0;
	memcpy(workfn, fn, fn_len), workfn[fn_len] = '\0';
	if(!looks_like_relative_url(workfn)
		|| !sep_url(&urls.working.url, workfn)) return 0;
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
		if(strchr(url_subordinate, *u)) break;
		u++, without++;
	}
	return without;
}

/** @return True if the first character in `fn`:`fn_len` is `?` or `#`. */
static int looks_like_fragment(const size_t fn_len, const char *const fn) {
	if(!fn_len) return 0;
	return !!strchr(url_subordinate, fn[0]);
}

/** Appends output directory to `fn`:`fn_name`, (if it exists.) For opening.
 @return A temporary url, invalid on calling any url function.
 @throws[malloc] */
const char *url_from_here(const size_t fn_len, const char *const fn) {
	if(looks_like_fragment(fn_len, fn)) return 0;
	string_array_clear(&urls.working.url);
	if(!cat_url(&urls.working.url, &urls.input.url)
		|| (fn && !append_working_url(strip_query_fragment(fn_len, fn), fn)))
		return 0;
	simplify_url(&urls.working.url);
	return url_to_string(&urls.result, &urls.working.url);
}

/** Appends inverse output directory and input directory to `fn`:`fn_name`, (if
 it exists.) It may return 0 if the url is weird, set `errno` before.
 @return A temporary url, invalid on calling any url function.
 @throws[malloc] */
const char *url_from_output(const size_t fn_len, const char *const fn) {
	/* If it's a absolute url to input, (that we've been given,) the best we
	 could do is <fn:url_from_here>. */
	if(urls.input.buffer.size
		&& urls.input.buffer.data[0] == '\0')
		return url_from_here(fn_len, fn);
	if(looks_like_fragment(fn_len, fn)) return 0;
	string_array_clear(&urls.working.url);
	if(!cat_url(&urls.working.url, &urls.outinv)
		|| !cat_url(&urls.working.url, &urls.input.url)
		|| (fn && !append_working_url(fn_len, fn))) return 0;
	simplify_url(&urls.working.url);
	return url_to_string(&urls.result, &urls.working.url);
}

/** Is it a fragment? This accesses only the first character. */
int url_is_fragment(const char *const str) {
	if(!str) return 0;
	return *str == *url_fragment;
}
