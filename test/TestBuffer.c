#include <stdlib.h> /* EXIT malloc free */
#include <stdio.h>  /* fprintf */
#include <string.h> /* strlen */
#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include "../src/Buffer.h"

static int do_hash(const char *const a) {
	const char *const fmt = "part-%lx";
	char *b;
	int len;
	size_t i, size = strlen(a);
	unsigned long hash = 5381;
	assert(a);
	/* h * 33 + c */
	for(i = 0; i < size; ++i) hash = ((hash << 5) + hash) + a[i];
	if((len = snprintf(0, 0, fmt, hash)) <= 0) return errno = EILSEQ, 0;
	if(!(b = BufferPrepare(len))) return 0;
	sprintf(b, fmt, hash);
	return 1;
}

int main(void) {
	if(!do_hash("foobar")) return EXIT_FAILURE;
	printf("%s\n", BufferGet());
	if(!do_hash("foobar")) return EXIT_FAILURE;
	printf("%s\n", BufferGet());
	BufferClear();
	if(!do_hash("foobar")) return EXIT_FAILURE;
	printf("%s\n", BufferGet());
	Buffer_();
	return EXIT_SUCCESS;
}
