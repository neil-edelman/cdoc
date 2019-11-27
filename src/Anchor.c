/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 `GitHub` is funny with their named anchors.

 @std C89/90 */

#include <stdlib.h> /* EXIT malloc free */
/*#include <stdio.h>*/  /* fprintf */
/*#include <assert.h>*/ /* assert */
#include <errno.h>  /* errno */

#define ARRAY_NAME Char
#define ARRAY_TYPE char
#include "Array.h"

static struct CharArray *buffer;
