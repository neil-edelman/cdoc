int Paths(const char *const in_fn, const char *const out_fn);
void Paths_(void);
size_t PathsStripQueryFragment(const char *const uri, const size_t uri_len);
const char *PathsFromHere(const size_t fn_len, const char *const fn);
const char *PathsFromOutput(const size_t fn_len, const char *const fn);
const char *PathsSafeFragment(const size_t fn_len, const char *const fn);
const char *PathsSafeFragmentHref(const size_t fn_len, const char *const fn);

#define XSTR(s) STR(s)
#define STR(s) #s

/* POSIX and URI -- may need to change if necessary. `path_dirsep` is included
 in the header. I know, query string fragment. */
#define DIRSEPCHAR /
#define FRAGMENTCHARS ?#
#define TWODOTS ..
#define DOT .

/* XSTR(#DIRSEP ## #OTHER_CHARS), crashes `clang`, hmm. */

static const char
	*const path_dirsep = XSTR(DIRSEPCHAR),
	*const path_fragment = XSTR(FRAGMENTCHARS),
	*const path_searchdirsep = XSTR(DIRSEPCHAR) XSTR(FRAGMENTCHARS),
	*const path_notallowed = XSTR(DIRSEPCHAR) XSTR(DIRSEPCHAR),
	*const path_twodots = XSTR(TWODOTS),
	*const path_dot = XSTR(DOT);

#undef XSTR
#undef STR
