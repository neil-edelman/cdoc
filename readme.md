 # Cdoc\.c\.re #

 * [Description](#user-content-part-a5e43a39)
 * [Function Summary](#user-content-part-5b9c058b)
 * [Function Definitions](#user-content-part-fef6433d)
 * [License](#user-content-part-77eb0f5a)

 ## <a id = "user-content-part-a5e43a39" name = "user-content-part-a5e43a39">Description</a> ##

This is a context\-sensitive lexer intended to process parts of a `C` compilation unit and extract documentation\. This does not do any compiling, just very basic text\-parsing\.

Documentation commands are `/` `**…` \(together\) and are ended with `*…/` , but not `/` `*…*` `/` ; one can still use this as a code break\. One can have an asterisk at the front, like Kernel comments, or asterisks all over like some crazy ASCII art\. All documentation goes at most two lines above what it documents or it's appended to the header\. Multiple documentation on the same command is appended, including in the command\. Two hard returns is a paragraph\. One can document typedefs, tags \(struct, enum, union,\) data, and functions; everything else is automatically inserted into the preamble\. The macro `A_B_(Foo,Bar)` is transformed into `&lt;A&gt;Foo&lt;B&gt;Bar` \.

This supports a stripped\-down version of `Markdown` that is much stricter\. Embedded inline in the documentation,

 * `\` escapes `_~!@&lt;&gt;[]` and "\`"; "\`" can not be represented in math/code, \(multiple escapes aren't supported\) but is in paragraph mode; in paragraph mode, the only ones that are needed except for ambiguous cases are \\\` and \\\_;
 * start lists with `\*` and end with a new paragraph; these are simple, can be anywhere and don't nest;
 * `\"` \(and optionally a space\) causes all the line after to be pre\-formatted;
 * Escapes included for convenience: `\,` "&#8239;" non\-breaking thin space, `\O` "&#927;" Bachmann–Landau notation, \(but really capital omicron because not many fonts have a shape for code\-point 120030,\) `\Theta` "&#920;", `\Omega` "&#937;", `\times` "&#215;", `\cdot` &#183;\.
 * `~` "&nbsp;" non\-breaking space;
 * \_emphasised\_: _emphasised_ ;
 * \`code/math\`: `code/math` ;
 * `&lt;url&gt;` : supports absolute URIs; relative URIs must have a slash or a dot to distinguish it;
 * `&lt;Source, 1999, pp. 1-2&gt;` : citation;
 * `&lt;fn:&lt;function&gt;&gt;` : function reference;
 * `&lt;tag:&lt;tag&gt;&gt;` : struct, union, or enum \(tag\) reference;
 * `&lt;typedef:&lt;typedef&gt;&gt;` : typedef reference;
 * `&lt;data:&lt;identifier&gt;&gt;` : data reference;
 * `[The link text](url)` : link;
 * `![Caption text](url.image)` : image;
 * a local include directive has a documentation comment immediately after that reads only `\include` , it will also be included in the documentation\.

Each\-block\-tags separate the documentation until the next paragraph or until the next each\-block\-tag, and specify a specific documentation structure\. Each\-block\-tags that overlap are concatenated in the file order\. Not all of these are applicable for all segments of text\. These are:

 * `@subtitle` : only makes sense for preamble, \(it doesn't matter what case one writes it, but multiple are concatenated using semicolons\);
 * `@param[&lt;param1&gt;[, ...]]` : parameters, \(multiple are concatenated using spaces, so this really should be sentence case\);
 * `@author` \(commas\);
 * `@std` : standard, eg, `@std GNU-C99` , \(semicolons\);
 * `@depend` : dependancy, \(semicolons\);
 * `@fixme` : something doesn't work as expected, \(spaces\);
 * `@return` : normal function return, \(spaces\);
 * `@throws[&lt;exception1&gt;[, ...]]` : exceptional function return; `C` doesn't have native exceptions, so `@throws` means whatever one desires; perhaps a null pointer or false is returned and `errno` is set to `exception1` , \(spaces\);
 * `@implements` : `C` doesn't have the concept of implements, but we would say that a function having a prototype of `(int (*)(const void *, const void *))` implements `bsearch` and `qsort` , \(commas\);
 * `@order` : comments about the run\-time or space, \(one single, eg, &#927;\(`arg` \) is encouraged, but spaces are used if multiple are concatenated\);
 * and `@allow` , the latter being to allow `static` functions or data in the documentation, which are usually culled; one will be warned if this has any text\.

Strict regular expressions that are much easier to parse have limited state and thus no notion of contextual position; as a result, tokens can be anywhere\. Differences in `Markdown` from `Doxygen` ,

 * no text headers;
 * no block\-quotes `&gt;` ;
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
 * `/` `*!&lt;` , `/` `**&lt;` , `//!&lt;` , `///&lt;` : not needed; automatically concatenates;
 * no `[in]` , `[out]` , one should be able to tell from `const` ;
 * no `\param c1` or `@param a` \-\- this probably is the most departure from `Doxygen` , but it's confusing having the text and the variable be indistinguishable;
 * no titles with `![Caption text](/path/to/img.jpg "Image title")` HTML4;
 * no titles with `[The link text](http://example.net/ "Link title")` HTML4;
 * instead of `\struct` , `\union` , `\enum` , `\var` , `\typedef` , just insert the documentation comment above the thing; use `&lt;data:&lt;thing&gt;&gt;` to reference;
 * `\def` : no documenting macros;
 * instead of `\fn` , just insert the documentation comment above the function; use `&lt;fn:&lt;function&gt;&gt;` to reference;
 * internal underscores are emphasis except in math/code mode;
 * a \`cool' word in a \`nice' sentence must be escaped;

If one sets `md` as output, it goes to `GitHub` Markdown that is visible on the `GitHub` page\. It bears no reseblance to Markdown suppoted in the document\.

 * Standard:  
   C89
 * Dependancies:  
   [re2c](http://re2c.org/)
 * Caveat:  
   Old\-style function support\. Trigraph support, \(haha\.\) Hide `const` on params when it can not affect function calls\. Prototypes and functions are the same thing; this will confuse it\. Hash map might be faster and more precise\. Links to non\-documented code which sometimes doesn't show up, work without error, and create broken links\. 80\-characters _per_ line limit, [https://xxyxyz\.org/line\-breaking/](https://xxyxyz.org/line-breaking/), \(needs buffering\.\) Eg, fixme with no args disappears; we should NOT check if the string is empty\. `A` `B` doesn't do what one expects in md\. If a segment has multiple licenses, they will show multiple times\. `foo.c:221` : space where it shouldn't be\. Why? `Array.h:304, SEE_FN "&lt;T&gt;Array_": link broken.` Why? Some help Markdown is; &lt;&gt; need to be escaped in all tables and headings\. It's a mess\. Arrrgg\! Fine, MD every anchor is hashed\. Ugly &lt;strong&gt;&lt;code&gt; blocks in MD\. Replace \` by literal `&lt;code&gt;` and it should be fine\.




 ## <a name = "summary-">Function Summary</a> ##

<table>

<tr><th>Modifiers</th><th>Function Name</th><th>Argument List</th></tr>

<tr><td align = right>int</td><td><a href = "#fn-CdocGetDebug">CdocGetDebug</a></td><td></td></tr>

<tr><td align = right>enum Format</td><td><a href = "#fn-CdocGetFormat">CdocGetFormat</a></td><td></td></tr>

<tr><td align = right>const char *</td><td><a href = "#fn-CdocGetInput">CdocGetInput</a></td><td></td></tr>

<tr><td align = right>const char *</td><td><a href = "#fn-CdocGetOutput">CdocGetOutput</a></td><td></td></tr>

</table>



 ## <a id = "user-content-part-bd16950b" name = "user-content-part-bd16950b">Struct, Union, and Enum Definitions</a> ##

 ### <a name = "fn:CdocGetDebug" id = "fixme fn:CdocGetDebug">CdocGetDebug</a> ###

`int `**`CdocGetDebug`**`(void)`

 - Return:  
   Whether the command\-line option to spam on `stderr` was set\.




 ### <a name = "fn:CdocGetFormat" id = "fixme fn:CdocGetFormat">CdocGetFormat</a> ###

`enum Format `**`CdocGetFormat`**`(void)`

 - Return:  
   What format the output was specified to be in `enum Format` \. If there was no output specified, guess before from the output filename\.




 ### <a name = "fn:CdocGetInput" id = "fixme fn:CdocGetInput">CdocGetInput</a> ###

`const char *`**`CdocGetInput`**`(void)`

 - Return:  
   The input filename\.




 ### <a name = "fn:CdocGetOutput" id = "fixme fn:CdocGetOutput">CdocGetOutput</a> ###

`const char *`**`CdocGetOutput`**`(void)`

 - Return:  
   The output filename\.






 ## <a name = "license-">License</a> ##

2019 Neil Edelman, distributed under the terms of the [MIT License](https://opensource.org/licenses/MIT)\.



