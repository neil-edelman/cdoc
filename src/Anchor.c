/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 This is a hack to output to GitHub, which transforms the anchor names but not
 fragment links, leading to broken anchor links over their whole site.

 @std C89/90 */

#include <stdlib.h> /* EXIT malloc free */
#include <stdio.h>  /* *printf */
#include <string.h> /* strlen strchr */
#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include <ctype.h>  /* tolower */
#include "Cdoc.h"
#include "Anchor.h"

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
	const size_t size = strlen(a);
	size_t i = 0;
	int stripped = 0, inserted = 0;
	assert(a);
	if(!CharArrayBuffer(&buffer, strlen(a))) return 0;
	for( ; i < size; ++i) {
		if(a[i] == '<') {
			/* Skip html tags? */
			while(i < size && a[i] != '>') i++;
		} else if(a[i] == '&') {
			/* Skip html entities? */
			while (i < size && a[i] != ';') i++;
		} else if(!isascii(a[i])
				|| strchr(" -&+$,/:;=?@\"#{}|^~[]`\\*()%.!'", a[i])) {
			/* Replace non-ascii or invalid characters with dashes.
			 fixme: slow. */
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

/** Safe anchor. */
const char *AnchorTransform(const char *anchor, const int is_href) {
	/* If it's not going to `GitHub`, this is the identity. */
	if(!CdocGetGithub()) return anchor;
	CharArrayClear(&buffer);
	if(is_href) {
		const char *const user = "user-content-";
		const size_t user_len = strlen(user);
		if(!CharArrayBuffer(&buffer, user_len)) return 0;
		memcpy(CharArrayGet(&buffer), user, user_len);
	}
	if(!github_anchor_working_cat(anchor)) return 0;
	return CharArrayGet(&buffer);
}

/** Destructor for anchor. */
void Anchor_(void) { CharArray_(&buffer); }
