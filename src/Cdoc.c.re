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
    variable be indistinguishable;
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
 @fixme Trigraph support, (haha.)
 @fixme Old-style function support.
 @fixme `re2c` appends a comma at the end of the enumeration list, not
 compliant with C90.
 @fixme Hide `const` on params when it can not affect function calls.
 @fixme Prototypes and functions are the same thing; this will confuse it. Hash
 map will be faster and more precise.
 @fixme Links to non-documented code which sometimes doesn't show up, work
 without error, and create broken links.
 @fixme Sometimes it's an error, sometimes it's a warning, seemingly at random.
 Make all the errors on-line.
 @fixme `cat file1 file2 > cdoc` is not cutting it because line numbers. Should
 be easy to load files individually.
 @fixme 80-characters _per_ line limit; I've got it working, just need to apply
 to this code. Needs buffering. */

#include <stdlib.h> /* EXIT */
#include <stdio.h>  /* fprintf */
#include <string.h> /* strcmp */
#include <errno.h>  /* errno */
#ifndef STRICT_ANSI
#include <unistd.h>
#endif /* STRICT_ANSI */
#include "../src/Scanner.h"
#include "../src/Report.h"
#include "../src/Semantic.h"
#include "../src/Cdoc.h"

static void usage(void) {
	fprintf(stderr, "cdoc <filename> [-h | --help] [-d | --debug]\n"
		"\t[(-d: | --directory:)<directory>]\n\n"
		"Given <filename>, a C file with encoded documentation,\n"
		"outputs that documentation.\n");
}

static struct {
	int debug;
	const char *fn, *dir;
	enum Output output;
} args;

static int parse_arg(const char *const string) {
	const char *s = string, *dir, *output;

/*!stags:re2c format = 'const char *@@;'; */

/*!re2c
	re2c:define:YYCTYPE = char;
	re2c:define:YYCURSOR = s;
	re2c:yyfill:enable = 0;

	end = "\x00";

	* {
		if(args.fn) return fprintf(stderr,
			"Error understanding \"%s\"; filename already provided \"%s\".\n",
			string, args.fn), 0;
		args.fn = string;
		return 1;
	}
	("-h" | "--help") end { usage(); exit(EXIT_SUCCESS); }
	("-d" | "--debug") end { args.debug = 1; return 1; }
	("-d:" | "--directory:") @dir [^\x00]+ end {
		if(args.dir) return fprintf(stderr,
			"Error understanding \"%s\"; directory already provided \"%s\".\n",
			dir, args.dir), 0;
		args.dir = dir;
		return 1;
	}
	("-o:" | "--output:") output_strings[OUT_HTML] end {
		if(args.output) return fprintf(stderr,
			"Error understanding \"%s\"; output already provided \"%s\".\n",
			string, output[args.output]);
		args.output = OUT_HTML;
		return 1;
	}
	("-o:" | "--output:") output_strings[OUT_MD] end {
		if(args.output) return fprintf(stderr,
			"Error understanding \"%s\"; output already provided \"%s\".\n",
			string, output[args.output]);
		args.output = OUT_MD;
		return 1;
	}
	"-" [^\x00]* end {
		return fprintf(stderr, "Unreconised option, \"%s\".\n", string), 0;
	}
*/
}

/** @return Whether the command-line option to print the scanner on `stderr`
 was set. */
int CdocOptionsDebug(void) {
	return args.debug;
}

/** @return What output was specified. */
enum Output CdocOptionsOutput(void) {
	return args.output == OUT_UNSPECIFIED ? OUT_MD : args.output;
}

/** @param[argc, argv] Argument vectors. */
int main(int argc, char **argv) {
	FILE *fp = 0;
	struct Scanner *scanner = 0;
	int exit_code = EXIT_FAILURE, i;

	for(i = 1; i < argc; i++) if(!parse_arg(argv[i])) goto catch;
	if(args.dir) {
#ifdef STRICT_ANSI
		fprintf(stderr,
		"Was compiled with STRICT_ANSI which does not support --directory.\n"
		), exit(EXIT_FAILURE);
#else /* !STRICT_ANSI */
		if(chdir(args.dir) == -1) goto catch;
#endif /* !STRICT_ANSI */
	}
	if(!(scanner = Scanner(args.fn, &ReportNotify))) goto catch;
	ReportWarn();
	ReportCull();
	if(!ReportOut()) goto catch;

	exit_code = EXIT_SUCCESS; goto finally;
	
catch:
	if(errno) {
		perror(args.fn ? args.fn : "(no file specified)");
	} else {
		usage();
	}
	
finally:
	Report_();
	Scanner_(scanner);
	if(fp) fclose(fp);

	return exit_code;
}
