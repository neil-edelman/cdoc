#include "attribute.h"
#include <stdio.h>

static void attribute_to_string(const struct attribute *const att,
	char (*const a)[12]) {
	strncpy(*a, symbols[att->token.symbol], sizeof *a - 1);
	(*a)[sizeof *a - 1] = '\0';
}

#define DEFINE
#include "attribute_array.h"

void attributes_deep_erase(struct attribute_array *const atts) {
	struct attribute *a;
	if(!atts) return;
	while((a = attribute_array_pop(atts)))
		token_array_(&a->header), token_array_(&a->contents);
	attribute_array_(atts);
}
