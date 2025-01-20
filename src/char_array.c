#include <stdio.h>
#include <ctype.h>

static void char_to_string(const char *const ch, char (*const a)[12]) {
	const unsigned char c = (unsigned char)*ch;
	unsigned char *const b = (unsigned char *)*a;
	if(isprint(c)) {
		/* Replace ' ' with Middle Dot. */
		if(c == ' ') b[0] = (unsigned char)0xC2, b[1] = (unsigned char)0xB7, b[2] = '\0';
		else b[0] = c, b[1] = '\0';
	} else {
		const unsigned char *hex = (const unsigned char *)"0123456789ABCDEF";
		/* Non-printable single-byte values become hex. */
		/*b[0] = '0', b[1] = 'x',*/
		b[0] = hex[c >> 4], b[1] = hex[c & 0x0F], b[2] = '\0';
	}
#if 0
	/* This is printing _strings_, we want to print _characters_. */
	const char *b0;
	unsigned left = 10 /* 12 - "\"\0" */;
	char b, *cur = *a;
	*cur++ = '\"', left--;
read:
	b = *ch;
	if(b == '\0') goto end; /* Modified UTF-8 overlong encoding of U+0000. */
	if((b & 128) == 0) goto one;
	if(left == 1) goto end;
	if((b & 224) == 192) goto two;
	if(left == 2) goto end;
	if((b & 240) == 224) goto three;
	if((b & 248) == 240) { if(left < 4) goto end; else goto four; }
	goto invalid;
invalid: /* U+FFFD �, this is 3 bytes. */
	cur[0] = (char)239, cur[1] = (char)191, cur[3] = (char)189, cur += 3, left -= 3;
	goto advance;
one:
	*cur++ = b, left--;
	goto advance;
two:
	b0 = ch;
	if((*++ch & 192) != 128) { if(left < 3) goto end; else goto invalid; }
	*cur++ = *b0++, *cur++ = *b0++, left -= 2;
	goto advance;
three:
	b0 = ch;
	if((*++ch & 192) != 128
		|| (*++ch & 192) != 128) goto invalid;
	*cur++ = *b0++, *cur++ = *b0++, *cur++ = *b0++, left -= 3;
	goto advance;
four:
	b0 = ch;
	if((*++ch & 192) != 128
		|| (*++ch & 192) != 128
		|| (*++ch & 192) != 128) goto invalid;
	*cur++ = *b0++, *cur++ = *b0++, *cur++ = *b0++, *cur++ = *b0++, left -= 4;
	goto advance;
advance:
	if(!left) goto end;
	ch++;
	goto read;
end:
	cur[0] = '\"', cur[1] = '\0';
#endif
}
#define DEFINE
#include "char_array.h"

/** Copies `str` (must not be null) into `a` (must not be null.)
 @return Success.
 @throws[malloc] @throws[ERANGE] The string is non-terminal in a `size_t`. */
int char_array_copy(struct char_array *const a, const char *const str) {
	size_t len;
	assert(a && str);
	len = strlen(str);
	if(len == ~((size_t)0)) return errno = ERANGE, 0;
	if(!char_array_reserve(a, len + 1)) return 0;
	a->size = len + 1, memcpy(a->data, str, len + 1);
	return 1;
}
