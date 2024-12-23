#include "attribute.h"
#include <stdio.h>

static void attribute_to_string(const struct attribute *const att,
	char (*const a)[12]) {
	strncpy(*a, symbols[att->token.symbol], sizeof *a - 1);
	(*a)[sizeof *a - 1] = '\0';
}

#define DEFINE
#include "attribute_array.h"
