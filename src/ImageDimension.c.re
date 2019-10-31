#include <stdio.h>  /* [f]printf fopen fclose fread */
#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include "../src/ImageDimension.h"

/** Attempt to read the size of a `jpeg`.
 @param[file] File that has been opened in binary mode and rewound; required.
 @param[width, height] Pointers that get overwritten on success; required.
 @return Success, otherwise `errno` will (probably) be set.
 @license <http://www.faqs.org/faqs/jpeg-faq/part1/> said to look up
 [rdjpgcom.c](https://github.com/ImageMagick/jpeg-turbo/blob/master/rdjpgcom.c),
 which has
 [IJG License](https://github.com/ImageMagick/jpeg-turbo/blob/master/README.ijg)
 and possibly related [ImageMagick](https://imagemagick.org/script/license.php).
 I used as a reference to write this function. */
static int jpeg_dim(FILE *const fp, unsigned *const width,
	unsigned *const height) {
	unsigned char f[8];
	unsigned skip;
	assert(fp && width && height);
	/* The start of the file has to be an `SOI`. */
	if(fread(f, 2, 1, fp) != 1) return 0;
	if(f[0] != 0xFF || f[1] != 0xD8) return errno = EDOM, 0;
	for( ; ; ) {
		/* Discard up until the last `0xFF`, then that is the marker type. */
		do { if(fread(f, 1, 1, fp) != 1) return 0; } while(f[0] != 0xFF);
		do { if(fread(f, 1, 1, fp) != 1) return 0; } while(f[0] == 0xFF);
		switch(f[0]) {
			case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC5: /* _sic_ */
			case 0xC6: case 0xC7: case 0xC9: /* _sic_ */ case 0xCA: case 0xCB:
			case 0xCD: /* _sic_ */ case 0xCE: case 0xCF:
				/* `SOF` markers. */
				if(fread(f, 8, 1, fp) != 1) return 0;
				if((skip = (f[0] << 8) | f[1]) != 8u + 3 * f[7])
					return errno = EDOM, 0;
				*width  = (f[5] << 8) | f[6];
				*height = (f[3] << 8) | f[4];
				return 1;
			case 0xD8: case 0xD9:
				/* Image data `SOS, EOI` without image size. */
				return errno = EDOM, 0;
			default:
				/* Skip the rest by reading it's size. */
				if(fread(f, 2, 1, fp) != 1) return 0;
				if((skip = (f[0] << 8) | f[1]) < 2) return errno = EDOM, 0;
				if(fseek(fp, skip - 2, SEEK_CUR) != 0) return 0;
		}
	}
}

/** Attempt to read the size of a `png`.
 [LibPNG specifications](http://www.libpng.org/pub/png/spec/1.2/png-1.2.pdf)
 @param[file] File that has been opened in binary mode and rewound; required.
 @param[width, height] Pointers that get overwritten on success; required.
 @return Success, otherwise `errno` will (probably) be set. */
static int png_dim(FILE *const fp, unsigned *const width,
	unsigned *const height) {
	unsigned char f[24];
	assert(fp && width && height);
	if(fread(f, 24, 1, fp) != 1) return 0;
	if(f[0] != 0x89 || f[1] != 0x50 || f[2] != 0x4E || f[3] != 0x47
		|| f[4] != 0x0D || f[5] != 0x0A || f[6] != 0x1A || f[7] != 0x0A
		|| f[12] != 0x49 || f[13] != 0x48 || f[14] != 0x44 || f[15] != 0x52)
		return errno = EDOM, 0;
	*width  = f[16] << 24 | f[17] << 16 | f[18] << 8 | f[19];
	*height = f[20] << 24 | f[21] << 16 | f[22] << 8 | f[23];
	return 1;
}

/*!re2c
re2c:yyfill:enable   = 0;
re2c:define:YYCTYPE  = char;
re2c:define:YYCURSOR = cursor;
re2c:define:YYMARKER = marker; */

int ImageDimension(const char *const fn, unsigned *const width,
	unsigned *const height) {
	const char *cursor = fn, *marker = cursor;
	FILE *fp = 0;
	int success = 0;
	if(!fn || !width || !height) return 0;
	if(!(fp = fopen(fn, "rb"))) goto catch;
/*!re2c
	* { fprintf(stderr, "%s: image format not reconised.\n", fn); goto catch; }
	// <https://en.wikipedia.org/wiki/JPEG> */
	[^\x00]* (".jpg" | ".jpeg" | ".jpe" | ".jif" | ".jfif" | ".jfi") "\x00"
		{ if(!jpeg_dim(fp, width, height)) goto catch; goto end; }
	[^\x00]* ".png" "\x00"
		{ if(!png_dim(fp, width, height)) goto catch; goto end; }
*/
end:
	success = 1;
	goto finally;
catch:
	if(errno) { perror(fn); errno = 0; }
finally:
	if(fp) fclose(fp);
	return success;
}
