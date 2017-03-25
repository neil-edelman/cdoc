Copyright (C) 2017 Neil Edelman.

neil dot edelman each mail dot mcgill dot ca

Version 1.1.

"sudo make install" will make "cdoc" and copy it to /usr/local/bin.
The delimiter of Cdoc comments is "/** ". An example:

cat Foo.c Foo.h | cdoc text > Foo.txt

Cdoc is a light weight, JavaDoc-style, documentation generator for
C. It is not aware of C, but searches in your file for the double
asterisk '** ' comment. It does not support ascii art (three or
more asterisks are not recognised.) It does not support writing
html in your source code, and escapes '<>&' in html mode; placing
two new lines is sufficient to start a new paragraph.

The following symbols are recognised: \url{url}, \cite{word},
\see{reference}, \${math}, and {emphasis}. Cdoc decides based on
context of the following code whether it goes in the preamble,
functions, or declarations. It uses simple, but inexact heuristic,
which may become confused. It supports macro-generics if you write
them just like void A_BI_(Create, Thing)(void); it will transform
it into, <A>Create<BI>Thing(void).

Each-expressions must come first on the line. Cdoc recognises:
@param, @author, @since, @fixme, @deprecated are accepted globally;
@title, @std, @version are accepted in the preamble; @return, @throws,
@implements, @allow are accepted before functions. The title is
set by @file. Functions that are marked static as the first modifier
are not included unless one marks them by @allow. The @param and
@throws have an optional sub-argument separated by ':' or a new
line that splits the "expression: description."

License:

The MIT License (MIT)

Copyright (c) 2017 Neil Edelman

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject
to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

The software is provided "as is," without warranty of any kind,
express or implied, including but not limited to the warranties of
merchantability, fitness for a particular purpose and noninfringement.
In no event shall the authors or copyright holders be liable for
any claim, damages or other liability, whether in an action of
contract, tort or otherwise, arising from, out of or in connection
with the software or the use or other dealings in the software.
