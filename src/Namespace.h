/* Define the namespaces of output; used by `Scanner.c.re_c` and
 `Semantic.c.re`. */
#define NAMESPACE(X) X(NAME_PREAMBLE), X(NAME_FUNCTION), X(NAME_TAG), \
	X(NAME_TYPEDEF), X(NAME_DATA)
enum Namespace { NAMESPACE(PARAM) };
static const char *const namespaces[] = { NAMESPACE(STRINGISE) };
