/** @license 2024 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).
 @std C89 */

#include "../src/file_buffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

int main(void) {
	int exit_code = EXIT_FAILURE;

	/* fixme: I don't know what that does? */
	buffer_clear();
	buffer_();

	exit_code = EXIT_SUCCESS;
	goto finally;
catch:
	perror("file_buffer");
finally:
	return exit_code;
}
