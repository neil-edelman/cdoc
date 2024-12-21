#include <stdio.h>

static void index_to_string(const size_t *i, char (*const a)[12]) {
	sprintf(*a, "%lu", *(const unsigned long *)i % 100000000000u);
}
#define DEFINE
#include "index_array.h"
