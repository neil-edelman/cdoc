#include <stddef.h>

void buffer_(void);
const char *buffer_get(void);
void buffer_clear(void);
char *buffer_prepare(const size_t length);
void buffer_swap(void);

struct text;

const char *text_name(const struct text *const file);
const char *text_base_name(const struct text *const file);
size_t text_size(const struct text *const file);
const char *text_get(const struct text *const file);
struct text *text_open(const char *const fn);
void text_close_all(void);

int url(const char *const in_fn, const char *const out_fn);
void url_(void);
size_t url_strip_query_fragment(const char *const uri, const size_t uri_len);
const char *url_from_here(const size_t fn_len, const char *const fn);
const char *url_from_output(const size_t fn_len, const char *const fn);
int url_is_fragment(const char *const str);
const char *url_encode(char const *const s, size_t length);

/* POSIX and URI -- may need to change if necessary. `url_dirsep` is included
 in the header. */
#define DIRSEPCHAR /
#define FRAGMENTCHAR #
#define SUBORDINATECHARS ?#
#define TWODOTS ..
#define DOT .

#define STR(s) #s
#define XSTR(s) STR(s)
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
