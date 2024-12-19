#ifndef FORMAT_H
#	define FORMAT_H

/* `punctuates` in `style.c` is dependant on this order and size. */

#	define FORMAT X(OUT_RAW), X(OUT_HTML), X(OUT_MD)

#	define X(n) n
enum format { FORMAT };
#	undef X
#	define X(n) #n
static const char *const format_strings[] = { FORMAT };
#	undef X

#endif
