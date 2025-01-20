#include "../src/image_dimension.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

int main(void) {
	int exit_code = EXIT_FAILURE, ret;
	struct { unsigned x, y; } dim;
	const char *fn = "";
	errno = 0;

	fn = "dne";
	ret = image_dimension(fn, &dim.x, &dim.y);
	assert(!ret && errno);
	printf("(Expected.)\n");
	perror(fn);
	errno = 0;

	fn = "diagrams/Ellen_Ripley_badass.png";
	if(!(ret = image_dimension(fn, &dim.x, &dim.y))) goto catch;
	assert(dim.x == 150 && dim.y == 190);

	fn = "diagrams/Inigo_Montoya.jpeg";
	if(!(ret = image_dimension(fn, &dim.x, &dim.y))) goto catch;
	assert(dim.x == 150 && dim.y == 100);

	exit_code = EXIT_SUCCESS;
	goto finally;
catch:
	perror(fn);
	assert(0);
finally:
	return exit_code;
}
