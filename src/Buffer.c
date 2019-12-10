/** @license 2017 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 This provides a space to store temporary strings.

 @std C89 */

#include <stddef.h> /* size_t */
#include <assert.h> /* assert */
#include "Division.h"
#include "Cdoc.h"
#include "Buffer.h"

#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "Array.h"

static struct CharArray buffers[2], *buffer = buffers;

/** Destructor for the buffers. */
void Buffer_(void) {
	CharArray_(buffers + 0);
	CharArray_(buffers + 1);
}

/** Get the buffer. */
const char *BufferGet(void) {
	if(!CharArraySize(buffer)) return "";
	return CharArrayGet(buffer);
}

/** Sets the length to zero. */
void BufferClear(void) {
	CharArrayClear(buffer);
}

/** Buffers `size`.
 @throws[malloc] */
int BufferBuffer(const size_t size) {
	assert(size + 1 > size);
	return !!CharArrayBuffer(buffer, size + 1);
}

/** Expands the buffer to include `length`, (not including the terminating
 null.) This must be followed by a write of `length`.
 @param[length] The length the string, not including the terminating null.
 @return The buffer position that is appropriate to place a `char` array of
 `length`. `length + 1` is a null terminator which one could overwrite or
 not. */
char *BufferPrepare(const size_t length) {
	const char *const last = CharArrayPeek(buffer);
	int is_null_term = !!last && *last == '\0';
	char *s;
	assert(length + !is_null_term >= length);
	if(!(s = CharArrayBuffer(buffer, length + !is_null_term))) return 0;
	CharArrayExpand(buffer, length + !is_null_term);
	*CharArrayPeek(buffer) = '\0';
	return s - is_null_term;
}

/** Switch the buffers so one can compare the two. */
void BufferSwap(void) {
	buffer = buffers + !(int)(buffer - buffers);
}
