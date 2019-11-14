int Paths(const char *const in_fn, const char *const out_fn);
void Paths_(void);
size_t PathsStripQueryFragment(const char *const uri, const size_t uri_len);
const char *PathsFromHere(const size_t fn_len, const char *const fn);
const char *PathsFromOutput(const size_t fn_len, const char *const fn);
