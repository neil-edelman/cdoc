
#include <stdio.h>

/** Perform a 32 bit
 [Fowler/Noll/Vo FNV-1a hash](http://www.isthe.com/chongo/tech/comp/fnv/) on a
 string. This assumes that size of `int` is at least 32 bits; if this is not
 true, we may get a different answer, (`stdint.h` is `C99`.) */
static unsigned fnv_32a_str(const char *str) {
	const char *s = str;
	/* 32 bit FNV-1 and FNV-1a non-zero initial basis, FNV1_32A_INIT */
	unsigned hval = 0x811c9dc5;
	assert(str);
	/* FNV magic prime `FNV_32_PRIME 0x01000193`. */
	while(*s) {
		hval ^= (unsigned)*s++;
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
	}
	if(cdoc_get_debug() & DBG_HASH)
		fprintf(stderr, "fnv32: %s -> %u\n", str, hval);
	return hval & 0xffffffff;
}

/* This is very `GitHub`-2019 specific: all anchor names are pre-concatenated
 with this string _except_ those that already have it, but the fragment
 links are unchanged. To make them line up and also be a valid Markdown for
 other sites, we must always start the anchor with this string. (Seems not to
 affect the latest browsers; possibly some JavaScript trickery going on.) We
 also must use a very restricted version of Unicode, but those restrictions are
 undocumented, thus the hash. */
static const char *const md_fragment_extra = "user-content-";

/* `SYMBOL` is declared in `scanner.h`. */
#define X(a, b, c, d, e, f) f
static const char *symbol_attribute_titles[] = { SYMBOL };
#undef X




/** Prints the documentation part of `brief` and erases it. */
static void print_brief(void) {
	print_tokens(&brief);
	token_array_clear(&brief);
}



typedef int (*divisionPredicate)(const enum division);

/** @implements divisionPredicate */
static int is_div_preamble(const enum division d) {
	return d == DIV_PREAMBLE;
}

/** @implements divisionPredicate */
static int is_not_div_preamble(const enum division d) {
	return d != DIV_PREAMBLE;
}

/** @return Searches though a valid `tok` for a match to `token`. */
static int any_token(struct token_array_cursor *tok,
	const struct token *const token) {
	for( ; token_array_exists(tok); token_array_next(tok))
		if(!token_compare(token, token_array_look(tok))) return 1;
	return 0;
}

/** @return If there exist an `attribute_symbol` with content within
 `segment`. */
static int segment_attribute_exists(/*const*/ struct segment *const segment,
	const enum symbol attribute_symbol) {
	struct attribute_array_cursor att;
	assert(segment);
	for(att = attribute_array_begin(&segment->attributes);
		attribute_array_exists(&att); attribute_array_next(&att)) {
		const struct attribute *const a = attribute_array_look(&att);
		if(a->token.symbol == attribute_symbol
		   && a->contents.size) return 1;
	}
	return 0;
}

/** @return Is `division` in the report? */
static int division_exists(const enum division divkey) {
	struct segment_array_cursor seg;
	for(seg = segment_array_begin(&report); segment_array_exists(&seg);
		segment_array_next(&seg))
		if(segment_array_look(&seg)->division == divkey) return 1;
	return 0;
}

/** `act` on all `division`. */
static void division_act(const enum division divkey,
	void (*act)(const struct segment *const segment)) {
	struct segment_array_cursor seg;
	assert(act);
	for(seg = segment_array_begin(&report); segment_array_exists(&seg);
		segment_array_next(&seg)) {
		const struct segment *const s = segment_array_look(&seg);
		if(s->division == divkey) act(s);
	}
}

/** @return Is `attribute_symbol` in the report? (needed for `@licence`.) */
static int attribute_exists(const enum symbol attribute_symbol) {
	struct segment_array_cursor seg;
	for(seg = segment_array_begin(&report); segment_array_exists(&seg);
		segment_array_next(&seg))
		if(segment_attribute_exists(segment_array_look(&seg), attribute_symbol))
		return 1;
	return 0;
}

/** Searches if attribute `symbol` exists within `segment` whose header
 contains `match`.
 @order O(`attributes`) */
static int segment_attribute_match_exists(const struct segment *const segment,
	const enum symbol symbol, const struct token *const match) {
	struct attribute_array_cursor att;
	assert(segment);
	for(att = attribute_array_begin(&segment->attributes);
		attribute_array_exists(&att); attribute_array_next(&att)) {
		const struct attribute *const a = attribute_array_look(&att);
		if(a->token.symbol != symbol) continue;
		if(any_token(&a->header, match)) return 1;
	}
	return 0;
}

static void print_best_guess_at_modifiers(const struct segment *const segment) {
	const struct token *code = segment->code.data,
		*stop = code + *segment->code_params.data;
	assert(segment && segment->division == DIV_FUNCTION
		&& segment->code_params.size >= 1
		&& segment->code.size > *segment->code_params.data);
	while(code < stop) {
		if(code->symbol == LPAREN) { /* I guess! */
			style_separate();
			style_push(ST_EM), style_flush();
			printf("function");
			style_pop();
			return;
		}
		/* fixme: No, the thing will not update, need `code` to be an iterator. */
		{
			const struct token_array_cursor tok
				= token_array_at(&segment->code, code - segment->code.data);
			print_token(&tok);
		}
		/*code = print_token(&segment->code, code);*/
	}
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

/* @param[label] Un-escaped label. */
static void print_fragment_for(const enum division d, const char *const label) {
	/* `effective_format` is NOT the thing we need; we need to raw format for
	 the link. */
	const enum format f = cdoc_get_format();
	const unsigned hash = fnv_32a_str(label);
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
		assert(0); exit(EXIT_FAILURE); return; }
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
		const unsigned hash = fnv_32a_str(label);
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

/** Toc subcategories. */
static void print_toc_extra(const enum division d) {
	struct segment_array_cursor seg;
	size_t *idxs;
	struct token *params;
	const char *b;
	printf(": ");
	style_push(ST_CSV), style_push(ST_NO_STYLE);
	for(seg = segment_array_begin(&report); segment_array_exists(&seg);
		segment_array_next(&seg)) {
		const struct segment *const s = segment_array_look(&seg);
		if(s->division != d) continue;
		if(!s->code_params.size) { fprintf(stderr,
			"%s: segment has no title.\n", division[s->division].symbol);
			continue; }
		idxs = s->code_params.data;
		params = s->code.data;
		assert(idxs[0] < s->code.size);
		style_push(ST_TO_RAW);
		b = print_token_s(&s->code, params + idxs[0]);
		style_pop();
		print_fragment_for(d, b);
		style_pop_push();
	}
	style_pop(), style_pop();
}

/** Functions as a bit-vector. */
enum AttShow { SHOW_NONE, SHOW_WHERE, SHOW_TEXT, SHOW_ALL };

static void segment_att_print_all(const struct segment *const segment,
	const enum symbol symbol, const struct token *const match,
	const enum AttShow show) {
	struct attribute_array_cursor att;
	assert(segment);
	if(!show) return;
	if(cdoc_get_debug() & DBG_ERASE)
		fprintf(stderr, "segment_att_print_all segment %s and symbol %s.\n", division[segment->division].symbol, symbols[symbol]);
	for(att = attribute_array_begin(&segment->attributes);
		attribute_array_exists(&att); attribute_array_next(&att)) {
		const struct attribute *const a = attribute_array_look(&att);
		if(a->token.symbol != symbol
		   || (match && !any_token(&a->header, match))) continue;
		style_flush();
		if(show & SHOW_WHERE) {
			/*struct index_array_cursor idx;
			for(idx = index_array_begin(&segment->code_params);
				index_array_exists(&idx); index_array_next(&idx)) {
				const struct token_array_cursor tok
					= token_array_at(&segment->code,
					*index_array_look(&idx));
				print_fragment_for(segment->division, 0/*print_token_s(&tok));*/
			/* fixme */

			/*if((pindex = index_array_next(&segment->code_params, 0))
			   && *pindex < segment->code.size) {
				const struct token *token = segment->code.data + *pindex;
				print_fragment_for(segment->division,
					print_token_s(&segment->code, token));*/
			if(1) {
				assert(0);
			} else {
				/* Not going to happen -- cull takes care of it. */
				printf("%s", division[segment->division].keyword);
			}
		}
		if(show == SHOW_ALL) fputs(": ", stdout);
		if(show & SHOW_TEXT) print_tokens(&a->contents);
		style_pop_push();
		/* Only do one if `SHOW_TEXT` is not set; in practice, this affects
		 license, only showing one _per_ function. */
		if(!(show & SHOW_TEXT)) break;
	}
}

/** For each `division` segment, print all attributes that match `symbol`.
 @param[div_pred] If specified, only prints when it returns true.
 @param[symbol, show] Passed to <fn:segment_att_print_all>. */
static void div_att_print(const divisionPredicate div_pred,
	const enum symbol symbol, const enum AttShow show) {
	struct segment_array_cursor seg;
	if(!show) return;
	for(seg = segment_array_begin(&report); segment_array_exists(&seg);
		segment_array_next(&seg)) {
		const struct segment *const s = segment_array_look(&seg);
		if(!div_pred || div_pred(s->division))
			segment_att_print_all(s, symbol, 0, show);
	}
}

static void dl_segment_att(const struct segment *const segment,
	const enum symbol attribute, const struct token *match,
	const enum style_punctuate p) {
	assert(segment && attribute);
	if((match && !segment_attribute_match_exists(segment, attribute, match))
	   || (!match && !segment_attribute_exists(segment, attribute))) return;
	style_push(ST_DT), style_push(ST_PLAIN), style_flush();
	printf("%s:", symbol_attribute_titles[attribute]);
/*	if(match) style_separate(), style_push(ST_EM),
		print_token(&segment->code, match), style_pop(); fixme */
	style_pop(), style_pop();
	style_push(ST_DD), style_push(p), style_push(ST_PLAIN);
	segment_att_print_all(segment, attribute, match, SHOW_TEXT);
	if(cdoc_get_debug() & DBG_ERASE)
		fprintf(stderr, "dl_segment_att for %s.\n", symbols[attribute]);
	style_pop(), style_pop(), style_pop();
}

/** This is used in preamble for attributes inside a `dl`.
 @param[is_recursive]  */
static void dl_preamble_att(const enum symbol attribute,
	const enum AttShow show, const enum style_punctuate p) {
	assert(!style_is_empty());
	if(cdoc_get_debug() & DBG_ERASE)
		fprintf(stderr, "dl_preamble_att for %s.\n", symbols[attribute]);
	if(!attribute_exists(attribute)) return;
	style_push(ST_DT), style_flush();
	printf("%s:", symbol_attribute_titles[attribute]);
	style_pop();
	style_push(ST_DD), style_push(p), style_push(ST_PLAIN);
	div_att_print(&is_div_preamble, attribute, SHOW_TEXT);
	style_pop(), style_push(ST_PAREN), style_push(ST_CSV), style_push(ST_PLAIN);
	div_att_print(&is_not_div_preamble, attribute, show);
	style_pop(), style_pop(), style_pop(), style_pop(), style_pop();
}

static void dl_segment_specific_att(const struct attribute *const attribute) {
	assert(attribute);
	style_push(ST_DT), style_push(ST_PLAIN), style_flush();
	printf("%s:", symbol_attribute_titles[attribute->token.symbol]);
	if(attribute->header.size) {
		const struct token *token = 0;
		style_separate();
		style_push(ST_CSV);
/*		while((token = token_array_next(&attribute->header, token)))
			print_token(&attribute->header, token), style_separate(); fixme*/
		style_pop();
	}
	style_pop(), style_pop();
	style_push(ST_DD), style_push(ST_PLAIN);
	print_tokens(&attribute->contents);
	style_pop(), style_pop();
}



/** Prints all a `segment`.
 @implements division_act */
static void segment_print_all(const struct segment *const segment) {
	const struct token *param;
	const char *b;
	assert(segment && segment->division != DIV_PREAMBLE);
	style_push(ST_DIV);

	/* The title is generally the first param. Only single-words. */
	if((param = param_no(segment, 0))) {
		style_push(ST_TO_RAW); /* Anchors are always raw. */
		b = print_token_s(&segment->code, param);
		style_pop();
		print_anchor_for(segment->division, b);
		style_push(ST_P), style_push(ST_TO_HTML);
		style_flush();
		printf("<code>");
		highlight_tokens(&segment->code, &segment->code_params);
		printf("</code>");
		style_pop_strong();
	} else {
		style_push(ST_H3), style_flush();
		printf("Unknown");
		style_pop_strong();
	}

	/* Now text. */
	style_push(ST_P);
	print_tokens(&segment->doc);
	style_pop_strong();

	/* Attrubutes. */
	style_push(ST_DL);
	if(segment->division == DIV_FUNCTION) {
		struct attribute_array_cursor att;
		size_t no;
		for(no = 1; (param = param_no(segment, no)); no++)
			dl_segment_att(segment, ATT_PARAM, param, ST_PLAIN);
		dl_segment_att(segment, ATT_RETURN, 0, ST_PLAIN);
		for(att = attribute_array_begin(&segment->attributes);
			attribute_array_exists(&att); attribute_array_next(&att)) {
			const struct attribute *const a = attribute_array_look(&att);
			if(a->token.symbol != ATT_THROWS) continue;
			dl_segment_specific_att(a);
		}
		dl_segment_att(segment, ATT_ORDER, 0, ST_PLAIN);
	} else if(segment->division == DIV_TAG) {
		struct attribute_array_cursor att;
		for(att = attribute_array_begin(&segment->attributes);
			attribute_array_exists(&att); attribute_array_next(&att)) {
			const struct attribute *const a = attribute_array_look(&att);
			if(a->token.symbol != ATT_PARAM) continue;
			dl_segment_specific_att(a);
		}
	}
	dl_segment_att(segment, ATT_IMPLEMENTS, 0, ST_CSV);
	dl_segment_att(segment, ATT_AUTHOR, 0, ST_CSV);
	dl_segment_att(segment, ATT_STD, 0, ST_SSV);
	dl_segment_att(segment, ATT_DEPEND, 0, ST_SSV);
	dl_segment_att(segment, ATT_FIXME, 0, ST_PLAIN);
	dl_segment_att(segment, ATT_LICENSE, 0, ST_PLAIN);
	dl_segment_att(segment, ATT_CF, 0, ST_SSV);
	style_pop_strong(); /* dl */
	style_pop_strong(); /* div */
}



/** Outputs a report.
 @throws[EILSEQ] Sequence error.
 @return Success. */
int report_out(void) {
	const char *const summary = "summary",
		*const summary_desc = "Function Summary",
		*const license = "license",
		*const license_desc = symbol_attribute_titles[ATT_LICENSE];
	const int is_preamble = division_exists(DIV_PREAMBLE),
		is_function = division_exists(DIV_FUNCTION),
		is_tag = division_exists(DIV_TAG),
		is_typedef = division_exists(DIV_TYPEDEF),
		is_data = division_exists(DIV_DATA),
		is_license = attribute_exists(ATT_LICENSE),
		is_abstract = attribute_exists(ATT_ABSTRACT);
	struct segment_array_cursor seg = segment_array_begin(&report);
	const int is_html = style_format() == OUT_HTML;
	const char *const in_fn = cdoc_get_input(),
		*const base_fn = strrchr(in_fn, *url_dirsep),
		*const title = base_fn ? base_fn + 1 : in_fn;

	assert(in_fn && style_is_empty());

	/* Set `errno` here so that we don't have to test output each time. */
	errno = 0;
	if(is_html) {
		printf("<!doctype html public \"-//W3C//DTD HTML 4.01//EN\" "
			"\"http://www.w3.org/TR/html4/strict.dtd\">\n\n"
			"<html>\n\n"
			"<head>\n"
			"<meta http-equiv = \"Content-Type\""
			" content = \"text/html;charset=UTF-8\">\n"
			"<!-- Steal these colour values from JavaDocs. -->\n"
			"<style type = \"text/css\">\n"
			"\ta:link,  a:visited { color: #4a6782; }\n"
			"\ta:hover, a:focus   { color: #bb7a2a; }\n"
			"\ta:active           { color: #4A6782; }\n"
			"\ttr:nth-child(even) { background: #dee3e9; }\n"
			"\tdiv {\n"
			"\t\tmargin:  4px 0;\n"
			"\t\tpadding: 0 4px 4px 4px;\n");
		printf("\t}\n"
			"\ttable      { width: 100%%; }\n"
			"\ttd         { padding: 4px; }\n"
			"\th3, h1 {\n"
			"\t\tcolor: #2c4557;\n"
			"\t\tbackground-color: #dee3e9;\n"
			"\t\tpadding:          4px;\n"
			"\t}\n"
			"\th3 {\n"
			"\t\tmargin:           0 -4px;\n"
			"\t\tpadding:          4px;\n"
			"\t}\n"
			"</style>\n");
		style_push(ST_TITLE), style_push(ST_SSV), style_push(ST_PLAIN);
		style_flush(), style_encode(title), style_pop_strong();
		assert(style_is_empty());
		printf("</head>\n\n"
			"<body>\n\n");
	}

	/* Title. */
	style_push(ST_H1), style_push(ST_SSV), style_push(ST_PLAIN);
	style_flush(), style_encode(title), style_pop_strong();
	assert(style_is_empty());

	/* Abstract, (_ie_, tl;dr.) */
	if(is_abstract) {
		style_push(ST_P);
		div_att_print(&is_div_preamble, ATT_ABSTRACT, SHOW_TEXT);
		style_pop_strong();
	}

	/* Subtitle. */
	style_push(ST_H2), style_push(ST_SSV), style_push(ST_PLAIN);
	div_att_print(&is_div_preamble, ATT_SUBTITLE, SHOW_TEXT);
	style_pop_strong();
	assert(style_is_empty());

	/* TOC. */
	style_push(ST_UL), style_push(ST_LI);
	if(is_preamble) print_heading_fragment_for(DIV_PREAMBLE), style_pop_push();
	if(is_typedef) print_heading_fragment_for(DIV_TYPEDEF),
		print_toc_extra(DIV_TYPEDEF), style_pop_push();
	if(is_tag) print_heading_fragment_for(DIV_TAG),
		print_toc_extra(DIV_TAG), style_pop_push();
	if(is_data) print_heading_fragment_for(DIV_DATA),
		print_toc_extra(DIV_DATA), style_pop_push();
	if(is_function) print_custom_heading_fragment_for(summary, summary_desc),
		style_pop_push(), print_heading_fragment_for(DIV_FUNCTION),
		style_pop_push();
	if(is_license) print_custom_heading_fragment_for(license, license_desc),
		style_pop_push();
	style_pop_strong();
	assert(style_is_empty());

	/* Preamble contents; it shows up as the more-aptly named "description" but
	 I didn't want to type that much. */
	if(is_preamble) {
		style_push(ST_DIV), style_push(ST_NO_STYLE);
		print_heading_anchor_for(DIV_PREAMBLE);
		style_push(ST_P);
		for( ; segment_array_exists(&seg); segment_array_next(&seg))
		/*while((segment = segment_array_next(&report, segment)))*/ {
			/*const*/ struct segment *const segment = segment_array_look(&seg);
			if(segment->division != DIV_PREAMBLE) continue;
			print_tokens(&segment->doc);
			style_pop_push();
		}
		style_pop_strong(); /* P */
		style_push(ST_DL);
		/* `ATT_TITLE` is above. */
		for( ; segment_array_exists(&seg); segment_array_next(&seg))
		/*while((segment = segment_array_next(&report, segment)))*/ {
			/*const*/ struct segment *const segment = segment_array_look(&seg);
			struct attribute_array_cursor att;
			if(segment->division != DIV_PREAMBLE) continue;
			for(att = attribute_array_begin(&segment->attributes);
				attribute_array_exists(&att); attribute_array_next(&att)) {
				const struct attribute *const a = attribute_array_look(&att);
				if(a->token.symbol != ATT_PARAM) continue;
				dl_segment_specific_att(a);
			}
		}
		dl_preamble_att(ATT_AUTHOR, SHOW_ALL, ST_CSV);
		dl_preamble_att(ATT_STD, SHOW_ALL, ST_CSV);
		dl_preamble_att(ATT_DEPEND, SHOW_ALL, ST_CSV);
		dl_preamble_att(ATT_FIXME, SHOW_WHERE, ST_PLAIN);
		dl_preamble_att(ATT_CF, SHOW_ALL, ST_SSV);
		/* `ATT_RETURN`, `ATT_THROWS`, `ATT_IMPLEMENTS`, `ATT_ORDER`,
		 `ATT_ALLOW` have warnings. `ATT_LICENSE` is below. */
		style_pop_strong();
		style_pop_strong();
	}
	assert(style_is_empty());

	/* Print typedefs. */
	if(is_typedef) {
		print_heading_anchor_for(DIV_TYPEDEF);
		division_act(DIV_TYPEDEF, &segment_print_all);
	}
	/* Print tags. */
	if(is_tag) {
		print_heading_anchor_for(DIV_TAG);
		division_act(DIV_TAG, &segment_print_all);
	}
	/* Print general declarations. */
	if(is_data) {
		print_heading_anchor_for(DIV_DATA);
		division_act(DIV_DATA, &segment_print_all);
	}
	/* Print functions. */
	if(is_function) {
		/* Function table. */
		style_push(ST_DIV);
		print_custom_heading_anchor_for(summary, summary_desc);
		style_push(ST_TO_HTML);
		style_flush();
		printf("<table>\n\n"
			"<tr><th>Modifiers</th><th>Function Name</th>"
			"<th>Argument List</th></tr>\n\n");
		for( ; segment_array_exists(&seg); segment_array_next(&seg))
		/*while((segment = segment_array_next(&report, segment)))*/ {
			/*const*/ struct segment *const segment = segment_array_look(&seg);
			struct index_array_cursor codepar
				= index_array_begin(&segment->code_params);
			struct token_array_cursor code;
			size_t codeparam;
			struct token *params;
			/*size_t *idxs, idxn, idx, paramn;*/
			const char *buffer;
			if(segment->division != DIV_FUNCTION
				|| /*!(idxn = segment->code_params.size)*/
				!index_array_exists(&codepar)) continue;
			/*idxs = segment->code_params.data;
			params = segment->code.data;
			paramn = segment->code.size;
			assert(idxs[0] < paramn);*/
			codeparam = *index_array_look(&codepar);
			assert(codeparam < segment->code.size);
			printf("<tr><td align = right>");
			style_push(ST_PLAIN);
			print_best_guess_at_modifiers(segment);
			style_pop();
			printf("</td><td>");
			style_push(ST_TO_RAW); /* Always get raw; translate after. */
			code = token_array_at(&segment->code, codeparam);
			if(!print_token_s(&segment->code,
				segment->code.data + codeparam, &buffer))
				goto catch;
			style_pop();
			print_fragment_for(DIV_FUNCTION, buffer);
			printf("</td><td>");
			for(idx = 1; idx < idxn; idx++) {
				assert(idxs[idx] < segment->code.size);
				if(idx > 1) printf(", ");
				print_token(&segment->code, params + idxs[idx]);
			}
			printf("</td></tr>\n\n");
		}
		printf("</table>\n\n");
		style_pop(); /* to_html */
		style_pop(); /* div */
		assert(style_is_empty());

		/* Functions. */
		style_push(ST_DIV);
		print_heading_anchor_for(DIV_FUNCTION);
		division_act(DIV_FUNCTION, &segment_print_all);
		style_pop_strong();
	}
	/* License. */
	if(is_license) {
		style_push(ST_DIV);
		print_custom_heading_anchor_for(license, license_desc);
		style_push(ST_P);
		div_att_print(&is_div_preamble, ATT_LICENSE, SHOW_TEXT);
		style_pop_push();
		style_push(ST_LICENSE), style_push(ST_PLAIN);
		div_att_print(&is_not_div_preamble, ATT_LICENSE, SHOW_WHERE);
		style_pop_strong();
		style_pop_strong();
	}
	if(is_html) printf("</body>\n\n"
		"</html>\n");
	style_();
	return errno ? 0 : 1;
catch:
	fprintf(stderr, "Parsing failed.\n");
	return 0;
}
