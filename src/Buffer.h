struct Buffer;

void Buffer_(struct Buffer **const pb);
const char *BufferGet(const struct Buffer *const b);
void BufferClear(struct Buffer *const b);
char *BufferPrepare(struct Buffer *const b, const size_t length);
