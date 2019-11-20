struct Buffer;

void Buffer_(struct Buffer **const p_file);
struct Buffer *Buffer(const char *const fn);
const char *BufferName(const struct Buffer *const file);
const char *BufferBaseName(const struct Buffer *const file);
size_t BufferSize(const struct Buffer *const file);
const char *BufferGet(const struct Buffer *const file);
