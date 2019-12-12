/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 A context-sensitive parser intended to process parts of a `C` compilation unit
 and extract documentation, as well as outputting that documentation into the
 format specified. Designed to be very strict, warning one of documentation
 errors; and simple, made for self-contained independent documenatation. This
 does not do any compiling, just text-parsing. Thus, one can easily confuse by
 redefining symbols. However, it assumes the macro `A_B_(Foo,Bar)` is
 transformed into `<A>Foo<B>Bar`.

 Documentation commands are `/` `**…` (together) and are ended with `*…/`, but
 not `/` `*…*` `/`, (common code break.) Asterisks at the start of a line, like
 Kernel comments, or asterisks all over like some crazy ASCII art, are
 supported. Documentation appearing at most two lines above `typedef`, `tag`
 (`struct`, `enum`, `union`,) data, and functions, is associated therewith;
 everything else is automatically inserted into the description. Multiple
 documentation on the same command is appended. Two hard returns is a paragraph.
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
    `\\Omega` "\Omega", `\\times` "\times", `\\cdot` "\cdot".
 \* `\~` "~" non-breaking space;
 \* `\<url\>`: relative URIs must have a slash or a dot to distinguish it from
    text;
 \* `\<Source, 1999, pp. 1-2\>`: citation;
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
 \* `\@param[<param1>[, ...]]`: parameters, (multiple are concatenated using
     spaces, so this really should be sentence case);
 \* `\@author` (commas);
 \* `\@std`: standard, eg, `\@std GNU-C99`, (semicolons);
 \* `\@depend`: dependancy, (semicolons);
 \* `\@fixme`: something doesn't work as expected, (spaces);
 \* `\@return`: normal function return, (spaces);
 \* `\@throws[<exception1>[, ...]]`: exceptional function return; `C` doesn't
    have native exceptions, so `@throws` means whatever one desires; perhaps a
    null pointer or false is returned and `errno` is set to `exception1`,
    (spaces);
 \* `\@implements`: `C` doesn't have the concept of implements, but we
    would say that a function having a prototype of
    `(int (*)(const void *, const void *))` implements `bsearch` and `qsort`,
    (commas);
 \* `\@order`: comments about the run-time or space, (spaces);
 \* and `\@allow`, the latter being to allow `static` functions or data in the
    documentation, which are usually culled; one will be warned if this has any
    text.

 Perhaps the most striking difference from `Javadoc` and `Doxygen` is the
 `\@param` has to be followed by a braced list, (it's confusing to have the
 variable be indistinguishable from the text.)

 If one sets `md` as output, it goes to `GitHub` Markdown that is specifically
 visible on the `GitHub` page, (including working anchor links on browsers
 > 2000.) It bears little to Markdown supported in the documentation.

 @std C89
 @depend [re2c](http://re2c.org/)
 @fixme Old-style function support. Trigraph support, (haha.)
 @fixme Hide `const` on params when it can not affect function calls.
 @fixme Documentation on functions should be added to documentation on
 prototypes with the same (similar) prototype.
 @fixme Links to non-documented code which sometimes doesn't show up, work
 without error, and create broken links.
 @fixme 80-characters _per_ line limit, <https://xxyxyz.org/line-breaking/>,
 (needs buffering.)
 @fixme Eg, fixme with no args disappears; we should NOT check if the string is
 empty for these values. Better yet, have a flag.
 @fixme pre needs to be investigated in md; it's not doing
 "[fun1](#user-content-ATT_PARAM-407e96bd)" when link?
 @fixme This needs to be fixed: Somebody, Nobody, (fixme: Cee: , fixme:)...
 @fixme Now `dl...att` scrunched up; push `plain_text`? */

#include <stdlib.h> /* EXIT */
#include <stdio.h>  /* fprintf */
#include <string.h> /* strcmp */
#include <errno.h>  /* errno */
#include <assert.h> /* assert */
#include "../src/Path.h"
#include "../src/Text.h"
#include "../src/Buffer.h"
#include "../src/Scanner.h"
#include "../src/Report.h"
#include "../src/Semantic.h"
#include "../src/Cdoc.h"

static void usage(void) {
	fprintf(stderr, "Usage: cdoc [options] <input-file>\n"
		"Where options are:\n"
		"  -h | --help               This information.\n"
		"  -d | --debug              Prints a lot of debug information.\n"
		"  -f | --format (html | md) Overrides built-in guessing.\n"
		"  -o | --output <filename>  Stick the output file in this.\n"
		"\n"
		"Given <input-file>, a C file with encoded documentation,\n"
		"outputs that documentation.\n");
}

static struct {
	enum { EXPECT_NOTHING, EXPECT_OUT, EXPECT_FORMAT } expect;
	const char *in_fn, *out_fn;
	enum Format format;
	int debug, github;
} args;

/** Parses the one `argument`; global state may be modified.
 @return Success. */
static int parse_arg(const char *const argument) {
	const char *a = argument, *m = a;
	switch(args.expect) {
	case EXPECT_NOTHING: break;
	case EXPECT_OUT: assert(!args.out_fn); args.expect = EXPECT_NOTHING;
		args.out_fn = argument; return 1;
	case EXPECT_FORMAT: assert(!args.format); args.expect = EXPECT_NOTHING;
		if(!strcmp("md", argument)) args.format = OUT_MD;
		else if(!strcmp("html", argument)) args.format = OUT_HTML;
		else return 0;
		return 1;
	}
/*!re2c
	re2c:define:YYCTYPE = char;
	re2c:define:YYCURSOR = a;
	re2c:define:YYMARKER = m;
	re2c:yyfill:enable = 0;
	end = "\x00";
	// If it's not any other, it's probably an input filename?
	* {
		if(args.in_fn) return 0;
		args.in_fn = argument;
		return 1;
	}
	("-h" | "--help") end { usage(); exit(EXIT_SUCCESS); }
	("-d" | "--debug") end { args.debug = 1; return 1; }
	("-f" | "--format") end {
		if(args.format) return 0;
		args.expect = EXPECT_FORMAT;
		return 1;
	}
	("-o" | "--output") end {
		if(args.out_fn) return 0;
		args.expect = EXPECT_OUT;
		return 1;
	}
*/
}

/** @return Whether the command-line option to spam on `stderr` was set. */
int CdocGetDebug(void) {
	return args.debug;
}

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
		if(args.debug) fprintf(stderr, "Guess format is %s.\n",
			format_strings[args.format]);
	}
}

/** @return What format the output was specified to be in `enum Format`. If
 there was no output specified, guess before from the output filename. */
enum Format CdocGetFormat(void) {
	guess();
	assert(args.format > 0 && args.format <= 2);
	return args.format;
}

/** @return The input filename. */
const char *CdocGetInput(void) {
	return args.in_fn;
}

/** @return The output filename. */
const char *CdocGetOutput(void) {
	return args.out_fn;
}

/** @param[argc, argv] Argument vectors. */
int main(int argc, char **argv) {
	FILE *fp = 0;
	struct Scanner *scanner = 0;
	int exit_code = EXIT_FAILURE, i;
	struct Text *text = 0;

	/* Parse args. Expecting something more? */
	for(i = 1; i < argc; i++) if(!parse_arg(argv[i])) goto catch;
	if(args.expect) goto catch;

	/* This prints to `stdout`. If the args have specified that it goes into a
	 file, then redirect. */
	if(args.out_fn && !freopen(args.out_fn, "w", stdout)) goto catch;

	/* Set up the paths. */
	if(!Path(args.in_fn, args.out_fn)) goto catch;

	/* Buffer the file. */
	if(!(text = TextOpen(args.in_fn))) goto catch;

	/* Open the input file and parse. The last segment is on-going. */
	if(!(scanner = Scanner(TextBaseName(text), TextGet(text), &ReportNotify,
		SSCODE))) goto catch;
	ReportLastSegmentDebug();

	/* Output the results. */
	ReportWarn();
	ReportCull();
	if(!ReportOut()) goto catch;

	exit_code = EXIT_SUCCESS; goto finally;
	
catch:
	if(errno) {
		perror(args.in_fn ? args.in_fn : "(no file)");
	} else {
		usage();
	}
	
finally:
	Buffer_();
	Semantic_();
	Scanner_(&scanner);
	Report_();
	TextCloseAll();
	Path_();
	if(fp) fclose(fp);

	return exit_code;
}
