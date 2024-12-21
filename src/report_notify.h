#include "division.h"
#include "symbol.h"
#include "scanner.h"

struct token_array;

const char *tokens_first_label(const struct token_array *const tokens);
size_t tokens_first_line(const struct token_array *const tokens);
size_t tokens_mark_size(const struct token_array *const tokens);
void tokens_mark(const struct token_array *const tokens, char *const marks);

struct token;

int report_current_division(const enum division division);
int report_current_param(const struct token *const token);
void report_current_reset(void);

void report_(void);
void report_division(const enum division division);
void report_last_segment_debug(void);
int report_notify(const struct scanner *const scan);
void report_cull(void);
void report_warn(void);
int report_out(void);
