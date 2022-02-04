#include <stddef.h>
#include "symbol.h"

enum scanner_start { START_CODE, START_DOC };

struct scanner;

typedef int (*scanner_predicate)(const struct scanner *);

void scanner_(struct scanner **const scanner);
struct scanner *scanner(const char *const label, const char *const buffer,
	const scanner_predicate notify, const enum scanner_start start);
enum symbol scanner_symbol(const struct scanner *const scan);
const char *scanner_from(const struct scanner *const scan);
const char *scanner_to(const struct scanner *const scan);
const char *scanner_label(const struct scanner *const scan);
size_t scanner_line(const struct scanner *const scan);
int scanner_indent_level(const struct scanner *const scan);
