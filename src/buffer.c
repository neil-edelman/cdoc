/** @license 2017 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 Handles strings. */

#include "buffer.h"
#include <assert.h>

#define ARRAY_NAME char
#define ARRAY_TYPE char
#include "array.h"

#define ARRAY_NAME string
#define ARRAY_TYPE char *
#include "array.h"


/* This is a temporary double-buffer. */

static struct char_array buffers[2], *buffer = buffers;

/** Destructor for both buffers. */
void buffer_(void) {
	char_array_(buffers + 0);
	char_array_(buffers + 1);
}

/** Get the selected buffer as a constant string. Always return a string. */
const char *buffer_get(void) {
	if(!buffer->size) return "";
	return buffer->data;
}

/** Sets the selected buffer's length to zero. */
void buffer_clear(void) { char_array_clear(buffer); }

/** Expands the buffer to include `length`, (not including the terminating
 null.) This must be followed by a write of `length`.
 @param[length] The length the string, not including the terminating null.
 @return The buffer position that is appropriate to place a `char` array of
 `length`. `length + 1` is a null terminator which one could overwrite or
 not. */
char *buffer_prepare(const size_t length) {
	const char *const last = char_array_peek(buffer);
	int is_null_term = !!last && *last == '\0';
	char *s;
	/* fixme: this is stupid. */
	assert(length + !is_null_term >= length);
	if(!(s = char_array_append(buffer, length + !is_null_term))) return 0;
	*char_array_peek(buffer) = '\0';
	return s - is_null_term;
}

/** Switch the buffers so one can compare the two. */
void buffer_swap(void) { buffer = buffers + !(int)(buffer - buffers); }


/* Handles reading entire text files and keeping them in memory. */

#include <stdio.h>  /* FILE fopen fclose fread */
#include <string.h> /* memcpy strrchr strlen */
#include <stdlib.h> /* malloc free */
#include <errno.h>  /* errno EILSEQ */

struct text { struct char_array buffer; char *filename, *basename; };

/** Unloads `file` from memory. */
static void text_(struct text **const pb) {
	struct text *b;
	if(!pb || !(b = *pb)) return;
	char_array_(&b->buffer);
	free(b);
	*pb = 0;
}

/** @return Reads `fn` to a string as text, (contains no zeros.)
 @throws[fopen, fread, malloc]
 @throws[EILSEQ] If the file has embedded zeros. */
static struct text *text(const char *const fn) {
	FILE *fp = 0;
	struct text *t = 0;
	const size_t granularity = 1024;
	size_t nread, fn_size;
	char *cursor, *base;
	int success = 0;
	if(!fn || !(fp = fopen(fn, "r"))) goto catch;
	fn_size = strlen(fn) + 1;
	if(!(t = malloc(sizeof *t + fn_size))) goto catch;
	char_array(&t->buffer);
	t->filename = (char *)(t + 1), memcpy(t->filename, fn, fn_size);
	t->basename = (base = strrchr(t->filename, *url_dirsep))
		? base + 1 : t->filename;
	/* Read entire file in chunks. */
	do if(!(cursor = char_array_buffer(&t->buffer, granularity))
		|| (nread = fread(cursor, 1, granularity, fp), ferror(fp))
		|| !char_array_append(&t->buffer, nread)) goto catch;
	while(nread == granularity);
	/* File to `C` string. */
	if(!(cursor = char_array_new(&t->buffer))) goto catch;
	*cursor = '\0';
	/* Binary files with embedded '\0' are not allowed. */
	if(strchr(t->buffer.data, '\0') != cursor) { errno = EILSEQ; goto catch; }
	{ success = 1; goto finally; }
catch:
	if(!errno) errno = EILSEQ; /* Will never be true on POSIX. */
	text_(&t), t = 0;
finally:
	if(fp) fclose(fp);
	return t;
}

/** @return The file name of `t` that it was read. */
const char *text_name(const struct text *const t)
	{ return t ? t->filename : 0; }

/** @return The base file name of `t`. */
const char *text_base_name(const struct text *const t)
	{ return t ? t->basename : 0; }

/** @return The length of the contents of `t`. */
size_t text_size(const struct text *const t)
	{ return t && t->buffer.size ? t->buffer.size - 1 : 0; }

/** @return The contents of `t`. */
const char *text_get(const struct text *const t)
	{ return t ? t->buffer.data : 0; }

/* Singleton array of files read from memory. */

#define ARRAY_NAME text
#define ARRAY_TYPE struct text *
#include "array.h"

static struct text_array files;

/** @return Loads new file from `fn` into memory.
 @throws[fopen, fread, malloc]
 @throws[EILSEQ] If the file has embedded zeros. */
struct text *text_open(const char *const fn) {
	struct text **ptext = text_array_new(&files);
	if(!ptext) return 0;
	if(!(*ptext = text(fn))) { text_array_pop(&files); return 0; }
	return *ptext;
}

/** Unloads all texts. */
void text_close_all(void) {
	struct text **ptext;
	while((ptext = text_array_pop(&files))) text_(ptext);
	text_array_(&files);
}


/* Singleton temporary strings for working with urls. The constructor <fn:url>,
 destructor <fn:url_>. */

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
			if(!(buf = char_array_append(str, len))) return 0;
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
	/* fixme: This is better handled by re2c. */
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

/** Concatenates a copy of `before` in `url`. */
static int prepend_url(struct string_array *const url,
	const struct string_array *const before) {
	assert(url && before);
	return string_array_splice(url, before, 0, 0);
}

/** I think we can only do this, but urls will be, in `foo`, `../foo/bar`, but
 that is probably using more information? */
static void simplify_url(struct string_array *const url) {
	char **p1, **p2;
	size_t i;
	assert(url);
	for(i = 0; i < url->size; i++) {
		p1 = url->data + i;
		/* "./" -> "" */
		if(strcmp(url_dot, *p1) == 0)
			{ string_array_remove(url, p1); continue; }
		if(i + 1 >= url->size) continue;
		p2 = url->data + i + 1;
		/* "<dir>/../" -> "" */
		if(strcmp(url_twodots, *p1) != 0
			&& strcmp(url_twodots, *p2) == 0)
			string_array_splice(url, 0, i, i + 2);
	}
}
#if 0
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
#endif

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
	if(!(buf = char_array_append(&extra->buffer, string_size))) return 0;
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
	if(!(workfn = char_array_append(&urls.working.buffer, fn_len + 1))) return 0;
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
	if(!prepend_url(&urls.working.url, &urls.input.url)
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
	if(!prepend_url(&urls.working.url, &urls.outinv)
		|| !prepend_url(&urls.working.url, &urls.input.url)
		|| (fn && !append_working_url(fn_len, fn))) return 0;
	simplify_url(&urls.working.url);
	return url_to_string(&urls.result, &urls.working.url);
}

/** Is it a fragment? This accesses only the first character. */
int url_is_fragment(const char *const str) {
	if(!str) return 0;
	return *str == *url_fragment;
}

#include <ctype.h>  /* isalnum */
#include <string.h> /* strchr */
#include <errno.h>  /* errno ERANGE */

/** URL-encode roughly uses [PHP](https://www.php.net/license/).

 --------------------------------------------------------------------
 The PHP License, version 3.01
 Copyright (c) 1999 - 2019 The PHP Group. All rights reserved.
 --------------------------------------------------------------------

 Redistribution and use in source and binary forms, with or without
 modification, is permitted provided that the following conditions
 are met:

 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in
 the documentation and/or other materials provided with the
 distribution.

 3. The name "PHP" must not be used to endorse or promote products
 derived from this software without prior written permission. For
 written permission, please contact group@php.net.

 4. Products derived from this software may not be called "PHP", nor
 may "PHP" appear in their name, without prior written permission
 from group@php.net.  You may indicate that your software works in
 conjunction with PHP by saying "Foo for PHP" instead of calling
 it "PHP Foo" or "phpfoo"

 5. The PHP Group may publish revised and/or new versions of the
 license from time to time. Each version will be given a
 distinguishing version number.
 Once covered code has been published under a particular version
 of the license, you may always continue to use it under the terms
 of that version. You may also choose to use such covered code
 under the terms of any subsequent version of the license
 published by the PHP Group. No one other than the PHP Group has
 the right to modify the terms applicable to covered code created
 under this License.

 6. Redistributions of any form whatsoever must retain the following
 acknowledgment:
 "This product includes PHP software, freely available from
 <http://www.php.net/software/>".

 THIS SOFTWARE IS PROVIDED BY THE PHP DEVELOPMENT TEAM ``AS IS'' AND
 ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PHP
 DEVELOPMENT TEAM OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.

 --------------------------------------------------------------------

 This software consists of voluntary contributions made by many
 individuals on behalf of the PHP Group.

 The PHP Group can be contacted via Email at group@php.net.

 For more information on the PHP Group and the PHP project,
 please see <http://www.php.net>.

 PHP includes the Zend Engine, freely available at
 <http://www.zend.com>.

 --------------------------------------------------------------------

 URL encode the substring `s` with `length` to a static string of fixed
 maximum length (short.)

 @throws[ERANGE] The string could not be encoded in this length.
 @return A static string or null. */
const char *url_encode(char const *const s, size_t length) {
	/* rfc1738:

	 ...The characters ";",
	 "/", "?", ":", "@", "=" and "&" are the characters which may be
	 reserved for special meaning within a scheme...

	 ...Thus, only alphanumerics, the special characters "$-_.+!*'(),", and
	 reserved characters used for their reserved purposes may be used
	 unencoded within a URL...

	 For added safety, we only leave -_. unencoded.
	 */
	static char hexchars[] = "0123456789ABCDEF";
	static char encoded[64];
	unsigned char c;
	char *to = encoded;
	char const *from = s, *const end = from + length;

	while(from < end) {
		c = (unsigned char)*from++;
		/* `php_raw_url_encode` encodes space as "%20", why? */
		if (c == ' ') {
			*to++ = '+';
		} else if (!isalnum(c) && strchr("_-.", c) == 0) {
			/* Allow only alphanumeric chars and '_', '-', '.'. */
			to[0] = '%';
			to[1] = hexchars[c >> 4];
			to[2] = hexchars[c & 15];
			to += 3;
		} else {
			*to++ = (char)c;
		}
		if((size_t)(to - encoded) > sizeof encoded - 4)
			{ *to = '\0'; errno = ERANGE; return 0; }
	}
	*to = '\0';
	return encoded;
}
