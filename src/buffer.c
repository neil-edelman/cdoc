/** @license 2017 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 This provides a space to store temporary strings.

 @std C89 */

#include <stddef.h> /* size_t */
#include <assert.h> /* assert */
#include "division.h"
#include "cdoc.h"
#include "buffer.h"

#define ARRAY_NAME char
#define ARRAY_TYPE char
#include "array.h"

static struct char_array buffers[2], *buffer = buffers;

/** Destructor for the buffers. */
void buffer_(void) {
	char_array_(buffers + 0);
	char_array_(buffers + 1);
}

/** Get the buffer. Always return a string. */
const char *buffer_get(void) {
	if(!buffer->size) return "";
	return buffer->data;
}

/** Sets the length to zero. */
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
	assert(length + !is_null_term >= length);
	if(!(s = char_array_buffer(buffer, length + !is_null_term))) return 0;
	*char_array_peek(buffer) = '\0';
	return s - is_null_term;
}

/** Switch the buffers so one can compare the two. */
void buffer_swap(void) {
	buffer = buffers + !(int)(buffer - buffers);
}
