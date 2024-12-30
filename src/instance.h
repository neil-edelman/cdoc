

struct instance {
	const char *in_fn, *out_fn;
	enum format format;
	struct text *text; /* Text holds the translation-unit in memory. */
	struct scanner *scan; /* Scanner reads the text and lexes it. */
	struct report *report; /* Report writes to the documentation file. */
};
