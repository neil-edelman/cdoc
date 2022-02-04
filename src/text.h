struct text;

const char *text_name(const struct text *const file);
const char *text_base_name(const struct text *const file);
size_t text_size(const struct text *const file);
const char *text_get(const struct text *const file);
struct text *text_open(const char *const fn);
void text_close_all(void);
