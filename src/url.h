#include <stddef.h>

int url(const char *const in_fn, const char *const out_fn);
void url_(void);
size_t url_strip_query_fragment(const char *const uri, const size_t uri_len);
const char *url_from_here(const size_t fn_len, const char *const fn);
const char *url_from_output(const size_t fn_len, const char *const fn);
int url_is_fragment(const char *const str);

/* POSIX and URI -- may need to change if necessary. `url_dirsep` is included
 in the header. */
#define DIRSEPCHAR /
#define FRAGMENTCHAR #
#define SUBORDINATECHARS ?#
#define TWODOTS ..
#define DOT .

/* XSTR(#DIRSEP ## #OTHER_CHARS), crashes `clang`, hmm. */

#define XSTR(s) STR(s)
#define STR(s) #s
static char
	*const url_dirsep = XSTR(DIRSEPCHAR),
	*const url_fragment = XSTR(FRAGMENTCHAR),
	*const url_subordinate = XSTR(SUBORDINATECHARS),
	*const url_searchdirsep = XSTR(DIRSEPCHAR) XSTR(SUBORDINATECHARS),
	*const url_relnotallowed = XSTR(DIRSEPCHAR) XSTR(DIRSEPCHAR),
	*const url_twodots = XSTR(TWODOTS),
	*const url_dot = XSTR(DOT);
#undef XSTR
#undef STR
