/** @license 2017 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 This provides a space to store temporary strings.

 @std C89 */

#include <stdio.h>  /* fprintf */
#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include <ctype.h>  /* tolower */
#include "Division.h"
#include "Cdoc.h"
#include "Buffer.h"

#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "Array.h"

static struct CharArray buffer;

/** This adds on to the buffer in a safe anchor string, (for GitHub.)
 @return Success; if not, error and the string may be in an intermediate state
 and not null-terminated.
 
 * This uses a small part of GitHub's rendering engine contained in
 * `html.c:rndr_header_anchor`.
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
	const size_t size = strlen(a);
	size_t i = 0;
	int stripped = 0, inserted = 0;
	assert(a);
	{ /* If it's null-terminated, overwrite null. */
		char *zero = CharArrayPeek(&buffer);
		if(zero && *zero == '\0') CharArrayPop(&buffer);
	}
	/* Upper bound on the size; we don't need to worry about new. */
	if(!CharArrayBuffer(&buffer, size + 1)) return 0;
	for( ; i < size; ++i) {
		if(a[i] == '<') {
			/* Skip html tags. */
			while(i < size && a[i] != '>') i++;
		} else if(a[i] == '&') {
			/* Skip html entities. */
			while (i < size && a[i] != ';') i++;
		} else if(!isascii(a[i]) || strchr(strip, a[i])) {
			/* Replace non-ascii or invalid characters with dashes. */
			if(inserted && !stripped) *CharArrayNew(&buffer) = '-';
			/* And do it only once. */
			stripped = 1;
		} else {
			*CharArrayNew(&buffer) = tolower(a[i]);
			stripped = 0;
			inserted++;
		}
	}
	/* Replace the last dash if there was anything added. */
	if(stripped && inserted) CharArrayPop(&buffer);
	/* If anchor found empty, use djb2 hash for it. */
	if(!inserted && size) {
		const char fmt[] = "part-%lx";
		char *b;
		int len;
		unsigned long hash = 5381;
		/* h * 33 + c */
		for(i = 0; i < size; ++i) hash = ((hash << 5) + hash) + a[i];
		if((len = snprintf(0, 0, fmt, hash)) <= 0) return errno = EILSEQ, 0;
		/* `size_t` is allowed to be very small. */
		assert((size_t)len + 1 > (size_t)len);
		if(!(b = CharArrayBuffer(&buffer, (size_t)len + 1))) return 0;
		sprintf(b, fmt, hash);
	} else {
		*CharArrayNew(&buffer) = '\0';
	}
	return 1;
}

/*const char *BufferBuildAnchor(const enum Division d,
	const struct Token *(*const sprint)(const struct TokenArray *const,
	const struct Token *), const struct TokenArray *const ta,
							 const struct Token *t, const int is_href) {
	return 0;
}*/

/** Clear the result. */
/*void PathsResultClear(void) { CharArrayClear(&paths.result); }*/

/*int PathsAnchorClear(void) {
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
}*/

/** Safe fragment name for whatever platform, (_aka_ GitHub.) This is a
 surjection, so don't rely on non-letter symbols to make the name unique.
 Doesn't include the `#`.
 @return A temporary name fragment made up from `fn`:`fn_len`, invalid on
 calling any path function. It does include the `#`
 @throws[malloc] */
/*const char *PathsSafeAnchor(const size_t fn_len, const char *const fn) {
	CharArrayBuffer(&paths.working.buffer, fn_len + 1);
	if(!CdocGetGithub()) return fn;
	fprintf(stderr, "fixme: invert %.*s.\n", (int)fn_len, fn);
	return 0;
}

const char *PathsSafeFragment(const size_t fn_len, const char *const fn) {
	return 0;
}*/
