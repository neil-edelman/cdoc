#include "division.h"
#include "style.h"
#include "hash.h"
#include "division.h"
#include "cdoc.h" /* cdoc_get_format */
#include "buffer.h"
#include "scanner.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* exit */

/* This is very `GitHub`-2019 specific: all anchor names are pre-concatenated
 with this string _except_ those that already have it, but the fragment
 links are unchanged. To make them line up and also be a valid Markdown for
 other sites, we must always start the anchor with this string. (Seems not to
 affect the latest browsers; possibly some JavaScript trickery going on.) We
 also must use a very restricted version of Unicode, but those restrictions are
 undocumented, thus the hash. */
static const char *const md_fragment_extra = "user-content-";

/** @return Used in `see`. */
const char *division_get_md_fragment_extra(void) { return md_fragment_extra; }

/** @param[label] In HTML. */
static void print_anchor_for(const enum division d, const char *const label) {
	const enum format f = style_format();
	const char *const divkey = division[d].keyword;
	assert(label);
	style_push(ST_H3), style_flush();
	printf("<a ");
	if(f == OUT_HTML) {
		printf("id = \"%s:%s\" name = \"%s:%s\"",
			divkey, label, divkey, label);
	} else {
		const unsigned hash = hash_str(label);
		printf("id = \"%s%s-%x\" name = \"%s%s-%x\"", md_fragment_extra,
			divkey, hash, md_fragment_extra, divkey, hash);
	}
	fputc('>', stdout);
	style_push(ST_TO_HTML); /* The format is HTML because it's in an HTML tag. */
	style_encode(label);
	style_pop();
	fputs("</a>", stdout);
	style_pop(); /* h2 */
}

static void print_custom_heading_anchor_for(const char *const divkey,
	const char *const desc) {
	assert(divkey && desc);
	style_push(ST_H2);
	style_flush();
	printf("<a ");
	(style_format() == OUT_HTML)
		? printf("id = \"%s:\" name = \"%s:\"", divkey, divkey)
		: printf("id = \"%s%s\" name = \"%s%s\"", md_fragment_extra, divkey,
		md_fragment_extra, divkey);
	printf(">%s</a>", desc);
	style_pop(); /* h2 */
}

static void print_heading_anchor_for(enum division d) {
	print_custom_heading_anchor_for(division[d].keyword, division[d].desc);
}

static unsigned log2u(unsigned i) {
	unsigned log = 0;
	while(i) log++, i >>= 1;
	return log;
}

/** For computing the number of digits. */
static unsigned compute_digits_x(unsigned x) {
	unsigned digits = (log2u(x) + 3) >> 2;
	/*fprintf(stderr, "digits log_16(%x) -> %u\n", x, digits);*/
	return digits ? digits : 1;
}

/** Scan the `str`, used below. */
static void scan_doc_string(const char *const str) {
	struct scanner *scan;
	assert(str);
	/* Generally this will be a bug in the programme, not in the input. */
	if(!(scan = scanner("string", str, &notify_brief, START_DOC)))
		{ fprintf(stderr, "In scan_doc_string. "); perror(str); assert(0);
		exit(EXIT_FAILURE); return; }
	style_flush(), print_brief();
	scanner_(&scan);
}

/* @param[label] Un-escaped label. */
static void print_fragment_for(const enum division d, const char *const label) {
	/* `effective_format` is NOT the thing we need; we need to raw format for
	 the link. */
	const enum format f = cdoc_get_format();
	const unsigned hash = hash_str(label);
	/* GCC is too smart but not smart enough. */
	const char *const fmt_html = "[%s](#%s:%s)",
		*const fmt_md = "[%s](#%s%s-%x)",
		*const divkey = division[d].keyword;
	const size_t label_len = strlen(label), divkey_len = strlen(divkey),
		md_fragment_extra_len = strlen(md_fragment_extra),
		fmt_len = (f == OUT_HTML) ? strlen("[](#:)") + 2 * label_len
		+ divkey_len : strlen("[](#-)") + label_len + md_fragment_extra_len
		+ divkey_len + compute_digits_x(hash);
	size_t len;
	char *b;
	assert(label);
	/* (Potentially) calling this with `label` as the other buffer. */
	buffer_swap();
	buffer_clear();
	if(!(b = buffer_prepare(fmt_len)))
		{ fprintf(stderr, "In print_fragment_for. "); perror(label);
		assert(0); exit(EXIT_FAILURE); /*return;*/ } /* fixme: This is icky. */
	len = (size_t)(f == OUT_HTML ? sprintf(b, fmt_html, label, divkey, label)
		: sprintf(b, fmt_md, label, md_fragment_extra, divkey, hash));
	assert(len == fmt_len);
	scan_doc_string(b);
	buffer_swap();
}

static void print_custom_heading_fragment_for(const char *const div_key,
	const char *const desc) {
	const enum format f = style_format();
	/* "*.0s" does not work on all libraries? */
	const char *const fmt_html = "[%s](#%s:)", *const fmt_md = "[%s](#%s%s)";
	const size_t fmt_len = (f == OUT_HTML)
		? strlen("[](#:)") + strlen(desc) + strlen(div_key)
		: strlen("[](#)") + strlen(desc) + strlen(md_fragment_extra)
		+ strlen(div_key);
	size_t len;
	char *b;
	assert(div_key && desc);
	buffer_clear();
	if(!(b = buffer_prepare(fmt_len)))
		{ fprintf(stderr, "In print_custom_heading_fragment_for. ");
		perror(div_key); assert(0); exit(EXIT_FAILURE); return; }
	len = (size_t)(f == OUT_HTML ? sprintf(b, fmt_html, desc, div_key) :
		sprintf(b, fmt_md, desc, md_fragment_extra, div_key));
	assert(len == fmt_len);
	scan_doc_string(b);
}

static void print_heading_fragment_for(const enum division d) {
	print_custom_heading_fragment_for(division[d].keyword, division[d].desc);
}
