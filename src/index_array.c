#include <stdio.h>

static void index_to_string(const size_t *i, char (*const a)[12]) {
	sprintf(*a, "%lu", *(const unsigned long *)i % 100000000000lu);
}
/* This is the other one…
static void index_to_string(const size_t *const n, char (*const a)[12]) {
	sprintf(*a, "%lu", (unsigned long)*n % 1000000000lu);
}*/
#define DEFINE
#include "index_array.h"
