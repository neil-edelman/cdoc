 # Cdoc\.c\.re #

 * [Description](#user-content-heading-586264b9)
 * [Function Summary](#user-content-heading-10a44713)
 * [Function Definitions](#user-content-heading-6322e9d5)
 * [License](#user-content-heading-83e3344)

 ## <a id = "user-content-heading-586264b9" name = "user-content-heading-586264b9">Description</a> ##

This is a context\-sensitive lexer intended to process parts of a `C` compilation unit and extract documentation\. This does not do any compiling, just very basic text\-parsing\.

Documentation commands are `/` `**…` \(together\) and are ended with `*…/` , but not `/` `*…*` `/` ; one can still use this as a code break\. One can have an asterisk at the front, like Kernel comments, or asterisks all over like some crazy ASCII art\. All documentation goes at most two lines above what it documents or it's appended to the header\. Multiple documentation on the same command is appended, including in the command\. Two hard returns is a paragraph\. One can document typedefs, tags \(struct, enum, union,\) data, and functions; everything else is automatically inserted into the preamble\. The macro `A_B_(Foo,Bar)` is transformed into `<A>Foo<B>Bar` \.

This supports a stripped\-down version of `Markdown` that is much stricter\. Embedded inline in the documentation,

 * `\` escapes `_~!@<>[]` and "\`"; "\`" can not be represented in math/code, \(multiple escapes aren't supported\) but is in paragraph mode; in paragraph mode, the only ones that are needed except for ambiguous cases are \\\` and \\\_;
 * start lists with `\*` and end with a new paragraph; these are simple, can be anywhere and don't nest;
 * `\"` \(and optionally a space\) causes all the line after to be pre\-formatted;
 * Escapes included for convenience: `\,` "&#8239;" non\-breaking thin space, `\O` "&#927;" Bachmann–Landau notation, \(but really capital omicron because not many fonts have a shape for code\-point 120030,\) `\Theta` "&#920;", `\Omega` "&#937;", `\times` "&#215;", `\cdot` &#183;\.
 * `~` "&nbsp;" non\-breaking space;
 * \_emphasised\_: _emphasised_ ;
 * \`code/math\`: `code/math` ;
 * `<url>` : supports absolute URIs; relative URIs must have a slash or a dot to distinguish it;
 * `<Source, 1999, pp. 1-2>` : citation;
 * `<fn:<function>>` : function reference;
 * `<tag:<tag>>` : struct, union, or enum \(tag\) reference;
 * `<typedef:<typedef>>` : typedef reference;
 * `<data:<identifier>>` : data reference;
 * `[The link text](url)` : link;
 * `![Caption text](url.image)` : image;
 * a local include directive has a documentation comment immediately after that reads only `\include` , it will also be included in the documentation\.

Each\-block\-tags separate the documentation until the next paragraph or until the next each\-block\-tag, and specify a specific documentation structure\. Each\-block\-tags that overlap are concatenated in the file order\. Not all of these are applicable for all segments of text\. These are:

 * `@subtitle` : only makes sense for preamble, \(it doesn't matter what case one writes it, but multiple are concatenated using semicolons\);
 * `@param[<param1>[, ...]]` : parameters, \(multiple are concatenated using spaces, so this really should be sentence case\);
 * `@author` \(commas\);
 * `@std` : standard, eg, `@std GNU-C99` , \(semicolons\);
 * `@depend` : dependancy, \(semicolons\);
 * `@fixme` : something doesn't work as expected, \(spaces\);
 * `@return` : normal function return, \(spaces\);
 * `@throws[<exception1>[, ...]]` : exceptional function return; `C` doesn't have native exceptions, so `@throws` means whatever one desires; perhaps a null pointer or false is returned and `errno` is set to `exception1` , \(spaces\);
 * `@implements` : `C` doesn't have the concept of implements, but we would say that a function having a prototype of `(int (*)(const void *, const void *))` implements `bsearch` and `qsort` , \(commas\);
 * `@order` : comments about the run\-time or space, \(one single, eg, &#927;\(`arg` \) is encouraged, but spaces are used if multiple are concatenated\);
 * and `@allow` , the latter being to allow `static` functions or data in the documentation, which are usually culled; one will be warned if this has any text\.

Strict regular expressions that are much easier to parse have limited state and thus no notion of contextual position; as a result, tokens can be anywhere\. Differences in `Markdown` from `Doxygen` ,

 * no text headers;
 * no block\-quotes `>` ;
 * no four spaces or tab; use `\"` for preformatted;
 * no lists with `*` , `+` , numbered, or multi\-level;
 * no horizontal rules;
 * no emphasis by `*` , `**` , `__` , bold; \(fixme\!\)
 * no strikethrough;
 * no code spans by ```` , _etc_ ;
 * no table of contents;
 * no tables;
 * no fenced code blocks;
 * no HTML blocks;
 * no `/` `*!` , `///` , `//!` ;
 * no `\brief` ;
 * `/` `*!<` , `/` `**<` , `//!<` , `///<` : not needed; automatically concatenates;
 * no `[in]` , `[out]` , one should be able to tell from `const` ;
 * no `\param c1` or `@param a` \-\- this probably is the most departure from `Doxygen` , but it's confusing having the text and the variable be indistinguishable;
 * no titles with `![Caption text](/path/to/img.jpg "Image title")` HTML4;
 * no titles with `[The link text](http://example.net/ "Link title")` HTML4;
 * instead of `\struct` , `\union` , `\enum` , `\var` , `\typedef` , just insert the documentation comment above the thing; use `<data:<thing>>` to reference;
 * `\def` : no documenting macros;
 * instead of `\fn` , just insert the documentation comment above the function; use `<fn:<function>>` to reference;
 * internal underscores are emphasis except in math/code mode;
 * a \`cool' word in a \`nice' sentence must be escaped;

If one sets `md` as output, it goes to `GitHub` Markdown that is visible on the `GitHub` page\. It bears no resemblance to Markdown supported in the document\.



 * Standard:  
   C89
 * Dependancies:  
   [re2c](http://re2c.org/)
 * Caveat:  
   Old\-style function support\. Trigraph support, \(haha\.\) Hide `const` on params when it can not affect function calls\. Prototypes and functions are the same thing; this will confuse it\. Hash map might be faster and more precise\. Links to non\-documented code which sometimes doesn't show up, work without error, and create broken links\. 80\-characters _per_ line limit, [https://xxyxyz\.org/line\-breaking/](https://xxyxyz.org/line-breaking/), \(needs buffering\.\) Eg, fixme with no args disappears; we should NOT check if the string is empty\. If a segment has multiple licenses, they will show multiple times\. `foo.c:221` : space where it shouldn't be\. Why? `Array.h:304, SEE_FN "<T>Array_": link broken.` Probably because we're escaping it\. Arrrgg\! Fine, MD every anchor is hashed\.


 ## <a id = "user-content-heading-10a44713" name = "user-content-heading-10a44713">Function Summary</a> ##

<table>

<tr><th>Modifiers</th><th>Function Name</th><th>Argument List</th></tr>

<tr><td align = right>int</td><td><a href = "#fn-CdocGetDebug">CdocGetDebug</a></td><td></td></tr>

<tr><td align = right>enum Format</td><td><a href = "#fn-CdocGetFormat">CdocGetFormat</a></td><td></td></tr>

<tr><td align = right>const char *</td><td><a href = "#fn-CdocGetInput">CdocGetInput</a></td><td></td></tr>

<tr><td align = right>const char *</td><td><a href = "#fn-CdocGetOutput">CdocGetOutput</a></td><td></td></tr>

</table>



 ## <a id = "user-content-heading-6322e9d5" name = "user-content-heading-6322e9d5">Function Definitions</a> ##

 ### <a name = "fn:CdocGetDebug" id = "fixme fn:CdocGetDebug">CdocGetDebug</a> ###

<code>int <strong>CdocGetDebug</strong>(void)</code>

 - Return:  
   Whether the command\-line option to spam on `stderr` was set\.




 ### <a name = "fn:CdocGetFormat" id = "fixme fn:CdocGetFormat">CdocGetFormat</a> ###

<code>enum Format <strong>CdocGetFormat</strong>(void)</code>

 - Return:  
   What format the output was specified to be in `enum Format` \. If there was no output specified, guess before from the output filename\.




 ### <a name = "fn:CdocGetInput" id = "fixme fn:CdocGetInput">CdocGetInput</a> ###

<code>const char *<strong>CdocGetInput</strong>(void)</code>

 - Return:  
   The input filename\.




 ### <a name = "fn:CdocGetOutput" id = "fixme fn:CdocGetOutput">CdocGetOutput</a> ###

<code>const char *<strong>CdocGetOutput</strong>(void)</code>

 - Return:  
   The output filename\.






 ## <a id = "user-content-heading-83e3344" name = "user-content-heading-83e3344">License</a> ##

2019 Neil Edelman, distributed under the terms of the [MIT License](https://opensource.org/licenses/MIT)\.



