/** 2017 Neil Edelman, distributed under the terms of the MIT License;
 see readme.txt, or \url{ https://opensource.org/licenses/MIT }.

 A dynamic string, intended to be used with modified UTF-8 encoding,
 \url{ https://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8 }. That is, this is a
 wrapper that automatically expands memory as needed around a standard C
 null-terminated string in a monolithic array and is compatible with {ASCII}.
 If you need to edit a potentially large string just one of {Text} will be
 generally linear-time and is unsuited alone for such a purpose.

 \see{TextGet} exposes a read-only, null-terminated {char *}. If an error
 occurs, (with memory allocation, for example,) these functions shall return a
 null pointer; all functions shall accept a null pointer being passed to them
 and return null immediately: this means one can compose functions safely.
 Reset the error with \see{TextGetError}; also, \see{TextIsError}.

 @param TEXT_DEBUG
 Prints debug information to {stderr}.

 @title		Text
 @author	Neil
 @std		C89/90; part of Common
 @version	2018-01
 @since		2017-03
 @fixme		TextCharAt, TextDelete, TextInsert,
 TextSetCharAt, TextSubsequence? */

#include <stdlib.h> /* malloc realloc free */
#include <stdio.h>  /* FILE fgets ferror vsnprintf fprintf */
#include <errno.h>	/* errno */
#include <string.h>	/* strerror strlen memmove memcpy strpbrk strdup memchr */
#include <limits.h>	/* INT_MAX */
#include <assert.h>	/* assert */
#include <ctype.h>	/* isspace */
#include <stdarg.h>	/* va_* */
#include "Text.h"

/* used in \see{Matches} */
static const size_t fibonacci6  = 8;
static const size_t fibonacci7  = 13;

/* used in \see{Text} */
static const size_t fibonacci11 = 89;
static const size_t fibonacci12 = 144;

/* used in \see{TextMatch}. */
static const char right_side[256] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', ')', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '>', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	']', '\0', '\0', '\0', '\0', '\'', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '}', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };

enum Error {
	E_NO_ERROR,
	E_ERRNO,
	E_PARAMETER,
	E_OVERFLOW,
	E_SYNTAX,
	E_CODE_CHOPPED
};
static const char *const error_explination[] = {
	"no error",
	0,
	"parameter out-of-range",
	"overflow",
	"syntax error"
};

static enum Error global_error = E_NO_ERROR;
static int        global_errno_copy;

struct Text {
	char *text;
	size_t length, capacity[2];
	enum Error error;
	int errno_copy;
};

/* private structs */

struct TextBraket {
	int is;
	const struct TextPattern *pattern;
	struct { char *s0, *s1; } bra, ket;
};

struct TextMatch {
	struct Text *text;
	char *start, *end;
};

struct TextMatches {
	struct Text *parent;
	struct TextMatch *matches;
	size_t size, capacity[2];
};

struct TextCut {
	int is;
	char *pos, stored;
};

static struct {
	int is_valid;
	struct Text *parent;
	size_t start, end;
} match_info;

/* private prototypes */

static void clear(struct Text *const this);
static struct Text *cat(struct Text *const this, const char *const str,
	const size_t str_len);
static int capacity_up(struct Text *const this, const size_t *const len_ptr);
static void swap_texts(struct Text *const a, struct Text *const b);
static char *alternate_root(char *const string, const int left,const int right);

static void t_cut(struct TextCut *const this, char *const pos);
static void t_uncut(struct TextCut *const this);

static int Matches(struct TextMatches *const this, struct Text *const parent);
static void Matches_(struct TextMatches *const this);
static int Matches_capacity_up(struct TextMatches *const this);
static struct TextMatch *Matches_new(struct TextMatches *const this,
	struct TextBraket *braket);
static void debug(struct Text *const this, const char *const fn,
	const char *const fmt, ...);



/**********
 * Public */

/** @return	A new, blank Text, will be constructed.
 @throws	E_ERRNO */
struct Text *Text(void) {
	struct Text *this;
	if(!(this = malloc(sizeof *this))) {
		global_error      = E_ERRNO;
		global_errno_copy = errno;
		Text_(&this);
		return 0;
	}
	this->text        = 0;
	this->length      = 0;
	this->capacity[0] = fibonacci11;
	this->capacity[1] = fibonacci12;
	this->error       = E_NO_ERROR;
	this->errno_copy  = 0;
	if(!(this->text = malloc(sizeof *this->text * this->capacity[0]))) {
		global_error      = E_ERRNO;
		global_errno_copy = errno;
		Text_(&this);
		return 0;
	}
	clear(this);
	debug(this, "Text", "constructed.\n");
	return this;
}

/** @param this_ptr: A pointer to Text that will be destructed. */
void Text_(struct Text **const this_ptr) {
	struct Text *this;

	if(!this_ptr || !(this = *this_ptr)) return;

	debug(this, "~Text", "destructing.\n");
	free(this->text);
	free(this);

	*this_ptr = 0;
}

/** Volatile, in the sense that it exposes the buffer; specifically, not
 guaranteed to last between {Text} calls to the same object. If you want a
 copy, do {strdup(TextGet(text))}.
 @return The string associated to {this}. */
const char *TextGet(const struct Text *const this) {
	if(!this) return 0;
	return this->text;
}

/** @return Gets the length in bytes, not code-points.
 @order O(1) */
size_t TextLength(const struct Text *const this) {
	if(!this) return 0;
	return this->length;
}

/** @return How many code-points in
 \url{ https://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8 }. If it is not a
 valid string in {UTF-8}, this will return an undefined value between
 {[0, size]}.
 @order O({size})
 @fixme Untested. */
size_t TextCodePointCount(const struct Text *const this) {
	if(!this) return 0;
	{
		char *text = this->text, ch;
		const char *const end_null = this->text + this->length;
		size_t length = this->length;
		assert(*end_null == '\0');
		while(text < end_null) {
			/* Less and less likely; optimise for the top. */
			if((ch = *text) > 0)    { text++; continue; }
			if((ch & 0xE0) == 0xC0) { text += 2; length -= 1; continue; }
			if((ch & 0xF0) == 0xE0) { text += 3; length -= 2; continue; }
			if((ch & 0xF8) == 0xF0) { text += 4; length -= 3; continue; }
			/* RFC 3629: "treat any ill-formed code unit sequence as an error
			 condition." Skip. */
			text++, length--;
		}
		return length;
	}
}

/** Opposite of {isEmpty}; the justification for it being this way is we want
 to be say {TextHasContents(0) = 0}. Use to see if \see{TextSep} has any more
 tokens.
 @return True if the buffer is not empty. */
int TextHasContent(const struct Text *const this) {
	return !this || *this->text == '\0' ? 0 : -1;
}

/** Clears the Text.
 @return {this}. */
struct Text *TextClear(struct Text *const this) {
	if(!this) return 0;
	clear(this); return this;
}

/** White-space trims the buffer associated with {this} using {isspace} only on
 the right.
 @return {this}.
 @fixme Untested. */
struct Text *TextRightTrim(struct Text *const this) {
	char *str, *z;
	if(!this || !this->length) return this;
	str = this->text;
	z = str + this->length - 1;
	while(z >= str && isspace(*z)) z--;
	z++, *z = '\0';
	this->length = (size_t)(z - str);
	return this;
}

/** White-space trims the buffer associated with {this} using {isspace}.
 @return {this}. */
struct Text *TextTrim(struct Text *const this) {
	char *str, *a, *z;
	if(!this || !this->length) return this;
	str = this->text;
	z = str + this->length - 1, a = str;
	while(z >= str && isspace(*z)) z--;
	z++, *z = '\0';
	while(isspace(*a)) a++;
	this->length = (size_t)(z - a);
	if(a - str) memmove(str, a, this->length + 1);
	return this;
}

/** Separates a new token at the first of the {delims} that satisfy {pred}.
 @return If {this_ptr} or {this} is null, returns null. Otherwise, you must
 call \see{Text_} on this pointer. If no tokens are found in {this}, {this} is
 destroyed and the entire {Text} is returned.
 @param delims: If null, uses {POSIX} white-space to separate, ({Python}
 {split(' ')}.)
 @param pred: Can be null, in which case, it behaves like {true}.
 @throws E_OVERFLOW, E_ERRNO */
struct Text *TextSep(struct Text **const this_ptr, const char *delims,
	const TextPredicate pred) {
	struct Text *this, *token;
	char *bork;
	if(!this_ptr || !(this = *this_ptr)) return 0;
	if(!delims || !*delims) delims = " \f\n\r\t\v";
	/* Find. */
	for(bork = this->text; (bork = strpbrk(bork, delims)); bork++) {
		if(!pred || pred(this->text, bork)) break;
	}
	/* Not found. Just return the original. */
	if(!bork) {
		*this_ptr = 0;
		return this;
	}
	/* Or else new text for the split at {bork}. This is a lot of wasted space
	 for large strings.
	 @fixme The error handling is lame; use {errno}? */
	if(!(token = Text())) {
		this->error = global_error, global_error = E_NO_ERROR;
		this->errno_copy = global_errno_copy; global_errno_copy = 0;
		return 0;
	}
	if(!cat(token, bork + 1, (size_t)(this->text + this->length - (bork + 1)))){
		this->error = token->error, this->errno_copy = token->errno_copy;
		Text_(&token);
		return 0;
	}
	/* Truncate at {bork}. */
	*bork        = '\0';
	this->length = bork - this->text;
	/* {this} is the truncated first token, and {token} is the rest: swap. */
	swap_texts(this, token);
	debug(this, "TextSep", "split.");
	return token;
}

/** Replaces the buffer in {this} with {str}.
 @return	{this}.
 @throws	E_PARAMETER, E_OVERFLOW, E_ERRNO */
struct Text *TextCopy(struct Text *const this, const char *const str) {
	if(!this) return 0;
	if(!str) { this->error = E_PARAMETER; return 0; }
	clear(this);
	return cat(this, str, strlen(str));
}

/** Concatenates {cat} onto the buffer in {this}.
 @return	{this}.
 @throws	E_PARAMETER, E_OVERFLOW, E_ERRNO */
struct Text *TextCat(struct Text *const this, const char *const str) {
	if(!this) return 0;
	if(!str) { this->error = E_PARAMETER; return 0; }
	return cat(this, str, strlen(str));
}

/** Concatenates up to {cat_len} characters of {cat} onto the buffer in {this}.
 @return	{this}.
 @throws	E_PARAMETER, E_OVERFLOW, E_ERRNO */
struct Text *TextNCat(struct Text *const this, const char *const str,
	const size_t str_len) {
	const char *end;
	if(!this) return 0;
	if(!str) { this->error = E_PARAMETER; return 0; }
	end = memchr(str, 0, str_len);
	return cat(this, str, end ? (size_t)(end - str) : str_len);
}

/** Concatenates {this} with {[a, b]}. If {a} > {b}, then empty.
 @return	{this}.
 @throws	E_PARAMETER, E_OVERFLOW, E_ERRNO */
struct Text *TextBetweenCat(struct Text *const this,
	const char *const a, const char *const b) {
	if(!this) return 0;
	if(!a || !b) { this->error = E_PARAMETER; return 0; }
	return (a <= b) ? cat(this, a, (size_t)(b - a + 1)) : this;
}

/** Concatenates the contents of the text file, {fp}, after the read cursor, to
 the buffer in {this}. On success, the read cursor will be at the end.
 @return {this}.
 @throws E_PARAMETER, E_OVERFLOW, E_ERRNO
 @deprecated Use {StoryFileCat} for more performance and line numbers. */
struct Text *TextFileCat(struct Text *const this, FILE *const fp) {
	size_t to_get;
	int to_get_int;
	int e;
	if(!this) return 0;
	if(!fp) { this->error = E_PARAMETER; return 0; }
	while(to_get = this->capacity[0] - this->length,
		to_get_int = to_get < INT_MAX ? (int)(to_get) : INT_MAX,
		fgets(this->text + this->length, to_get_int, fp)) {
		this->length += strlen(this->text + this->length);
		if(this->length >= this->capacity[0] - 1 && !capacity_up(this, 0))
			return 0;
	}
	if((e = ferror(fp)))
		{ this->error = E_ERRNO, this->errno_copy = e; return 0; }
	debug(this, "TextFileCat", "appended file descriptor %d.\n", (long)fp);
	return this;
}

/** Concatenates one line on the text file, {fp}, after the read cursor, to the
 buffer in {this}. On success, the read cursor will be at the end.
 @param fp: If this is null, this returns 0.
 @return On true, the file had more lines and a line was stored. If false, the
 file does not have more lines or an error occured; use \see{TextIsError} to
 differentiate.
 @throws E_OVERFLOW, E_ERRNO */
int TextFileLineCat(struct Text *const this, FILE *const fp) {
	size_t to_get;
	int to_get_int;
	int e;
	if(!this || !fp) return 0;
	while(to_get = this->capacity[0] - this->length,
		to_get_int = to_get < INT_MAX ? (int)(to_get) : INT_MAX,
		fgets(this->text + this->length, to_get_int, fp)) {
		this->length += strlen(this->text + this->length);
		/* "NULL on error or when end of file occurs while no characters have
		 been read." So this is always true. */
		assert(this->length >= 1);
		if(this->text[this->length - 1] == '\n') break;
		if(this->length >= this->capacity[0] - 1 && !capacity_up(this, 0))
			return 0;
	}
	if((e = ferror(fp)))
	{ this->error = E_ERRNO, this->errno_copy = e; return 0; }
	debug(this, "TextFileLineCat",
		"appended a line from file descriptor %d.\n", (long)fp);
	/* Exactly the same as if we'd had an {length_init != length_final}. */
	return !feof(fp);
}

/** Concatenates the buffer with a {printf};
 \url{http://pubs.opengroup.org/onlinepubs/007908799/xsh/fprintf.html}.
 @return {this}.
 @fixme Have a function that allows replacing in the middle,
 TextPrintCat(this, fmt, ...) -> TextPrint(this, n, fmt, ...) */
struct Text *TextPrintCat(struct Text *const this, const char *const fmt, ...) {
	va_list argp;
	char garbage;
	int length;
	size_t total_length;

	if(!this) return 0;
	if(!fmt) { this->error = E_PARAMETER; return 0; }

	va_start(argp, fmt);
	length = vsnprintf(&garbage, 0ul, fmt, argp);
	va_end(argp);

	if(length < 0) { this->error = E_ERRNO, this->errno_copy = errno; return 0;}
	total_length = this->length + length;
	if(total_length < (size_t)length) { this->error = E_OVERFLOW; return 0; }
	if(!capacity_up(this, &total_length)) return 0;

	va_start(argp, fmt);
	length = vsnprintf(this->text + this->length, this->capacity[0], fmt, argp);
	va_end(argp);

	if(length < 0) { this->error = E_ERRNO, this->errno_copy = errno; return 0;}
	this->length += length;

	debug(this, "TextPrintfCat", "printed.\n");
	return this;
}

/** Transforms the original text according to {fmt}.
 @param fmt: Accepts %% as '%' and %s as the original string.
 @return {this}.
 @throws E_OVERFLOW, E_ERRNO */
struct Text *TextTransform(struct Text *const this, const char *fmt) {
	char *copy, *t;
	const char *f;
	size_t copy_len = 0;

	if(!this) return 0;
	if(!(copy = strdup(this->text)))
		{ this->error = E_ERRNO, this->errno_copy = errno; return 0; }

	/* count */
	for(f = fmt; *f; f++) {
		if(*f != '%') { copy_len++; continue; }
		switch(*++f) {
			case '%': copy_len++; break;
			case 's': copy_len += this->length; break;
		}
	}
	/* allocate */
	if(!capacity_up(this, &copy_len)) { free(copy); return 0; }
	/* new string */
	for(t = this->text, f = fmt; *f; f++) {
		if(*f != '%') { *t++ = *f; continue; }
		switch(*++f) {
			case '%': *t++ = '%'; break;
			case 's': memcpy(t, copy, this->length), t += this->length; break;
		}
	}
	*t = '\0';
	this->length = copy_len;
	/* free */
	free(copy);

	debug(this, "TextTransform", "transformed.\n");
	return this;
}

/** Transforms {this} according to all specified {patterns} array of
 {patterns_size}.
 @return {this}.
 @param patterns: An array of {TextPattern { const char *start, *end;
 TextAction transform; }; when the {begin} of a pattern encompasses another
 pattern, it should be before in the array. All patterns must have a non-empty
 string, {begin}, and {TextAction}, {transform}; {end} is optional; where
 missing, it will just call {transform} with {begin}. */
struct Text *TextMatch(struct Text *const this,
	const struct TextPattern *const patterns, const size_t patterns_size) {
	struct Text *temp = 0;
	char *s0, *cursor;
	/* no, I don't use it uninitialised, but . . . whatever */
	struct TextBraket braket = { 0, 0, {0, 0}, {0, 0} }; /* working */
	struct TextMatches matches; /* storage array */
	struct TextMatch *match; /* one out of the array */
	struct TextCut cut = { 0, 0, 0 };
	enum { E_NO, E_MISSING, E_BUMP, E_TEMP, E_THIS } e = E_NO;

	if(!this || !Matches(&matches, this)) return 0;

	cursor = this->text;
	do { /* this is an actual do-loop! */
		const struct TextPattern *pattern; size_t p;

		/* search for the next pattern, short-circuit O(n^2) */
		braket.is = 0;
		for(p = 0; p < patterns_size; p++) {
			pattern = patterns + p;
			if(!(s0 = strstr(cursor, pattern->start))) continue;
			/* this happens when first_pos is [abcdefg] and [cdef] is matched */
			if(braket.is && s0 >= braket.bra.s0) continue;
			t_uncut(&cut);
			braket.is = -1;
			braket.pattern = pattern;
			braket.bra.s0 = s0;
			braket.bra.s1 = s0 + strlen(pattern->start);
			t_cut(&cut, braket.bra.s1);
		}
		if(!braket.is) break;

		/* if the pattern has an ending, search for it */
		if(braket.pattern->end) {
			/* determine if it's braces, then match them, else do plain */
			const char left  = braket.bra.s1[-1];
			const char right = right_side[(int)left];
			t_uncut(&cut);
			if(!(braket.ket.s0 = (right && right == *braket.pattern->end)
				? alternate_root(braket.bra.s1 - 1, left, right)
				: strstr(braket.bra.s1, braket.pattern->end)))
				{ this->error = E_SYNTAX; return 0; }
			braket.ket.s1 = braket.ket.s0 + strlen(braket.pattern->end);
			t_cut(&cut, braket.ket.s0);
		}

		/* allocate the recursion; set the value back to how it was */
		if(!(match = Matches_new(&matches, &braket))) { e = E_BUMP; break; }
		t_uncut(&cut);
		if(braket.pattern->transform) {
			/* this assumes uni-process! */
			match_info.parent = this;
			match_info.start  = braket.bra.s0 - this->text;
			match_info.end    = (braket.pattern->end ?
				braket.ket.s1 : braket.bra.s1) - this->text;
			/* call the handler */
			match_info.is_valid = -1; /* ++ (we want them to be invalidated) */
			braket.pattern->transform(match->text);
			match_info.is_valid = 0; /* -- */
		}
		cursor = braket.pattern->end ? braket.ket.s1 : braket.bra.s1;

	} while(cursor);

	t_uncut(&cut);

	switch(e) {
		case E_NO:		break;
		case E_MISSING:	this->error = E_SYNTAX; break;
		case E_BUMP:	break;
		default:		break;
	}

	/* now go though the matches and substitute them in to {temp} */
	do {
		size_t i;
		if(e) break;
		cursor = this->text;
		if(!(temp = Text())) { e = E_TEMP; break; }
		for(i = 0; i < matches.size; i++) {
			match = matches.matches + i;
			if(!cat(temp, cursor, (size_t)(match->start - cursor))
				|| !cat(temp, match->text->text, match->text->length))
				{ e = E_TEMP; break; }
			cursor = match->end;
		}
		if(e) break;
		if(!cat(temp, cursor, strlen(cursor))) { e = E_TEMP; break; }
		swap_texts(this, temp);
	} while(0);

	switch(e) {
		case E_NO:		break;
		case E_TEMP:
			/* propagate */
			if(temp) {
				this->error = temp->error, this->errno_copy = temp->errno_copy;
			} else {
				this->error = global_error, this->errno_copy =global_errno_copy,
				global_error = E_NO_ERROR, global_errno_copy = 0;
			}
			break;
		case E_THIS:	break;
		default:		break;
	}

	Matches_(&matches);
	Text_(&temp);

	debug(this, "TextMatch", "final matched.\n");
	return e ? 0 : this;
}

/** When {TextMatch} finds a match, it also stores a global data on where the
 match occurred within the parent. This gets that information for the handler.
 Concurrent uses of \see{TextMatch} leave this fuction ill-defined. If any
 pointers passed are null, ignores them. If one calls \see{TextMatch} from
 within the handler, then {TextGetMatchInfo} will not work after that point.
 @return Success; otherwise the values are invalid and will not be set. */
int TextGetMatchInfo(struct Text **parent_ptr,
	size_t *const start_ptr, size_t *const end_ptr) {
	if(!match_info.is_valid) return 0;
	if(parent_ptr) *parent_ptr = match_info.parent;
	if(start_ptr)   *start_ptr = match_info.start;
	if(end_ptr)       *end_ptr = match_info.end;
	return -1;
}

/** @return Whether an error has occurred on {this}; can be null. */
int TextIsError(struct Text *const this) {
	return this ? (this->error ? -1 : 0) : (global_error ? -1 : 0);
}

/** Resets the error flag.
 @return A lower-case string, (or in the case of a E_ERRNO, the first letter
 has an extraneous upper case on most systems,) without any punctuation, that
 explains the last error associated with {this}; can be null. */
const char *TextGetError(struct Text *const this) {
	const char *str;
	enum Error *perr;
	int *perrno;

	perr   = this ? &this->error      : &global_error;
	perrno = this ? &this->errno_copy : &global_errno_copy;
	if(!(str = error_explination[*perr])) str = strerror(*perrno);
	*perr = 0, *perrno = 0;

	return str;
}





/************
 * Private. */

/** Clears the text. */
static void clear(struct Text *const this) {
	this->text[0] = '\0';
	this->length  = 0;
}

/** @throws E_OVERFLOW, E_ERRNO */
static struct Text *cat(struct Text *const this, const char *const str,
	const size_t str_len) {
	const size_t old_len = this->length, new_len = old_len + str_len;

	if(new_len == old_len) return this;
	if(new_len < old_len) { this->error = E_OVERFLOW; return 0; }
	if(!capacity_up(this, &new_len)) return 0;
	memcpy(this->text + old_len, str, str_len);
	this->text[new_len] = '\0';
	this->length = new_len;

#ifdef TEXT_DEBUG
	if(strlen(this->text) != this->length) {
		fprintf(stderr, "Error '%s' strlen %lu length %lu\n", this->text,
			strlen(this->text), this->length);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "cat: '%s' <- '%.*s'; %lu <- %lu\n", this->text,
		(int)str_len, str, this->length, str_len);
#endif

	return this;
}

/** Ensures value capacity.
 @param size_ptr: Can be null, in which case the capacity increases one level
in size. If it is not, the capacity increases at or beyond this number.
 @return Success.
 @throws E_OVERFLOW, E_ERRNO */
static int capacity_up(struct Text *const this, const size_t *const len_ptr) {
	size_t c0, c1;
	char *text;

	/* fibonacci */
	c0 = this->capacity[0];
	c1 = this->capacity[1];
	while(!len_ptr || c0 <= *len_ptr) {
		if(c0 == (size_t)-1) return this->error = E_OVERFLOW, 0;
		c0 ^= c1, c1 ^= c0, c0 ^= c1, c1 += c0;
		if(c1 <= c0) c1 = (size_t)-1;
		if(!len_ptr) break;
	}
	if(this->capacity[0] >= c0) return -1; /* it's already that size */
	debug(this, "capacity_up", "%lu->%lu for %lu+1.\n", this->capacity[0], c0, len_ptr ? *len_ptr : 0);
	if(!(text = realloc(this->text, c0 * sizeof *this->text)))
		return this->error = E_ERRNO, this->errno_copy = errno, 0;
	this->text = text, this->capacity[0] = c0, this->capacity[1] = c1;

	return -1;
}

/** Switches the pointers of the two buffers; struct Text has file scope, so
 this is reasonably safe, and avoids all the copying. */
static void swap_texts(struct Text *const a, struct Text *const b) {
	/* https://en.wikipedia.org/wiki/XOR_swap_algorithm */
	{
		char *const temp = a->text;
		a->text = b->text;
		b->text = temp;
	} {
		size_t temp = a->length;
		a->length = b->length;
		b->length = temp;
	} {
		size_t temp = a->capacity[0];
		a->capacity[0] = b->capacity[0];
		b->capacity[0] = temp;
	} {
		size_t temp = a->capacity[1];
		a->capacity[1] = b->capacity[1];
		b->capacity[1] = temp;
	}
}

/** {alternate_root("[[[foo]]]bar", '[', ']')}, would return {"]bar"} */
static char *alternate_root(char *const string, const int left,const int right){
	unsigned stack = 1;
	char *s = string + 1;

	if(*string != left) return s;
	while(*s) {
		if(*s == left) stack++;
		else if(*s == right && !--stack) return s;
		s++;
	}
	return 0;
}

/***********************************************
 * TextCut (just initialise a new to all zero) */

/** Stores the character at {pos} in {this} and terminates the string there. */
static void t_cut(struct TextCut *const this, char *const pos) {
	if(this->is) return;
	this->is = -1, this->pos = pos, this->stored = *pos, *pos = '\0';
}
/** If the string has been cut, undoes that. */
static void t_uncut(struct TextCut *const this) {
	if(!this->is) return;
	this->is = 0;
	*this->pos = this->stored;
}

/***************
 * TextMatches */

/** Initialise. All errors are bumped up to the parent; you can just return 0.
 @return Success.
 @throws E_ERRNO */
static int Matches(struct TextMatches *const this, struct Text *const parent) {
	this->parent      = parent;
	this->matches     = 0;
	this->size        = 0;
	this->capacity[0] = fibonacci6;
	this->capacity[1] = fibonacci7;
	if(!(this->matches = malloc(this->capacity[0] * sizeof *this->matches)))
		return parent->error = E_ERRNO, parent->errno_copy = errno, 0;
	return -1;
}

/** Destroy. */
static void Matches_(struct TextMatches *const this) {
	size_t i;

	for(i = 0; i < this->size; i++) {
		Text_(&this->matches[i].text);
	}
	free(this->matches);
	this->matches = 0;
	this->size = 0;
}

/** Increases the buffer size.
 @throws E_OVERFLOW, E_ERRNO */
static int Matches_capacity_up(struct TextMatches *const this) {
	size_t c0, c1;
	struct TextMatch *matches;

	c0 = this->capacity[0];
	c1 = this->capacity[1];
	if(c0 == (size_t)-1) return this->parent->error = E_OVERFLOW, 0;
	c0 ^= c1;
	c1 ^= c0;
	c0 ^= c1;
	c1 += c0;
	if(c1 <= c0) c1 = (size_t)-1;
	if(!(matches = realloc(this->matches, c0 * sizeof *this->matches)))
		return this->parent->error = E_ERRNO, this->parent->errno_copy=errno, 0;
	this->matches = matches, this->capacity[0] = c0, this->capacity[1] = c1;

	return -1;
}

/** Constructs new match within {this}. */
static struct TextMatch *Matches_new(struct TextMatches *const this,
	struct TextBraket *braket) {
	struct TextMatch *match;
	if(this->size >= this->capacity[0] && !Matches_capacity_up(this)) return 0;
	match        = this->matches + this->size;
	match->text  = 0;
	match->start = braket->bra.s0;
	match->end   = braket->pattern->end ? braket->ket.s1 : braket->bra.s1;
	if(!(match->text = Text())) {
		this->parent->error      = global_error;
		this->parent->errno_copy = global_errno_copy;
		return 0;
	}
	if(braket->pattern->end && !cat(match->text, braket->bra.s1,
		(size_t)(braket->ket.s0 - braket->bra.s1))) {
		this->parent->error      = match->text->error;
		this->parent->errno_copy = match->text->errno_copy;
		Text_(&match->text);
		return 0;
	}
	/*printf("Matches_new: '%s'\n", TextGet(match->text));*/
	this->size++;
	return match;
}

static void debug(struct Text *const this, const char *const fn,
	const char *const fmt, ...) {
#ifdef TEXT_DEBUG
	const size_t length = this->text ? strlen(this->text) : 0;
	va_list parg;

	va_start(parg, fmt);
	fprintf(stderr, "Text.%s[%.60s]: ", fn, this->text);
	vfprintf(stderr, fmt, parg);
	va_end(parg);
	if(length != this->length) fprintf(stderr, "Text.length %lu but strlen %lu."
		"\n", this->length, length), exit(EXIT_FAILURE);
#else
	UNUSED(this); UNUSED(fn); UNUSED(fmt);
#endif
}
