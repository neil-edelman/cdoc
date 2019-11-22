struct Text;

void Text_(struct Text **const p_file);
struct Text *Text(const char *const fn);
const char *TextName(const struct Text *const file);
const char *TextBaseName(const struct Text *const file);
size_t TextSize(const struct Text *const file);
const char *TextGet(const struct Text *const file);
