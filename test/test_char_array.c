/** @license 2024 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).
 @std C89 */

#include "../src/char_array.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

int main(void) {
	int success = 0;
	struct char_array s = char_array();
	const char *string;
	unsigned i;
	const char *const tests[] = {
		"",
		"?",
		"Foo.",
		"aaaab—",
		"aaaabbbb",
		"aaaabb—",
		"a b c"
	}, *const requires[] = {
		"(00)",
		"(?, 00)",
		"(F, o, o, ., 00)",
		"(a, a, a, a, b, E2, 80, 94, 00)",
		"(a, a, a, a, b, b, b, b, 00)",
		"(a, a, a, a, b, b, E2, 80, 94, 00)",
		"(a, ·, b, ·, c, 00)"
	}, *test, *require;

	printf("Start:\n");

	require = "(?, 00)";
	if(!char_array_reserve(&s, 2)) goto catch;
	s.data[0] = '?', s.data[1] = '\0', s.size = 2;
	string = char_array_to_string(&s);
	printf("Should be %s: %s.\n", require, string);
	assert(!strcmp(require, string));

	for(i = 0; i < sizeof tests / sizeof *tests; i++) {
		test = tests[i], require = requires[i];
		if(!char_array_copy(&s, test)) goto catch;
		string = char_array_to_string(&s);
		printf("Should be %s: %s.\n", require, string);
		assert(!strcmp(require, string));
	}

	success = 1;
	goto finally;
catch:
	perror("char_array");
finally:
	char_array_(&s);
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
