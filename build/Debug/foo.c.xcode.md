 # foo\.c #

 ## The Subtitle <>&\*\{\}\[\]\(\)\#\+\-\.\!`RAW?_` &lt;&gt;&amp; <>\\&\\\*\\\{\\\}\[\]\\\(\\\)\\\#\\\+\\\-\\\.\!\`\_; Link [http://127\.0\.0\.1/](http://127.0.0.1/) ##

 * [Description](#user-content-preamble)
 * [Typedef Aliases](#user-content-typedef):  [&lt;PT&gt;Type](#user-content-typedef-8b318acb), [&lt;A&gt;Foo&lt;B&gt;Bar](#user-content-typedef-3bb96b9), [&lt;PT&gt;ToString](#user-content-typedef-c92c3b0f), [&lt;PT&gt;Action](#user-content-typedef-33725a81), [&lt;PT&gt;Predicate](#user-content-typedef-d7c73930)
 * [Struct, Union, and Enum Definitions](#user-content-tag):  [Cee](#user-content-tag-68097f70), [Scanner](#user-content-tag-ace769e5), [Token](#user-content-tag-3a355bd2)
 * [General Declarations](#user-content-data):  [Cee](#user-content-data-68097f70), [y](#user-content-data-fc0c4ef4), [foo](#user-content-data-a9f37ed7), [c](#user-content-data-e60c2c52), [&lt;T&gt;foo](#user-content-data-80b9524b)
 * [Function Summary](#user-content-summary)
 * [Function Definitions](#user-content-fn)
 * [License](#user-content-license)

 ## <a id = "user-content-preamble" name = "user-content-preamble">Description</a> ##

[This is text &amp;&amp;](http://yo.c) Header\. Yo [http://foo\.org/](http://foo.org/) [MIT](https://mit.edu/)

 * Paragraph @ @a &#8239;&nbsp;&nbsp;
 * Links to entities: [&lt;T&gt;fun4](#user-content-fn-ebe4b0f2), [y](#user-content-data-fc0c4ef4)\.
 * Broken: [not_fn](#user-content-fn-9ab2437f), [&lt;T&gt;not_fn](#user-content-fn-afc45f23)\.
 * Citation [ÀæĔge2007](https://scholar.google.ca/scholar?q=%C3%80%C3%A6%C4%94ge2007)\.
 * [absolute](http://arr.com/index.html#ddddd<u>sfdg) \.
 * [relative](../../samples/foo.h) [relative](../../src/Cdoc.c.re) \.
 * png: ![Ellen Ripley](../../diagrams/Ellen_Ripley_badass.png) \.
 * jpeg: ![Inigo Montoya](../../diagrams/Inigo_Montoya.jpeg) \.

Escapes\. \\ \` @ \_ \` This is not prefomated\.

This is preformated &lt;http://foo.com/&gt;*

<pre>
 
</pre>

pre***

para: <>&\*\{\}\[\]\(\)\#\+\-\.\!`RAW?_` &lt;&gt;&amp; <>\\&\\\*\\\{\\\}\[\]\\\(\\\)\\\#\\\+\\\-\\\.\!\`\_

code: `RAW?<>&RAW?*RAW?{}[]()#+-.RAW?!RAW?_ RAW?&lt;&gt;&amp; RAW?<RAW?>RAW?\RAW?&RAW?\RAW?*RAW?\RAW?{RAW?\RAW?}RAW?[RAW?]RAW?\RAW?(RAW?\RAW?)RAW?\RAW?#RAW?\RAW?+RAW?\RAW?-RAW?\RAW?.RAW?!RAW?_` \(cannot \`?\)

em: _<>&\*\{\}\[\]\(\)\#\+\-\.\!\`&lt;&gt;&amp;<>\\&\\\*\\\{\\\}\[\]\\\(\\\)\\\#\\\+\\\-\\\.\!\`_ \(cannot \_?\)

pre: &lt;&gt;&amp;\*{}[]()#+-.!`_` &amp;lt;&amp;gt;&amp;amp; \&lt;\&gt;\&amp;\\\*\{\}\[\]\(\)\#\+\-\.\!\`\_

 * list: <>&\*\{\}\[\]\(\)\#\+\-\.\!`RAW?_` &lt;&gt;&amp; <>\\&\\\*\\\{\\\}\[\]\\\(\\\)\\\#\\\+\\\-\\\.\!\`\_

image: \*\{\}\[\]\(\)\#\+\-\.\!`RAW?_` &lt;&gt;&amp; <>\\&\\\*\\\{\\\}\[\]\\\(\\\)\\\#\\\+\\\-\\\.\!\`\_ ![&lt;&gt;&amp;*+-.!](../../diagrams/Ellen_Ripley_badass.png) \.

link: [&lt;&gt;&amp;*#+-.!\(](../../samples/foo.c) \.

Headings accept html entities; accept <>&; accept backslash escapes; normal code\-em; but if enclosed in html, transfer to html\.

Para accepts entities; accept <>&; accept backslash

Code does not accept entities; escapes all

Em accepts entities; accept <>&; accepts backslash

Image/link accepts html entities but doesn't accept \(\)\[\]\{\}, but does \\\(\\\)\.

Pre is not working\.



Header\. That also goes in the header\.

This is a kernel\-style comment\. [http://www\.\`@\.com/index\.html](http://www.`@.com/index.html) [Yo2019](https://scholar.google.ca/scholar?q=Yo2019) ?<>&\! [fun1](#user-content-fn-407e96bd)

Header\. This is an ascii art comment\. Eww\.

There should be [fun1](#user-content-fn-407e96bd) to [fun7](#user-content-fn-3a7e8d4b) and [main](#user-content-fn-ea90e208)\.

 * Author:  
   Somebody, Nobody, (fixme: Cee: , fixme: fun2\_scan\_eof:  External Author, fixme: fun3\_scan\_comment:  Prof\. Snape, fixme: fun6:  Nobody, fixme: fun6:  Sombody)
 * Standard:  
   (fixme: fun6:  C, fixme: fun6:  Java)
 * Dependancies:  
   C89, (fixme: fun6:  Meh, fixme: fun6:  Duh)
 * Caveat:  
    (fixme: Cee, fixme: fun2\_scan\_eof, fixme: fun6, fixme: fun6)


 ## <a id = "user-content-typedef" name = "user-content-typedef">Typedef Aliases</a> ##

 ### <a id = "user-content-typedef-8b318acb" name = "user-content-typedef-8b318acb">&lt;PT&gt;Type</a> ###

<code>typedef int <strong>&lt;PT&gt;Type</strong>;</code>

Troubles with this line? check to ensure that `RAW?ARRAY_TYPE` is a valid type, whose definition is placed above `RAW?#include RAW?"Array.h"` \.



 ### <a id = "user-content-typedef-3bb96b9" name = "user-content-typedef-3bb96b9">&lt;A&gt;Foo&lt;B&gt;Bar</a> ###

<code>typedef int(*<strong>&lt;A&gt;Foo&lt;B&gt;Bar</strong>)(int);</code>

Typedef\.



 ### <a id = "user-content-typedef-c92c3b0f" name = "user-content-typedef-c92c3b0f">&lt;PT&gt;ToString</a> ###

<code>typedef void(*<strong>&lt;PT&gt;ToString</strong>)(const T *, char(*const)[12]);</code>

Typedef\.



 ### <a id = "user-content-typedef-33725a81" name = "user-content-typedef-33725a81">&lt;PT&gt;Action</a> ###

<code>typedef void(*<strong>&lt;PT&gt;Action</strong>)(T *const data);</code>

Typedef\. Operates by side\-effects on `RAW?data` only\.



 ### <a id = "user-content-typedef-d7c73930" name = "user-content-typedef-d7c73930">&lt;PT&gt;Predicate</a> ###

<code>typedef int(*<strong>&lt;PT&gt;Predicate</strong>)(const T *const data);</code>

Typedef\. Given constant `RAW?data` , returns a boolean\.



 ## <a id = "user-content-tag" name = "user-content-tag">Struct, Union, and Enum Definitions</a> ##

 ### <a id = "user-content-tag-68097f70" name = "user-content-tag-68097f70">Cee</a> ###

<code>struct <strong>Cee</strong>;</code>

In the header\.

 - Parameter: ALLYOURBASE  
   Also in the header\.




 ### <a id = "user-content-tag-ace769e5" name = "user-content-tag-ace769e5">Scanner</a> ###

<code>struct <strong>Scanner</strong>;</code>

Tag



 ### <a id = "user-content-tag-3a355bd2" name = "user-content-tag-3a355bd2">Token</a> ###

<code>enum <strong>Token</strong> { END };</code>

Tag



 ## <a id = "user-content-data" name = "user-content-data">General Declarations</a> ##

 ### <a id = "user-content-data-68097f70" name = "user-content-data-68097f70">Cee</a> ###

<code>struct Cee *<strong>Cee</strong>();</code>

Prototype\!



 ### <a id = "user-content-data-fc0c4ef4" name = "user-content-data-fc0c4ef4">y</a> ###

<code>char(*(*<strong>y</strong>[3])())[5];</code>

declare x as array 3 of pointer to function returning pointer to array 5 of char



 ### <a id = "user-content-data-a9f37ed7" name = "user-content-data-a9f37ed7">foo</a> ###

<code>int(*(*<strong>foo</strong>)(void))[3];</code>

declare foo as pointer to function \(void\) returning pointer to array 3 of int



 ### <a id = "user-content-data-e60c2c52" name = "user-content-data-e60c2c52">c</a> ###

<code>int(*<strong>c</strong>);</code>

Declare a pointer\-to\-int?



 ### <a id = "user-content-data-80b9524b" name = "user-content-data-80b9524b">&lt;T&gt;foo</a> ###

<code>int <strong>&lt;T&gt;foo</strong>;</code>

Test parameter\.



 ## <a id = "user-content-summary" name = "user-content-summary">Function Summary</a> ##

<table>

<tr><th>Modifiers</th><th>Function Name</th><th>Argument List</th></tr>

<tr><td align = right>void</td><td><a href = "#user-content-fn-f5f336fd">Cee_</a></td><td>ceeptr</td></tr>

<tr><td align = right>char *</td><td><a href = "#user-content-fn-b55497dd">CeeGetVar</a></td><td>cee</td></tr>

<tr><td align = right>int</td><td><a href = "#user-content-fn-3f7e952a">fun0</a></td><td>arg1_2, arg2_2</td></tr>

<tr><td align = right>int <em>function</em></td><td><a href = "#user-content-fn-407e96bd">fun1</a></td><td>arg1_1</td></tr>

<tr><td align = right>static enum Token</td><td><a href = "#user-content-fn-5ce16895">fun2_scan_eof</a></td><td>arg1_2, arg2_2</td></tr>

<tr><td align = right>static enum Token</td><td><a href = "#user-content-fn-48c98dc7">fun3_scan_comment</a></td><td>arg1_1</td></tr>

<tr><td align = right>void</td><td><a href = "#user-content-fn-ebe4b0f2">&lt;T&gt;fun4</a></td><td></td></tr>

<tr><td align = right>T</td><td><a href = "#user-content-fn-693cf86a">fun5_</a></td><td>arg1_2, arg2_3, arg3_3</td></tr>

<tr><td align = right>int</td><td><a href = "#user-content-fn-397e8bb8">fun6</a></td><td>oy, vey</td></tr>

<tr><td align = right>void</td><td><a href = "#user-content-fn-3a7e8d4b">fun7</a></td><td>a, b, c</td></tr>

<tr><td align = right>int</td><td><a href = "#user-content-fn-ea90e208">main</a></td><td></td></tr>

</table>



 ## <a id = "user-content-fn" name = "user-content-fn">Function Definitions</a> ##

 ### <a id = "user-content-fn-f5f336fd" name = "user-content-fn-f5f336fd">Cee_</a> ###

<code>void <strong>Cee_</strong>(struct Cee **<em>ceeptr</em>);</code>



 ### <a id = "user-content-fn-b55497dd" name = "user-content-fn-b55497dd">CeeGetVar</a> ###

<code>char *<strong>CeeGetVar</strong>(const struct Cee *<em>cee</em>);</code>



 ### <a id = "user-content-fn-3f7e952a" name = "user-content-fn-3f7e952a">fun0</a> ###

<code>int <strong>fun0</strong>(int(*<em>arg1_2</em>)(int, int(*not)(int)), int <em>arg2_2</em>)</code>



 ### <a id = "user-content-fn-407e96bd" name = "user-content-fn-407e96bd">fun1</a> ###

<code>int(*(*<strong>fun1</strong>(const int <em>arg1_1</em>))(int))(int(*)(int))</code>

Function declare x as function \(int\) returning pointer to function \(int\) returning pointer to function \(pointer to function \(int\) returning int\) returning int

 - Parameter: _arg1\_1_  
   Yes\.




 ### <a id = "user-content-fn-5ce16895" name = "user-content-fn-5ce16895">fun2_scan_eof</a> ###

<code>static enum Token <strong>fun2_scan_eof</strong>(struct Scanner *const <em>arg1_2</em>, int <em>arg2_2</em>)</code>

Returns eof\.

 - Parameter: _arg1\_2_  
   Arg 1/2\.
 - Parameter: _arg2\_2_  
   Arg 2/2\.
 - Return:  
   END\.
 - Implements:  
   ScannerFn
 - Author:  
   External Author
 - License:  
   External license\!




 ### <a id = "user-content-fn-48c98dc7" name = "user-content-fn-48c98dc7">fun3_scan_comment</a> ###

<code>static enum Token <strong>fun3_scan_comment</strong>(struct Scanner *const <em>arg1_1</em>)</code>

Function\.

 - Implements:  
   ScannerFn
 - Author:  
   Prof\. Snape
 - License:  
   Other license\!\!




 ### <a id = "user-content-fn-ebe4b0f2" name = "user-content-fn-ebe4b0f2">&lt;T&gt;fun4</a> ###

<code>void <strong>&lt;T&gt;fun4</strong>(void)</code>

This is a foo\.

 - Order:  
   &#937; &#920; &#927; &#215; &#183; O




 ### <a id = "user-content-fn-693cf86a" name = "user-content-fn-693cf86a">fun5_</a> ###

<code>T <strong>fun5_</strong>(int(*<em>arg1_2</em>)(int arg1_3, int(*fn)(void)), Foo <em>arg2_3</em>, struct &lt;T&gt;Array <em>arg3_3</em>)</code>

Function `RAW?arg1_2` \. yo

 - Return:  
   Function of `RAW?arg2_2` \.




 ### <a id = "user-content-fn-397e8bb8" name = "user-content-fn-397e8bb8">fun6</a> ###

<code>int <strong>fun6</strong>(<em>oy</em>, <em>vey</em>)</code>

Oyvey lipsum\.

O\!\!\!\!

<> <foo> [i\.](i.) [i\.o](i.o) [i/o](i/o)



 - Parameter: _oy_  
   Nothing\. Maybe something\.
 - Parameter: _vey_  
   Nothing\.
 - Return:  
   No\. Way\.
 - Implements:  
   Foo, Bar
 - Exceptional return: AUGH, OY  
   Bad\.
 - Exceptional return: RUN, AWAY  
   Ohno\.
 - Author:  
   Nobody, Sombody
 - Standard:  
   C; Java
 - Dependancies:  
   Meh; Duh
 - Caveat:  
   Ohoh\. Ohno\.
 - License:  
   No\. Way\.




 ### <a id = "user-content-fn-3a7e8d4b" name = "user-content-fn-3a7e8d4b">fun7</a> ###

<code>void <strong>fun7</strong>(char(*const <em>a</em>)[12], int(*<em>b</em>)(int a, int b), int(((<em>c</em>))))</code>

Does nothing to `RAW?a` and `RAW?b` \.



 ### <a id = "user-content-fn-ea90e208" name = "user-content-fn-ea90e208">main</a> ###

<code>int <strong>main</strong>(void)</code>

Does main stuff\.



 ## <a id = "user-content-license" name = "user-content-license">License</a> ##

Lol I don't need a license\.

No\.

(See license details fixme: fun2\_scan\_eof, fixme: fun3\_scan\_comment, fixme: fun6, fixme: fun6.)



