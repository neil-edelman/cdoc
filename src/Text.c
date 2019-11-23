/** @license 2019 Neil Edelman, distributed under the terms of the MIT License;
 see readme.txt, or \url{ https://opensource.org/licenses/MIT }.

 Handles reading entire files and keeping them in memory.

 @file Text
 @author Neil
 @std C89 */

#include <stdio.h>  /* FILE fopen fclose fread */
#include <string.h> /* memcpy strrchr strlen */
#include <stdlib.h> /* malloc free */
#include <assert.h> /* assert */
#include <errno.h>  /* errno EILSEQ */
#include "Cdoc.h"
#include "Text.h"

const char dirsep = '/';

/* Define `CharArray`, a vector of characters. */
#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "Array.h"

struct Text { struct CharArray buffer; char *filename, *basename; };

/** Zeros `file`. */
static void zero_buffer(struct Text *const b) {
	assert(b);
	CharArray(&b->buffer);
	b->filename = 0;
	b->basename = 0;
}

/** Unloads `file` from memory. */
static void Text_(struct Text **const pb) {
	struct Text *b;
	if(!pb || !(b = *pb)) return;
	if(CdocGetDebug()) fprintf(stderr, "Freeing data assciated with %s.\n",
		b->basename);
	CharArray_(&b->buffer);
	free(b);
	*pb = 0;
}

/** Opens the file as text and ensures that the file contents has no zeros, but
 doesn't do any checks otherwise.
 @return Reads `fn` to memory as a `Text` or null is error.
 @throws[malloc, fread]
 @throws[EILSEQ] If the file has embedded zeros. */
static struct Text *Text(const char *const fn) {
	FILE *fp = 0;
	struct Text *t = 0;
	const size_t granularity = 1024;
	size_t nread, zero_len, file_size, fn_size;
	char *read_here, *terminating, *buffer, *base;
	if(!fn || !(fp = fopen(fn, "r"))) goto catch;
	fn_size = strlen(fn) + 1;
	if(!(t = malloc(sizeof *t + fn_size))) goto catch;
	zero_buffer(t);
	t->filename = (char *)(t + 1);
	memcpy(t->filename, fn, fn_size);
	t->basename = (base = strrchr(t->filename, dirsep)) ? base + 1 :t->filename;
	/* Read all contents at once and close the file; now in memory. */
	do {
		if(!(read_here = CharArrayBuffer(&t->buffer, granularity))
			|| (nread = fread(read_here, 1, granularity, fp), ferror(fp))
			|| !CharArrayExpand(&t->buffer, nread)) goto catch;
	} while(nread == granularity);
	fclose(fp), fp = 0;
	/* Embed '\0' on the end for simple lexing. */
	if(!(terminating = CharArrayNew(&t->buffer))) goto catch;
	*terminating = '\0';
	/* The file can have no embedded '\0'. */
	buffer = CharArrayGet(&t->buffer);
	zero_len = (size_t)(strchr(buffer, '\0') - buffer);
	file_size = CharArraySize(&t->buffer);
	assert(file_size > 0);
	if(zero_len != file_size - 1) { errno = EILSEQ; goto catch; }
	return t;
catch:
	if(fp) fclose(fp);
	Text_(&t);
	return 0;
}

/** @return The file name of `file`. */
const char *TextName(const struct Text *const b) {
	return b ? b->filename : 0;
}

/** @return The base file name of `file`. */
const char *TextBaseName(const struct Text *const b) {
	return b ? b->basename : 0;
}

/** @return The length of the contents of `file`. */
size_t TextSize(const struct Text *const b) {
	return b ? CharArraySize(&b->buffer) : 0;
}

/** @return The contents of `file`. */
const char *TextGet(const struct Text *const b) {
	return b ? CharArrayGet(&b->buffer) : 0;
}

#define ARRAY_NAME Text
#define ARRAY_TYPE struct Text *
#include "Array.h"

static struct TextArray files;

/** Loads new `Text` from `fn` into memory. */
struct Text *TextOpen(const char *const fn) {
	struct Text **ptext = TextArrayNew(&files);
	if(!ptext) return 0;
	if(!(*ptext = Text(fn))) { TextArrayPop(&files); return 0; }
	return *ptext;
}

/** Unloads all texts. */
void TextCloseAll(void) {
	struct Text **ptext;
	while((ptext = TextArrayPop(&files))) Text_(ptext);
	TextArray_(&files);
}
