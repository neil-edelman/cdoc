/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 Handles reading entire text files and keeping them in memory. */

#include "url.h"    /* url_dirsep */
#include "text.h"
#include <stdio.h>  /* FILE fopen fclose fread */
#include <string.h> /* memcpy strrchr strlen */
#include <stdlib.h> /* malloc free */
#include <assert.h> /* assert */
#include <errno.h>  /* errno EILSEQ */

#define ARRAY_NAME char
#define ARRAY_TYPE char
#include "array.h"

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
