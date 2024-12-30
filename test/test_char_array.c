/** @license 20xx Neil Edelman, distributed under the terms of the
 [GNU General Public License 3](https://opensource.org/licenses/GPL-3.0).
 @license 20xx Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 This is a standard C file.

 @std C89 */

#include "../src/char_array.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

int main(void) {
	struct char_array arr = char_array();
	char *a;
	unsigned i;
	printf("Foo!!!\n");
	for(i = 0; i < 10; i++) {
		a = char_array_new(&arr);
		if(!a) assert(0);
		*a = (char)(rand() / (RAND_MAX / 254 + 1) + 1);
	}
	a = char_array_new(&arr);
	if(!a) assert(0);
	*a = '\0';
	printf("Array of chars: %s.\n", char_array_to_string(&arr));
	char_array_(&arr);
	return EXIT_SUCCESS;
}
