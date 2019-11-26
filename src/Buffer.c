/** @license 2017 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 This provides a space to store temporary strings.

 @std C89 */

#include <stdio.h>  /* snprintf sprintf */
#include <string.h> /* strlen */
#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include <ctype.h>  /* tolower */
#include "Division.h"
#include "Cdoc.h"
#include "Buffer.h"

#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "Array.h"

struct Buffer { struct CharArray buffer; };

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
	struct CharArray buffer; /* fixme */
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
		} else if(!isascii(a[i])
			|| strchr(" -&+$,/:;=?@\"#{}|^~[]`\\*()%.!'", a[i])) {
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
		const char *const fmt = "part-%lx";
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

/** Destructor for the buffer. */
void Buffer_(struct Buffer **const pb) {
	struct Buffer *b;
	if(!pb || !(b = *pb)) return;
	CharArray_(&b->buffer);
}

/** Get the buffer. */
const char *BufferGet(const struct Buffer *const b) {
	if(!b || !CharArraySize(&b->buffer)) return "";
	return CharArrayGet(&b->buffer);
}

/** Sets the length to zero. */
void BufferClear(struct Buffer *const b) {
	if(!b) return;
	CharArrayClear(&b->buffer);
}

/** Expands the buffer to include `length`, (not including the terminating
 null.) This must be followed by a write of `length`.
 @param[length] The length the string, not including the terminating null.
 @return The buffer position that is appropriate to place a `char` array of
 `length`. `length + 1` is a null terminator which one could overwrite or
 not. */
char *BufferPrepare(struct Buffer *const b, const size_t length) {
	const char *last;
	char *s;
	int is_null_term;
	if(!b) return 0;
	last = CharArrayPeek(&b->buffer);
	is_null_term = !!last && *last == '\0';
	assert(length + !is_null_term >= length);
	if(!(s = CharArrayBuffer(&b->buffer, length + !is_null_term))) return 0;
	CharArrayExpand(&b->buffer, length + !is_null_term);
	*CharArrayPeek(&b->buffer) = '\0';
	fprintf(stderr, "Buffer size: %lu.\n", CharArraySize(&b->buffer));
	return s - is_null_term;
}
