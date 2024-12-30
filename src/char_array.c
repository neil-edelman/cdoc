#include <stdio.h>

static void char_to_string(const char *buffer, char (*const a)[12]) {
	const char *b0;
	unsigned left = 10 /* 12 - "\"\0" */;
	char b, *cur = *a;
	*cur++ = '\"', left--;
read:
	b = *buffer;
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
	b0 = buffer;
	if((*++buffer & 192) != 128) { if(left < 3) goto end; else goto invalid; }
	*cur++ = *b0++, *cur++ = *b0++, left -= 2;
	goto advance;
three:
	b0 = buffer;
	if((*++buffer & 192) != 128
		|| (*++buffer & 192) != 128) goto invalid;
	*cur++ = *b0++, *cur++ = *b0++, *cur++ = *b0++, left -= 3;
	goto advance;
four:
	b0 = buffer;
	if((*++buffer & 192) != 128
		|| (*++buffer & 192) != 128
		|| (*++buffer & 192) != 128) goto invalid;
	*cur++ = *b0++, *cur++ = *b0++, *cur++ = *b0++, *cur++ = *b0++, left -= 4;
	goto advance;
advance:
	if(!left) goto end;
	buffer++;
	goto read;
end:
	cur[0] = '\"', cur[1] = '\0';
}
#define DEFINE
#include "char_array.h"
