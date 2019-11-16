 # Cdoc #

 * <a href = "#preamble:">Preamble</a>
 * <a href = "#summary:">Function Summary</a>
 * <a href = "#fn:">Function Definitions</a>
 * <a href = "#license:">License</a>

<a name = "preamble:"><!-- --></a>
<h2>Preamble</h2>

This is a context\-sensitive lexer intended to process parts of a `C` compilation unit and extract documentation\. This does not do any compiling, just very basic text\-parsing\.

Documentation commands are `/``\*\*…` and are ended with `\*…/`, but not `/``\*…\*``/`; one can still use this as a code break\. You can have an asterisk at the front, like Kernel comments, or asterisks all over like some crazy ASCII art\. All documentation goes at most two lines above what it documents or it's appended to the header\. Multiple documentation on the same command is appended, including in the command\. Two hard returns is a paragraph\. One can document typedefs, tags \(struct, enum, union,\) data, and functions; everything else is automatically inserted into the preamble\. The macro `A\_B\_\(Foo,Bar\)` is transformed into `<A>Foo<B>Bar`\.

This supports a stripped\-down version of `Markdown` that is much stricter\. Embedded inline in the documentation,

 * `\\` escapes these `\*\_\`~\!\\@<>\[\]` but one only needed when ambiguous;
 * start lists with `\[ \]\\\*\[ \]` and end with a new paragraph; these are simple, can be anywhere and don't nest;
 * `\\"` \(and optionally a space\) causes all the line after to be pre\-formatted;
 * Escapes included for convenience: `\\,` non\-breaking thin space, `\\O` &#927 Bachmann–Landau notation, \(but really capital omicron because fonts,\) `\\Theta` &#920;, `\\Omega` &Omega;, `\\times` &#215;, `\\cdot` &#183;\.
 * `~` non\-breaking space;
 * \_emphasised\_: _emphasised_;
 * \`code/math\`: `code/math`;
 * `<url>`: supports absolute URIs; relative URIs must have a slash or a dot to distinguish it;
 * `<Source, 1999, pp\. 1\-2>`: citation;
 * `<fn:<function>>`: function reference;
 * `<tag:<tag>>`: struct, union, or enum \(tag\) reference;
 * `<typedef:<typedef>>`: typedef reference;
 * `<data:<identifier>>`: data reference;
 * `\[The link text\]\(url\)`: link;
 * `\!\[Caption text\]\(url\.image\)`: image;

As well, if a local include directive has a documentation comment immediately after that reads only `\\include`, it will also be included in the documentation\.

Each\-block\-tags separate the documentation until the next paragraph or until the next each\-block\-tag, and specify a specific documentation structure\. Each\-block\-tags that overlap are concatenated in the file order\. Not all of these are applicable for all segments of text\. These are:

 * `@title`: only makes sense for preamble; multiple are concatenated;
 * `@param\[<param1>\[, \.\.\.\]\]`: parameters;
 * `@author`;
 * `@std`: standard, eg, `@std GNU\-C99`;
 * `@depend`: dependancy;
 * `@fixme`: something doesn't work as expected;
 * `@return`: normal function return;
 * `@throws\[<exception1>\[, \.\.\.\]\]`: exceptional function return; `C` doesn't have native exceptions, so `@throws` means whatever one desires; perhaps a null pointer or false is returned and `errno` is set to this `exception1`;
 * `@implements`: `C` doesn't have the concept of implement, but we would say that a function having a prototype of `\(int \(\*\)\(const void \*, const void \*\)\)` implements `bsearch` and `qsort`;
 * `@order`: comments about the run\-time or space;
 * and `@allow`, the latter being to allow `static` functions or data in the documentation, which are usually culled\.

Strict regular expressions that are much easier to parse have limited state and thus no notion of contextual position; as a result, tokens can be anywhere\. Differences in `Markdown` from `Doxygen`,

 * no headers;
 * no block\-quotes `> `;
 * no four spaces or tab; use `\\"` for preformatted;
 * no lists with `\*`, `\+`, numbered, or multi\-level;
 * no horizontal rules;
 * no emphasis by `\*`, `\*\*`, `\_\_`, bold;
 * no strikethrough;
 * no code spans by `\`\``, _etc_;
 * no table of contents;
 * no tables;
 * no fenced code blocks;
 * no HTML blocks;
 * no `/``\*\!`, `///`, `//\!`;
 * no `\\brief`;
 * `/``\*\!<`, `/``\*\*<`, `//\!<`, `///<`: not needed; automatically concatenates;
 * no `\[in\]`, `\[out\]`, one should be able to tell from `const`;
 * no `\\param c1` or `@param a` \-\- this probably is the most departure from normal documentation generators, but it's confusing having the text and the variable be indistinguishable;
 * no titles with `\!\[Caption text\]\(/path/to/img\.jpg "Image title"\)` HTML4;
 * no titles with `\[The link text\]\(http://example\.net/ "Link title"\)` HTML4;
 * instead of `\\struct`, `\\union`, `\\enum`, `\\var`, `\\typedef`, just insert the documentation comment above the thing; use `<data:<thing>>` to reference;
 * `\\def`: no documenting macros;
 * instead of `\\fn`, just insert the documentation comment above the function; use `<fn:<function>>` to reference;
 * internal underscores are emphasis except in math/code mode;
 * A `cool' word in a `nice' sentence must be escaped\.

Note that it does not validate html; nothing stops one from writing eg, a link, or emphasis, in the title\.

 * Author:  
   Neil
 * Standard:  
   C89
 * Dependancy:  
   [re2c](http://re2c.org/)
 * Caveat:  
   Trigraph support, \(haha\.\) Old\-style function support\. Hide `const` on params when it can not affect function calls\. Prototypes and functions are the same thing; this will confuse it\. Hash map will be faster and more precise\. Links to non\-documented code which sometimes doesn't show up, work without error, and create broken links\. Sometimes it's an error, sometimes it's a warning, seemingly at random\. Make all the errors on\-line? 80\-characters _per_ line limit; I've got it working, just need to apply to this code\. Needs buffering\. Eg, fixme with no args disappears; we should NOT check if the string is empty\. For md, have a field in `Style` that says whether we should escape all, or just some, \(eg, inside a \`\` the md changes\.\) Complete md\-ising eg table\. `A``B` doesn't do what one expects in md\. &\#927 is not reconised\. FormatIndex is stupid; just create a zero in the front\.




<a name = "summary:"><!-- --></a><h2>Function Summary</h2>

<table>

<tr><th>Modifiers</th><th>Function Name</th><th>Argument List</th></tr>

<tr><td align = right>int</td><td><a href = "#fn:CdocGetDebug">CdocGetDebug</a></td><td></td></tr>

<tr><td align = right>enum Format</td><td><a href = "#fn:CdocGetFormat">CdocGetFormat</a></td><td></td></tr>

<tr><td align = right>int</td><td><a href = "#fn:CdocGetFormatIndex">CdocGetFormatIndex</a></td><td></td></tr>

<tr><td align = right>const char \*</td><td><a href = "#fn:CdocGetOutput">CdocGetOutput</a></td><td></td></tr>

</table>



<a name = "fn:"><!-- --></a><h2>Function Definitions</h2>

<a name = "fn:CdocGetDebug"><!-- --></a>
 ### CdocGetDebug ###

`int<strong> CdocGetDebug</strong>\(void\)`

 - Return:  
   Whether the command\-line option to print the scanner on `stderr` was set\.




<a name = "fn:CdocGetFormat"><!-- --></a>
 ### CdocGetFormat ###

`enum Format<strong> CdocGetFormat</strong>\(void\)`

 - Return:  
   What format the output was specified to be in `enum Format`\. If there was no output specified, guess before from the output filename\.




<a name = "fn:CdocGetFormatIndex"><!-- --></a>
 ### CdocGetFormatIndex ###

`int<strong> CdocGetFormatIndex</strong>\(void\)`

 - Return:  
   What the format is in an index\.




<a name = "fn:CdocGetOutput"><!-- --></a>
 ### CdocGetOutput ###

`const char \*<strong>CdocGetOutput</strong>\(void\)`

 - Return:  
   The output filename\.






<a name = "license:"><!-- --></a>
<h2>License</h2>

2019 Neil Edelman, distributed under the terms of the [MIT License](https://opensource.org/licenses/MIT)\.



