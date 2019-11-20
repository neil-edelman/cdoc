/** @license 2019 Neil Edelman, distributed under the terms of the MIT License;
 see readme.txt, or \url{ https://opensource.org/licenses/MIT }.

 Handles reading an entire file.

 @file File
 @author Neil
 @std C89 */

#include <stdio.h>  /* FILE fopen fclose fread */
#include <string.h> /* memcpy strrchr strlen */
#include <stdlib.h> /* malloc free */
#include <assert.h> /* assert */
#include <errno.h>  /* errno EILSEQ */
#include "Buffer.h"

const char dirsep = '/';

/* Define `CharArray`, a vector of characters. */
#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "Array.h"

struct Buffer { struct CharArray buffer; const char *filename, *basename; };

/** Zeros `file`. */
static void zero_buffer(struct Buffer *const b) {
	assert(b);
	CharArray(&b->buffer);
	b->filename = 0;
	b->basename = 0;
}

/** Unloads `file` from memory. */
void Buffer_(struct Buffer **const pb) {
	struct Buffer *b;
	if(!pb || !(b = *pb)) return;
	CharArray_(&b->buffer);
	free(b);
	*pb = 0;
}

/** Ensures that the file has no zeros but not if the file has a '\n' at the
 end.
 @return Reads `fn` to memory. */
struct Buffer *Buffer(const char *const fn) {
	FILE *fp = 0;
	struct Buffer *b = 0;
	const size_t granularity = 1024;
	size_t nread, zero_len, file_size, fn_size;
	char *read_here, *terminating, *buffer, *base;
	/* Dynamically allocate `file`. */
	if(!fn || !(fp = fopen(fn, "r"))) goto catch;
	fn_size = strlen(fn) + 1;
	if(!(b = malloc(sizeof *b + fn_size))) goto catch;
	zero_buffer(b);
	b->filename = (const char *)(b + 1);
	memcpy(&b->filename, fn, fn_size);
	b->basename = (base = strrchr(b->filename, dirsep)) ? base + 1 :b->filename;
	/* Read all contents at once. */
	do {
		if(!(read_here = CharArrayBuffer(&b->buffer, granularity))
			|| (nread = fread(read_here, 1, granularity, fp), ferror(fp))
			|| !CharArrayExpand(&b->buffer, nread)) goto catch;
	} while(nread == granularity);
	fclose(fp), fp = 0;
	/* Embed '\0' on the end for simple lexing. */
	if(!(terminating = CharArrayNew(&b->buffer))) goto catch;
	*terminating = '\0';
	/* The file can have no embedded '\0'. */
	buffer = CharArrayGet(&b->buffer);
	zero_len = (size_t)(strchr(buffer, '\0') - buffer);
	file_size = CharArraySize(&b->buffer);
	assert(file_size > 0);
	if(zero_len != file_size - 1) { errno = EILSEQ; goto catch; }
	return b;
catch:
	if(fp) fclose(fp);
	Buffer_(&b);
	return 0;
}

/** @return The file name of `file`. */
const char *BufferName(const struct Buffer *const b) {
	return b ? b->filename : 0;
}

/** @return The base file name of `file`. */
const char *BufferBaseName(const struct Buffer *const b) {
	return b ? b->basename : 0;
}

/** @return The length of the contents of `file`. */
size_t BufferSize(const struct Buffer *const b) {
	return b ? CharArraySize(&b->buffer) : 0;
}

/** @return The contents of `file`. */
const char *BufferGet(const struct Buffer *const b) {
	return b ? CharArrayGet(&b->buffer) : 0;
}
