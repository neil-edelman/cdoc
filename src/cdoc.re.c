/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 Static documentation generator for one `C` translation unit. This is not a
 full `C` context-free grammar decoder; it is limited to regular grammar.
 Remapping with the pre-processor, complex types, K&R style function
 definitions, trigraphs, are confusing to it. It just guesses based on
 reasonable modern coding-standards. However, it assumes that capital latin
 letters underscores are concatenation commands for the pre-processor, such
 that `A_BC_(foo,bar)` is transformed into `<A>foo<BC>bar`.

 In keeping with `Javadoc` and `Doxygen`, documentation commands are `/` `**…`
 (together) and are ended with `*…/`, but not `/` `*…*` `/`. Accounts for
 asterisks at the start of a line, (Kernel comments,) or asterisks on both
 sides, (ASCII art?) Documentation appearing at most two lines above `typedef`,
 `tag` (`struct`, `enum`, `union`,) data, and functions, is associated
 therewith; everything else is automatically inserted into the description.
 Multiple documentation on the same command is appended. Two hard returns is a
 paragraph. The big difference is the fixed `\@param` notation; now it has to
 be followed by a braced list, `\@param\[a, b\] Concatenates both.`.

 Supports some `Markdown` commands included in the documentation,

 \* `\\` escapes `_\~!\@<>[]` and "\`"; "\`" can not be represented in
    math/code, (multiple escapes aren't supported); in paragraph mode, except
    in ambiguous cases, the only ones that are needed are \\\` and \\\_;
 \* \_emphasised\_: _emphasised_;
 \* \`code/math\`: `code/math`;
 \* start lists with ` \\* ` (including spaces) and end with a new paragraph;
    these are simple, can be anywhere and don't nest;
 \* `\\"` (and optionally a space) causes all the line after to be
    pre-formatted;
 \* Escapes included for convenience: `\\,` "\," non-breaking thin space,
    `\\O` "\O" Bachmann–Landau notation, (but really capital omicron because
    not many fonts have a shape for code-point 120030,) `\\Theta` "\Theta",
    `\\Omega` "\Omega", `\\times` "\times", `\\cdot` "\cdot", `\\log` "\log".
 \* `\~` "~" non-breaking space;
 \* `\<url\>`: relative URIs must have a slash or a dot to distinguish it from
    text;
 \* `\<Source, 1999, pp. 1-2\>`: citation, searches on Google scholar;
 \* `\<fn:\<function\>\>`: function reference;
 \* `\<tag:\<tag\>\>`: struct, union, or enum (tag) reference;
 \* `\<typedef:\<typedef\>\>`: typedef reference;
 \* `\<data:\<identifier\>\>`: data reference;
 \* `\[The link text\](url)`: link, Markdown doesn't like `{}[]()` in text;
 \* `\!\[Caption text\](url.image)`: image, same;
 \* a local include directive has a documentation comment immediately
    after that reads only `\include`, it will also be included in the
    documentation.

 Each-block-tags separate the documentation until the next paragraph or until
 the next each-block-tag, and specify a specific documentation structure.
 Each-block-tags that overlap are concatenated in the file order. Not all of
 these are applicable for all segments of text. These are:

 \* `\@subtitle`: only makes sense for preamble, (it doesn't matter what case
    one writes it, but multiple are concatenated using semicolons);
 \* `\@abstract`: tl;dr appearing first;
 \* `\@param[<param1>[, ...]]`: parameters, (multiple are concatenated using
     spaces, so this really should be sentence case);
 \* `\@author` (commas);
 \* `\@std`: standard, eg, `\@std GNU-C99`, (semicolons);
 \* `\@depend`: dependancy, (semicolons);
 \* `\@fixme`: something doesn't work as expected, (spaces);
 \* `\@return`: normal function return, (spaces);
 \* `\@throws[<exception1>[, ...]]`: exceptional function return; `C` doesn't
    have native exceptions, so `@throws` means whatever one desires; perhaps a
    null pointer or false is returned and `errno` is set to `exception1`;
 \* `\@implements`: `C` doesn't have the concept of implements, but we
    would say that a function having a prototype of
    `(int (*)(const void *, const void *))` implements `bsearch` and `qsort`;
 \* `\@order`: comments about the run-time or space;
 \* and `\@allow`, the latter being to allow `static` functions or data in the
    documentation, which are usually culled.

 If one sets `md` as output, it goes to `GitHub` Markdown that is specifically
 visible on the `GitHub` page, (including working anchor links on browsers
 > 2000.) This is not the Markdown supported in the documentation.

 @std C89
 @depend [re2c](http://re2c.org/)
 @fixme Prototype function parameters ignore `const`.
 @fixme Documentation on prototypes.
 @fixme Links to non-documented code which sometimes doesn't show up, work
 without error, and create broken links.
 @fixme A fixme with no args disappears; we should NOT check if the string is
 empty for these values.
 @fixme Documentation on global variables is not output. */

#include <stdlib.h> /* EXIT */
#include <stdio.h>  /* fprintf */
#include <string.h> /* strcmp */
#include <errno.h>  /* errno */
#include <assert.h> /* assert */
#include "../src/buffer.h"
#include "../src/report.h"
#include "../src/semantic.h"
#include "../src/cdoc.h"

/*!re2c
re2c:define:YYCTYPE = char;
re2c:define:YYCURSOR = a;
re2c:define:YYMARKER = m;
re2c:yyfill:enable = 0;
end = [\x00];
*/

static void usage(void) {
	fprintf(stderr,
		"Given <input-file>, a C file with encoded documentation,\n"
		"outputs that documentation.\n"
		"\n"
		"Usage: cdoc [options] <input-file>\n"
		"Where options are:\n"
		"  -h | --help               This information.\n"
		"  -d | --debug <read | output | semantic | hash | erase | style>\n"
		"                            Prints debug information.\n"
		"  -f | --format <html | md> Overrides built-in guessing.\n"
		"  -o | --output <filename>  Output file.\n");
}

static struct {
	enum { EXPECT_NOTHING, EXPECT_DEBUG, EXPECT_OUT, EXPECT_FORMAT } expect;
	const char *in_fn, *out_fn;
	enum format format;
	enum debug debug;
} args;

/** Parses the one `argument` to `args`. @return Success. */
static int parse_arg(const char *const argument) {
	const char *a = argument, *m = a;
	switch(args.expect) {
	case EXPECT_NOTHING: break;
	case EXPECT_OUT: assert(!args.out_fn); args.expect = EXPECT_NOTHING;
		args.out_fn = argument; return 1;
	case EXPECT_DEBUG: args.expect = EXPECT_NOTHING;
/*!re2c
	*              { return 0; }
	"read" end     { args.debug |= DBG_READ; return 1; }
	"output" end   { args.debug |= DBG_OUTPUT; return 1; }
	"semantic" end { args.debug |= DBG_SEMANTIC; return 1; }
	"hash" end     { args.debug |= DBG_HASH; return 1; }
	"erase" end    { args.debug |= DBG_ERASE; return 1; }
	"style" end    { args.debug |= DBG_STYLE; return 1; }
*/
	case EXPECT_FORMAT: assert(!args.format); args.expect = EXPECT_NOTHING;
/*!re2c
	*      { return 0; }
	"md"   end { args.format = OUT_MD; return 1; }
	"html" end { args.format = OUT_HTML; return 1; }
*/
	}
/*!re2c
	// If it's not any other, it's probably an input filename?
	* { if(args.in_fn) return 0; args.in_fn = argument; return 1; }
	("-h" | "--help") end { usage(); exit(EXIT_SUCCESS); }
	("-d" | "--debug") end { args.expect = EXPECT_DEBUG; return 1; }
	("-f" | "--format") end
		{ if(args.format) return 0; args.expect = EXPECT_FORMAT; return 1; }
	("-o" | "--output") end
		{ if(args.out_fn) return 0; args.expect = EXPECT_OUT; return 1; }
*/
}

/** @return Whether the command-line was set. */
enum debug cdoc_get_debug(void) { return args.debug; }

/** @return True if `suffix` is a suffix of `string`. */
static int is_suffix(const char *const string, const char *const suffix) {
	const size_t str_len = strlen(string), suf_len = strlen(suffix);
	assert(string && suffix);
	if(str_len < suf_len) return 0;
	return !strncmp(string + str_len - suf_len, suffix, suf_len);
}

static void guess(void) {
	if(args.format == OUT_RAW) {
		if(args.out_fn && (is_suffix(args.out_fn, ".html")
			|| is_suffix(args.out_fn, ".htm"))) args.format = OUT_HTML;
		else args.format = OUT_MD;
		if(args.debug & DBG_OUTPUT) fprintf(stderr, "Guess format is %s.\n",
			format_strings[args.format]);
	}
}

/** @return What format the output was specified to be in `enum format`. If
 there was no output format specified, guess. */
enum format cdoc_get_format(void) {
	guess();
	assert(args.format > 0 && args.format <= 2);
	return args.format;
}

/** @return The input filename. */
const char *cdoc_get_input(void) { return args.in_fn; }

/** @return The output filename. */
const char *cdoc_get_output(void) { return args.out_fn; }

/** @param[argc, argv] Argument vectors. */
int main(int argc, char **argv) {
	FILE *fp = 0;
	struct scanner *scan = 0;
	int exit_code = EXIT_FAILURE, i;
	struct text *text = 0;

	errno = 0;
	for(i = 1; i < argc; i++) if(!parse_arg(argv[i])) goto catch;
	if(!args.in_fn || args.expect) goto catch;

	/* If the args have specified that it goes into a file, then redirect. */
	if(args.out_fn && !freopen(args.out_fn, "w", stdout)) goto catch;

	/* Set up the urls. */
	if(!url(args.in_fn, args.out_fn)) goto catch;

	/* buffer the file. */
	if(!(text = text_open(args.in_fn))) goto catch;

	/* Open the input file and parse. The last segment is on-going. */
	if(!(scan = scanner(text_base_name(text), text_get(text), &report_notify,
		START_CODE))) goto catch;
	report_last_segment_debug();

	/* Output the results. */
	report_warn();
	report_cull();
	if(!report_out()) goto catch;

	exit_code = EXIT_SUCCESS; goto finally;

catch:
	if(errno) fprintf(stderr, "In main. "),
		perror(args.in_fn ? args.in_fn : "(no file)");
	else usage();

finally:
	scanner_(&scan);
	report_();
	text_close_all();
	url_();
	buffer_(); /* Should be after ~report because might do debug print. */
	if(fp) fclose(fp);

	return exit_code;
}
