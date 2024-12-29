#include "segment_array.h"
#include "division.h"
#include "symbol.h"
#include "scanner.h"

struct report {
	struct segment_array segments;
	struct token_array brief;
};

const char *tokens_first_label(const struct token_array *const tokens);
size_t tokens_first_line(const struct token_array *const tokens);
size_t tokens_mark_size(const struct token_array *const tokens);
void tokens_mark(const struct token_array *const tokens, char *const marks);

struct token;

int report_current_division(const enum division division);
int report_current_param(const struct token *const token);
void report_current_reset(void);

void report_(struct report *const report);
void report_division(const enum division division);
void report_last_segment_debug(const struct report *const report);
/*I don't think this should be here, static in scanner?*/
int report_notify(/*struct report *const report*/const struct scanner *const scan);
void report_cull(void);
void report_warn(void);
int report_out(void);
