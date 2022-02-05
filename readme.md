# cdoc\.re\.c #

 * [Description](#user-content-preamble)
 * [Function Summary](#user-content-summary)
 * [Function Definitions](#user-content-fn)
 * [License](#user-content-license)

## <a id = "user-content-preamble" name = "user-content-preamble">Description</a> ##

Static documentation generator for one `C` translation unit\. This does not do any parsing, only lexical analysis, thus can be confused by remapping and complex values\. It's fairly simple, and does not support K&R style function definitions nor trigraphs\. It assumes that capital latin letters underscores are concatenation commands for the pre\-processor, such that `A_BC_(foo,bar)` is transformed into `<A>foo<BC>bar`\.

In keeping with `Javadoc` and `Doxygen`, documentation commands are `/` `**…` \(together\) and are ended with `*…/`, but not `/` `*…*` `/`\. Accounts for asterisks at the start of a line, \(Kernel comments,\) or asterisks on both sides, \(ASCII art?\) Documentation appearing at most two lines above `typedef`, `tag` \(`struct`, `enum`, `union`,\) data \(will not be output, fixme\), and functions, is associated therewith; everything else is automatically inserted into the description\. Multiple documentation on the same command is appended\. Two hard returns is a paragraph\. The big difference is we've fixed the `@param` notation; now it has to be followed by a braced list, `@param[a, b] Concatenates both.`\.

Supports some `Markdown` commands included in the documentation,

 * `\` escapes `_~!@<>[]` and "\`"; "\`" can not be represented in math/code, \(multiple escapes aren't supported\); in paragraph mode, except in ambiguous cases, the only ones that are needed are \\\` and \\\_;
 * \_emphasised\_: _emphasised_;
 * \`code/math\`: `code/math`;
 * start lists with `\*` \(including spaces\) and end with a new paragraph; these are simple, can be anywhere and don't nest;
 * `\"` \(and optionally a space\) causes all the line after to be pre\-formatted;
 * Escapes included for convenience: `\,` "&#8239;" non\-breaking thin space, `\O` "&#927;" Bachmann–Landau notation, \(but really capital omicron because not many fonts have a shape for code\-point 120030,\) `\Theta` "&#920;", `\Omega` "&#937;", `\times` "&#215;", `\cdot` "&#183;"\.
 * `~` "&nbsp;" non\-breaking space;
 * `<url>`: relative URIs must have a slash or a dot to distinguish it from text;
 * `<Source, 1999, pp. 1-2>`: citation, searches on Google scholar;
 * `<fn:<function>>`: function reference;
 * `<tag:<tag>>`: struct, union, or enum \(tag\) reference;
 * `<typedef:<typedef>>`: typedef reference;
 * `<data:<identifier>>`: data reference;
 * `[The link text](url)`: link, Markdown doesn't like `{}[]()` in text;
 * `![Caption text](url.image)`: image, same;
 * a local include directive has a documentation comment immediately after that reads only `\include`, it will also be included in the documentation\.

Each\-block\-tags separate the documentation until the next paragraph or until the next each\-block\-tag, and specify a specific documentation structure\. Each\-block\-tags that overlap are concatenated in the file order\. Not all of these are applicable for all segments of text\. These are:

 * `@subtitle`: only makes sense for preamble, \(it doesn't matter what case one writes it, but multiple are concatenated using semicolons\);
 * `@param[<param1>[, ...]]`: parameters, \(multiple are concatenated using spaces, so this really should be sentence case\);
 * `@author` \(commas\);
 * `@std`: standard, eg, `@std GNU-C99`, \(semicolons\);
 * `@depend`: dependancy, \(semicolons\);
 * `@fixme`: something doesn't work as expected, \(spaces\);
 * `@return`: normal function return, \(spaces\);
 * `@throws[<exception1>[, ...]]`: exceptional function return; `C` doesn't have native exceptions, so `@throws` means whatever one desires; perhaps a null pointer or false is returned and `errno` is set to `exception1`;
 * `@implements`: `C` doesn't have the concept of implements, but we would say that a function having a prototype of `(int (*)(const void *, const void *))` implements `bsearch` and `qsort`;
 * `@order`: comments about the run\-time or space;
 * and `@allow`, the latter being to allow `static` functions or data in the documentation, which are usually culled\.

If one sets `md` as output, it goes to `GitHub` Markdown that is specifically visible on the `GitHub` page, \(including working anchor links on browsers > 2000\.\) This is not the Markdown supported in the documentation\.



 * Standard:  
   C89
 * Dependancies:  
   [re2c](http://re2c.org/)
 * Caveat:  
   Hide `const` on params when it can not affect function calls\. Documentation on functions should be added to documentation on prototypes with the same \(similar\) prototype\. Links to non\-documented code which sometimes doesn't show up, work without error, and create broken links\. 80\-characters _per_ line limit, [https://xxyxyz\.org/line\-breaking/](https://xxyxyz.org/line-breaking/), \(needs buffering\.\) fixme with no args disappears; we should NOT check if the string is empty for these values\.


## <a id = "user-content-summary" name = "user-content-summary">Function Summary</a> ##

<table>

<tr><th>Modifiers</th><th>Function Name</th><th>Argument List</th></tr>

<tr><td align = right>enum debug</td><td><a href = "#user-content-fn-c6df3e1b">cdoc_get_debug</a></td><td></td></tr>

<tr><td align = right>enum format</td><td><a href = "#user-content-fn-37ca931f">cdoc_get_format</a></td><td></td></tr>

<tr><td align = right>const char *</td><td><a href = "#user-content-fn-7655d1a0">cdoc_get_input</a></td><td></td></tr>

<tr><td align = right>const char *</td><td><a href = "#user-content-fn-93e48c81">cdoc_get_output</a></td><td></td></tr>

</table>



## <a id = "user-content-fn" name = "user-content-fn">Function Definitions</a> ##

### <a id = "user-content-fn-c6df3e1b" name = "user-content-fn-c6df3e1b">cdoc_get_debug</a> ###

<code>enum debug <strong>cdoc_get_debug</strong>(void)</code>

 * Return:  
   Whether the command\-line was set\.




### <a id = "user-content-fn-37ca931f" name = "user-content-fn-37ca931f">cdoc_get_format</a> ###

<code>enum format <strong>cdoc_get_format</strong>(void)</code>

 * Return:  
   What format the output was specified to be in `enum format`\. If there was no output format specified, guess\.




### <a id = "user-content-fn-7655d1a0" name = "user-content-fn-7655d1a0">cdoc_get_input</a> ###

<code>const char *<strong>cdoc_get_input</strong>(void)</code>

 * Return:  
   The input filename\.




### <a id = "user-content-fn-93e48c81" name = "user-content-fn-93e48c81">cdoc_get_output</a> ###

<code>const char *<strong>cdoc_get_output</strong>(void)</code>

 * Return:  
   The output filename\.






## <a id = "user-content-license" name = "user-content-license">License</a> ##

2019 Neil Edelman, distributed under the terms of the [MIT License](https://opensource.org/licenses/MIT)\.



