#include "Division.h"

struct TokenArray;

int Semantic(const struct TokenArray *const code);
enum Division SemanticDivision(void);
void SemanticParams(size_t *const no, const size_t **const array);
