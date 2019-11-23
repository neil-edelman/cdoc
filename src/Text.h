struct Text;

const char *TextName(const struct Text *const file);
const char *TextBaseName(const struct Text *const file);
size_t TextSize(const struct Text *const file);
const char *TextGet(const struct Text *const file);
struct Text *TextOpen(const char *const fn);
void TextCloseAll(void);
