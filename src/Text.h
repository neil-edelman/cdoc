#ifndef HAVE_TEXT_H /* <-- guards */
#define HAVE_TEXT_H

/* unused macro */
#ifdef UNUSED
#undef UNUSED
#endif
#define UNUSED(a) ((void)(a))

/** See \see{Text}. */
struct Text;

/** Action function. */
typedef void (*TextAction)(struct Text *const);
/** Predicate function.
 @param string: The string.
 @param sub: The position in the string which you must make a true/false
 decision. Necessarily, {sub \in string}. */
typedef int (*TextPredicate)(const char *const string, const char *sub);

/** Used in \see{TextMatch} as an array of patterns. Recognises brackets.
 @param start: Must be at least one character.
 @param end: can be null, in which case, {start} is the whole text.
 @param transform: if {end}, copies a buffer ({start}, {end}) as argument;
 can be null, it will just ignore. */
struct TextPattern {
	const char *start, *end;
	TextAction transform;
};

struct Text *Text(void);
void Text_(struct Text **const this_ptr);
const char *TextGet(const struct Text *const this);
size_t TextGetLength(const struct Text *const this);
size_t TextCodePointCount(const struct Text *const this);
int TextHasContent(const struct Text *const this);
struct Text *TextClear(struct Text *const this);
struct Text *TextTrim(struct Text *const this);
struct Text *TextSep(struct Text *const this, const char *delims,
	const TextPredicate pred);
struct Text *TextCopy(struct Text *const this, const char *const str);
struct Text *TextCat(struct Text *const this, const char *const str);
struct Text *TextNCat(struct Text *const this, const char *const str,
	const size_t cat_len);
struct Text *TextBetweenCat(struct Text *const this,
	const char *const a, const char *const b);
struct Text *TextFileCat(struct Text *const this, FILE *const fp);
struct Text *TextPrintCat(struct Text *const this, const char *const fmt, ...);
struct Text *TextTransform(struct Text *const this, const char *fmt);
struct Text *TextMatch(struct Text *const this,
	const struct TextPattern *const patterns, const size_t patterns_size);
int TextGetMatchInfo(struct Text **const parent_ptr,
	size_t *const start_ptr, size_t *const end_ptr);
int TextIsError(struct Text *const this);
const char *TextGetError(struct Text *const this);

#endif /* guards --> */
