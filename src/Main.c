/** 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 This is a context-sensitive lexer intended to process parts of a `C`
 compilation unit and extract documentation. This documentation is an even more
 stripped-down, simplified, sort-of version of `Markup` then is used in
 `Doxygen`, with the intent of making it (much) stricter and simpler, and also
 suitable for sharing and compiling. This does not do any compiling, just very
 basic text-parsing, including the macro `A_B_(Foo,Bar) -> <A>Foo<B>Bar`.
 
 Documentation commands are { "/" "*" "*"+ } and are ended with { "*"+ "/" },
 but not { "/" "*"+ "*" "/" }; one can still use this as a code break. You can
 have an asterisk at the front, like Kernel comments, or asterisks all over
 like some crazy ASCII art, (unfortunately, this causes problems with lists
 being started with "*", so start them with " -* ".) All documentation goes at
 most two lines above what it documents or it's appended to the header.
 Multiple documentation on the same command is appended. Two hard returns is a
 paragraph. One can document typedefs, tags (struct, enum, union,) data, and
 functions; everything else is automatically inserted into the preamble.

 "\\" escapes whatever comes after, with one exception "\\," inserts a slim
 no-breaking space and backslash-end-comment is nonsense and will not compile.
 When two or more definitions are present in a single statement, the first one
 is used.

 Embedded inline in the documentation,

 -* \_ _emphasised_;
 -* \` `code/math`;
 -* \<url\>;
 -* \<Source, 1999, pp. 1-2\>;
 -* \<fn:\<function\>\>;
 -* \<tag:\<struct|union|enum\>\>;
 -* \<typedef:\<typedef\>\>;
 -* \<data:\<identifier\>\>;
 -* \[The link text\](url);
 -* \!\[Caption text\](/path/to/img.jpg);
 -* lists with "\ -* "; lists can be anywhere and don't nest;
 -* a tab causes all the line after to be preformatted, (except '*' ends, for
    checking if the comment is ended);
 -* \~ non-breaking space;
 -* \\, non-breaking thin space (U+202F HTML &#8239; for working with units.)

 Each-block-tags separate the documentation until the next paragraph or until
 the next each-block-tag, and specify a specific documentation structure.
 Each-block-tags that overlap are concatenated in the file order. Not all of
 these are applicable for all segments of text. These are:

 -* \@title;
 -* \@param[<param1>[, ...]];
 -* \@author;
 -* \@std (remove fixme);
 -* \@depend;
 -* \@version[<version>];
 -* \@fixme (remove fixme);
 -* \@bug (fixme);
 -* \@return;
 -* \@throws[<exception1>[, ...]];
 -* \@implements;
 -* \@order;
 -* and \@allow, the latter being to allow `static` functions in the
    documentation.

 Things that are not planned for inclusion,
 
 -* headers;
 -* block quotes "> ";
 -* lists with "*", "+", numbered, or multi-level;
 -* horizontal rules;
 -* emphasis by *, **, \_\_;
 -* strikethrough;
 -* code spans by \`\`, _etc_;
 -* table of contents;
 -* tables;
 -* fenced code blocks;
 -* HTML blocks;
 -* '/ * !', '/ / /', '/ / !';
 -* \\brief;
 -* '/ * ! <', '/ * * <', '/ / ! <', '/ / / <': not needed; automatically
    concatenates;
 -* [in], [out];
 -* '\\param c1' or '\@param a' -- this probably is the most departure from
    normal documentation generators, but it's confusing having the text and the
    variable be indistinguishable;
 -* Titles with \!\[Caption text\](/path/to/img.jpg "Image title");
 -* Titles with \[The link text\](http://example.net/ "Link title");
 -* instead of \\struct, \\union, \\enum, \\var, \\typedef, just insert the
    documentation comment above the thing; use \<data:<thing>\> to reference;
 -* \\def;
 -* instead of \\fn, just insert the documentation comment above the function;
    use \<fn:\<function\>\> to reference.
 
 "Unlike standard Markdown and Github Flavored Markdown doxygen will not touch
 internal underscores or stars or tildes, so the following will appear as-is:
 a_nice_identifier
 Furthermore, a * or _ only starts an emphasis if
 it is followed by an alphanumerical character, and
 it is preceded by a space, newline, or one the following characters <{([,:;
 An emphasis or a strikethrough ends if
 it is not followed by an alphanumerical character, and
 it is not preceded by a space, newline, or one the
 following characters ({[<=+-\@
 Lastly, the span of the emphasis or strikethrough is limited to a single
 paragraph."
 "Note that unlike standard Markdown, doxygen leaves the following untouched.
 A `cool' word in a `nice' sentence."

 @title Main.c
 @author Neil
 @version 2019-06
 @std C89
 @depend [re2c](http://re2c.org/)
 @fixme Old-style function definitions are not supported.
 @fixme \@depend in functions should automatically be placed in header.
 @fixme Warnings about un-documented variables, `this` or \@param[this].
 @fixme Authors can only be ASCII.
 @fixme Trigraph support, (haha.)
 @fixme Old-style function support.
 @bug `re2c` appends a comma at the end of the enumeration list, not compliant
 with C90. */

#include <stdlib.h> /* EXIT */
#include <stdio.h>  /* fprintf */
#include <string.h> /* strcmp */
#include "Scanner.h"
#include "Report.h"
#include "Semantic.h"

/** @param[argc, argv] If "debug", `freopens` a path that is on my computer. */
int main(int argc, char **argv) {
	int exit_code = EXIT_FAILURE;

	/* https://stackoverflow.com/questions/10293387/piping-into-application-run-under-xcode/13658537 */
	if (argc == 2 && strcmp(argv[1], "debug") == 0 ) {
		const char *test_file_path = "/Users/neil/Movies/Cdoc/foo.c";
		fprintf(stderr, "== [RUNNING IN DEBUG MODE with %s]==\n\n",
				test_file_path);
		freopen(test_file_path, "r", stdin);
	}

	/* `parser` is the thing that tells us which division it is by looking at
	 the code. */
	fputs("\n\n-- In --\n", stdout);
	if(!Scanner()) goto catch;
	fputs("\n\n-- Warn --\n", stdout);
	ReportWarn();
	fputs("\n\n-- Cull --\n", stdout);
	ReportCull();
	fputs("\n\n-- Debug --\n", stdout);
	ReportDebug();
	fputs("\n\n-- Out --\n", stdout);
	ReportOut();

	exit_code = EXIT_SUCCESS; goto finally;
	
catch:
	perror("scanner");
	
finally:
	Report_();
	Scanner_();
	Semantic(0);

	return exit_code;
}
