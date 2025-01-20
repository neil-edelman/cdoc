#include "segment_array.h"
#include "division.h"
#include "symbol.h"
#include "scanner.h"

struct report {
	struct segment_array segments;
	struct token_array brief;
};

struct token;

int report_current_division(const enum division division);
int report_current_param(const struct token *const token);
void report_current_reset(void);

void report_(struct report *const report);
void report_division(const enum division division);
void report_last_segment_debug(const struct report *const report);
/*I don't think this should be here, static in scanner?*/
int report_notify(/*struct report *const report*/const struct scanner *const scan);
void report_cull(struct report *const report);
void report_warn(void);
int report_out(void);
