#include <stddef.h>
#include "division.h"

struct token_array;

int semantic(const struct token_array *const code);
enum division semantic_division(void);
void semantic_params(size_t *const no, const size_t **const array);
