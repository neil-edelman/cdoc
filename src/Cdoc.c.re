/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 This is a context-sensitive lexer intended to process parts of a `C`
 compilation unit and extract documentation. This does not do any compiling,
 just very basic text-parsing.
 
 Documentation commands are `/``**…` and are ended with `*…/`, but not
 `/``*…*``/`; one can still use this as a code break. You can have an
 asterisk at the front, like Kernel comments, or asterisks all over like some
 crazy ASCII art.  All documentation goes at most two lines above what it
 documents or it's appended to the header. Multiple documentation on the same
 command is appended, including in the command. Two hard returns is a
 paragraph. One can document typedefs, tags (struct, enum, union,) data, and
 functions; everything else is automatically inserted into the preamble. The
 macro `A_B_(Foo,Bar)` is transformed into `<A>Foo<B>Bar`.

 This supports a stripped-down version of `Markdown` that is much stricter.
 Embedded inline in the documentation,

 \* `\\` escapes these `\*\_\`\~\!\\\@\<\>\[\]` but one only needed when
    ambiguous;
 \* start lists with `[ ]\\*[ ]` and end with a new paragraph; these are
    simple, can be anywhere and don't nest;
 \* `\\"` (and optionally a space) causes all the line after to be
    pre-formatted;
 \* Escapes included for convenience: `\\,` non-breaking thin space,
    `\\O` \O Bachmann–Landau notation, (but really capital omicron because
    fonts,) `\\Theta` \Theta, `\\Omega` \Omega, `\\times` \times,
    `\cdot` \cdot.
 \* `\~` non-breaking space;
 \* \_emphasised\_: _emphasised_;
 \* \`code/math\`: `code/math`;
 \* `\<url\>`: supports absolute URIs; relative URIs must have a slash or a dot
    to distinguish it;
 \* `\<Source, 1999, pp. 1-2\>`: citation;
 \* `\<fn:\<function\>\>`: function reference;
 \* `\<tag:\<tag\>\>`: struct, union, or enum (tag) reference;
 \* `\<typedef:\<typedef\>\>`: typedef reference;
 \* `\<data:\<identifier\>\>`: data reference;
 \* `\[The link text\](url)`: link;
 \* `\!\[Caption text\](url.image)`: image; fixme.

 Each-block-tags separate the documentation until the next paragraph or until
 the next each-block-tag, and specify a specific documentation structure.
 Each-block-tags that overlap are concatenated in the file order. Not all of
 these are applicable for all segments of text. These are:

 \* `\@title`: only makes sense for preamble; multiple are concatenated;
 \* `\@param[<param1>[, ...]]`: parameters;
 \* `\@author`;
 \* `\@std`: standard, eg, `\@std GNU-C99`;
 \* `\@depend`: dependancy;
 \* `\@fixme`: something doesn't work as expected;
 \* `\@return`: normal function return;
 \* `\@throws[<exception1>[, ...]]`: exceptional function return; `C` doesn't
    have native exceptions, so `@throws` means whatever one desires; perhaps a
    null pointer or false is returned and `errno` is set to this `exception1`;
 \* `\@implements`: `C` doesn't have the concept of implement, but we
    would say that a function having a prototype of
    `(int (*)(const void *, const void *))` implements `bsearch` and `qsort`;
 \* `\@order`: comments about the run-time or space;
 \* and `\@allow`, the latter being to allow `static` functions or data in the
    documentation, which are usually culled.

 Strict regular expressions that are much easier to parse have limited state
 and thus no notion of contextual position; as a result, tokens can be
 anywhere. Differences in `Markdown` from `Doxygen`,

 \* no headers;
 \* no block-quotes `> `;
 \* no four spaces or tab; use `\\"` for preformatted;
 \* no lists with `*`, `+`, numbered, or multi-level;
 \* no horizontal rules;
 \* no emphasis by `*`, `**`, `\_\_`, bold;
 \* no strikethrough;
 \* no code spans by `\`\``, _etc_;
 \* no table of contents;
 \* no tables;
 \* no fenced code blocks;
 \* no HTML blocks;
 \* no `/``*!`, `///`, `//!`;
 \* no `\\brief`;
 \* `/``*!<`, `/``**<`, `//!<`, `///<`: not needed; automatically concatenates;
 \* no `[in]`, `[out]`, one should be able to tell from `const`;
 \* no `\\param c1` or `\@param a` -- this probably is the most departure from
    normal documentation generators, but it's confusing having the text and the
    variable be indistinguishable and complicates the state;
 \* no titles with `\!\[Caption text\](/path/to/img.jpg "Image title")` HTML4;
 \* no titles with `\[The link text\](http://example.net/ "Link title")` HTML4;
 \* instead of `\\struct`, `\\union`, `\\enum`, `\\var`, `\\typedef`, just
    insert the documentation comment above the thing; use `\<data:<thing>\>` to
    reference;
 \* `\\def`: no documenting macros;
 \* instead of `\\fn`, just insert the documentation comment above the
    function; use `\<fn:\<function\>\>` to reference;
 \* internal underscores are emphasis except in math/code mode;
 \* A `cool' word in a `nice' sentence must be escaped.

 @title Main.c
 @author Neil
 @std C89
 @depend [re2c](http://re2c.org/)
 @fixme Nothing prevents invalid html from being output, for example, having a
 link in the title. Switch to md if that happens?
 @fixme Authors can only be ASCII.
 @fixme Trigraph support, (haha.)
 @fixme Old-style function support.
 @fixme `re2c` appends a comma at the end of the enumeration list, not
 compliant with C90.
 @fixme Hide `const` on params when it can not affect function calls.
 @fixme Prototypes and functions are the same thing; this will confuse it. Hash
 map will be faster and more precise.
 @fixme Links to non-documented code which sometimes doesn't show up, work.
 @fixme Sometimes it's an error, sometimes it's a warning, seemingly at random.
 Make all the errors on-line.
 @fixme `cat file1 file2 > cdoc` is not cutting it because line numbers. Should
 be easy to load files individually. */

#include <stdlib.h> /* EXIT */
#include <stdio.h>  /* fprintf */
#include <string.h> /* strcmp */
#include <errno.h>  /* errno */
#include "../src/Scanner.h"
#include "../src/Report.h"
#include "../src/Semantic.h"
#include "../src/Cdoc.h"

static struct {
	int provide_file, print_scanner;
} args;

static int parse_arg(const char *const string) {
	const char *m, *s = string;
	/* !stags:re2c format = 'const char *@@;'; */

/*!re2c
	re2c:define:YYCTYPE = char;
	re2c:define:YYCURSOR = s;
	re2c:define:YYMARKER = m;
	re2c:yyfill:enable = 0;

	end = "\x00";

	* { fprintf(stderr, "Error with argument \"%s\".\n", string); return 0; }
	"-d" | "--debug" end { args.provide_file = 1; return 1; }
	"-s" | "--scanner" end { args.print_scanner = 1; return 1; }
*/
}

/** @return Whether the command-line option to print the scanner on `stderr`
 was set. */
int CdocOptionsScanner(void) {
	return args.print_scanner;
}

/** @param[argc, argv] If "debug", `freopens` a path that is on my computer. */
int main(int argc, char **argv) {
	int exit_code = EXIT_FAILURE, i;
	const char *reason = 0;

	for(i = 1; i < argc; i++) if(!parse_arg(argv[i]))
		{ errno = EDOM; reason = "unreconised argument"; goto catch; }

	/* https://stackoverflow.com/questions/10293387/piping-into-application-run-under-xcode/13658537 */
	if(args.provide_file) {
		const char *test_file_path = "/Users/neil/Movies/Cdoc/foo.c";
		fprintf(stderr, "== [RUNNING IN DEBUG MODE with %s]==\n\n",
				test_file_path);
		freopen(test_file_path, "r", stdin);
	}

	if(!Scanner(&ReportNotify)) { reason = "scanner"; goto catch; }
	ReportWarn();
	ReportCull();
	if(!ReportOut()) { reason = "output"; goto catch; }

	exit_code = EXIT_SUCCESS; goto finally;
	
catch:
	perror(reason);
	
finally:
	Report_();
	Scanner_();

	return exit_code;
}
