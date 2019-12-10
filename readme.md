 # Cdoc\.c\.re #

 * [Description](#user-content-preamble)
 * [Function Summary](#user-content-summary)
 * [Function Definitions](#user-content-fn)
 * [License](#user-content-license)

 ## <a id = "user-content-preamble" name = "user-content-preamble">Description</a> ##

A context\-sensitive parser intended to process parts of a `RAW?C` compilation unit and extract documentation, as well as outputting that documentation into the format specified\. Designed to be very strict, warning one of documentation errors; and simple, made for self\-contained independent documenatation\. This does not do any compiling, just text\-parsing\. Thus, one can easily confuse by redefining symbols\. However, it assumes the macro `RAW?A_B_(Foo,Bar)` is transformed into `RAW?<A>Foo<B>Bar` \.

Documentation commands are `RAW?/` `RAW?*RAW?*RAW?…` \(together\) and are ended with `RAW?*RAW?…/` , but not `RAW?/` `RAW?*RAW?…RAW?*` `RAW?/` , \(common code break\.\) Asterisks at the start of a line, like Kernel comments, or asterisks all over like some crazy ASCII art, are supported\. Documentation appearing at most two lines above `RAW?typedef` , `RAW?tag` \(`RAW?struct` , `RAW?enum` , `RAW?union` ,\) data, and functions, is associated therewith; everything else is automatically inserted into the description\. Multiple documentation on the same command is appended\. Two hard returns is a paragraph\. Supports some `RAW?Markdown` commands included in the documentation,

 * `RAW?\` escapes `RAW?_RAW?~RAW?!RAW?@RAW?<>[]` and "\`"; "\`" can not be represented in math/code, \(multiple escapes aren't supported\); in paragraph mode, except in ambiguous cases, the only ones that are needed are \\\` and \\\_;
 * \_emphasised\_: _emphasised_ ;
 * \`code/math\`: `RAW?code/math` ;
 * start lists with `RAW?\RAW?*` \(including spaces\) and end with a new paragraph; these are simple, can be anywhere and don't nest;
 * `RAW?\RAW?"` \(and optionally a space\) causes all the line after to be pre\-formatted;
 * Escapes included for convenience: `RAW?\RAW?,` "&#8239;" non\-breaking thin space, `RAW?\RAW?O` "&#927;" Bachmann–Landau notation, \(but really capital omicron because not many fonts have a shape for code\-point 120030,\) `RAW?\RAW?Theta` "&#920;", `RAW?\RAW?Omega` "&#937;", `RAW?\RAW?times` "&#215;", `RAW?\RAW?cdot` "&#183;"\.
 * `RAW?~` "&nbsp;" non\-breaking space;
 * `RAW?<RAW?urlRAW?>` : relative URIs must have a slash or a dot to distinguish it from text;
 * `RAW?<RAW?Source, RAW?1999, RAW?pp. RAW?1-2RAW?>` : citation;
 * `RAW?<RAW?fn:RAW?<RAW?functionRAW?>RAW?>` : function reference;
 * `RAW?<RAW?tag:RAW?<RAW?tagRAW?>RAW?>` : struct, union, or enum \(tag\) reference;
 * `RAW?<RAW?typedef:RAW?<RAW?typedefRAW?>RAW?>` : typedef reference;
 * `RAW?<RAW?data:RAW?<RAW?identifierRAW?>RAW?>` : data reference;
 * `RAW?[RAW?The RAW?link RAW?textRAW?]RAW?(url)` : link;
 * `RAW?!RAW?[RAW?Caption RAW?textRAW?]RAW?(url.image)` : image;
 * a local include directive has a documentation comment immediately after that reads only `RAW?\RAW?include` , it will also be included in the documentation\.

Each\-block\-tags separate the documentation until the next paragraph or until the next each\-block\-tag, and specify a specific documentation structure\. Each\-block\-tags that overlap are concatenated in the file order\. Not all of these are applicable for all segments of text\. These are:

 * `RAW?@RAW?subtitle` : only makes sense for preamble, \(it doesn't matter what case one writes it, but multiple are concatenated using semicolons\);
 * `RAW?@RAW?param[<param1>[, RAW?...]]` : parameters, \(multiple are concatenated using spaces, so this really should be sentence case\);
 * `RAW?@RAW?author` \(commas\);
 * `RAW?@RAW?std` : standard, eg, `RAW?@RAW?std RAW?GNU-C99` , \(semicolons\);
 * `RAW?@RAW?depend` : dependancy, \(semicolons\);
 * `RAW?@RAW?fixme` : something doesn't work as expected, \(spaces\);
 * `RAW?@RAW?return` : normal function return, \(spaces\);
 * `RAW?@RAW?throws[<exception1>[, RAW?...]]` : exceptional function return; `RAW?C` doesn't have native exceptions, so `RAW?@throws` means whatever one desires; perhaps a null pointer or false is returned and `RAW?errno` is set to `RAW?exception1` , \(spaces\);
 * `RAW?@RAW?implements` : `RAW?C` doesn't have the concept of implements, but we would say that a function having a prototype of `RAW?(int RAW?(RAW?*RAW?)(const RAW?void RAW?*RAW?, RAW?const RAW?void RAW?*RAW?))` implements `RAW?bsearch` and `RAW?qsort` , \(commas\);
 * `RAW?@RAW?order` : comments about the run\-time or space, \(spaces\);
 * and `RAW?@RAW?allow` , the latter being to allow `RAW?static` functions or data in the documentation, which are usually culled; one will be warned if this has any text\.

Perhaps the most striking difference from `RAW?Javadoc` and `RAW?Doxygen` is the `RAW?@RAW?param` has to be followed by a braced list, \(it's confusing to have the variable be indistinguishable from the text\.\)

If one sets `RAW?md` as output, it goes to `RAW?GitHub` Markdown that is specifically visible on the `RAW?GitHub` page, \(including working anchor links on browsers > 2000\.\) It bears little to Markdown supported in the documentation\.



 * Standard:  
   C89
 * Dependancies:  
   [re2c](http://re2c.org/)
 * Caveat:  
   Old\-style function support\. Trigraph support, \(haha\.\) Hide `RAW?const` on params when it can not affect function calls\. Documentation on functions should be added to documentation on prototypes with the same \(similar\) prototype\. Links to non\-documented code which sometimes doesn't show up, work without error, and create broken links\. 80\-characters _per_ line limit, [https://xxyxyz\.org/line\-breaking/](https://xxyxyz.org/line-breaking/), \(needs buffering\.\) Eg, fixme with no args disappears; we should NOT check if the string is empty\. If a segment has multiple licenses, they will show multiple times\. `RAW?foo.c:221` : space where it shouldn't be\. Why? @throws relax constriants on having something there\. Markdown doesn't allow any brackets within links/images but html does; provide a separate escape table? \(Not likely; no brackets\. Fixed\.\)


 ## <a id = "user-content-summary" name = "user-content-summary">Function Summary</a> ##

<table>

<tr><th>Modifiers</th><th>Function Name</th><th>Argument List</th></tr>

<tr><td align = right>int</td><td><a href = "#user-content-fn-155d6ff">CdocGetDebug</a></td><td></td></tr>

<tr><td align = right>enum Format</td><td><a href = "#user-content-fn-334aa1ab">CdocGetFormat</a></td><td></td></tr>

<tr><td align = right>const char *</td><td><a href = "#user-content-fn-7ee5d21c">CdocGetInput</a></td><td></td></tr>

<tr><td align = right>const char *</td><td><a href = "#user-content-fn-18fcd065">CdocGetOutput</a></td><td></td></tr>

</table>



 ## <a id = "user-content-fn" name = "user-content-fn">Function Definitions</a> ##

 ### <a id = "user-content-fn-155d6ff" name = "user-content-fn-155d6ff">CdocGetDebug</a> ###

<code>int <strong>CdocGetDebug</strong>(void)</code>

 - Return:  
   Whether the command\-line option to spam on `RAW?stderr` was set\.




 ### <a id = "user-content-fn-334aa1ab" name = "user-content-fn-334aa1ab">CdocGetFormat</a> ###

<code>enum Format <strong>CdocGetFormat</strong>(void)</code>

 - Return:  
   What format the output was specified to be in `RAW?enum RAW?Format` \. If there was no output specified, guess before from the output filename\.




 ### <a id = "user-content-fn-7ee5d21c" name = "user-content-fn-7ee5d21c">CdocGetInput</a> ###

<code>const char *<strong>CdocGetInput</strong>(void)</code>

 - Return:  
   The input filename\.




 ### <a id = "user-content-fn-18fcd065" name = "user-content-fn-18fcd065">CdocGetOutput</a> ###

<code>const char *<strong>CdocGetOutput</strong>(void)</code>

 - Return:  
   The output filename\.






 ## <a id = "user-content-license" name = "user-content-license">License</a> ##

2019 Neil Edelman, distributed under the terms of the [MIT License](https://opensource.org/licenses/MIT) \.



