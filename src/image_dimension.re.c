#include <stdio.h>  /* printf fopen fclose fread */
#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include "../src/image_dimension.h"

/** Attempt to read the size of a `jpeg`.
 @param[file] File that has been opened in binary mode and rewound; required.
 @param[width, height] Pointers that get overwritten on success; required.
 @param[no_discarded] Optional discarded bytes pointer.
 @return Success, otherwise `errno` will (probably) be set.
 @throws[EILSEQ] Not a valid jpeg.
 @throws[fread]
 @license <http://www.faqs.org/faqs/jpeg-faq/part1/> said to look up
 [rdjpgcom.c](https://github.com/ImageMagick/jpeg-turbo/blob/master/rdjpgcom.c),
 which has
 [IJG License](https://github.com/ImageMagick/jpeg-turbo/blob/master/README.ijg)
 and possibly related [ImageMagick](https://imagemagick.org/script/license.php).
 I used as a reference to write this function. */
static int jpeg_dim(FILE *const fp, unsigned *const width,
	unsigned *const height, size_t *no_discarded) {
	unsigned char f[8];
	unsigned skip;
	assert(fp && width && height);
	if(no_discarded) *no_discarded = 0;
	/* The start of the file has to be an `SOI`. */
	if(fread(f, 2, 1, fp) != 1) return 0;
	if(f[0] != 0xFF || f[1] != 0xD8) return errno = EILSEQ, 0;
	for( ; ; ) {
		/* Unspecified what we do in this case, so let the caller decide. */
		for( ; ; ) {
			if(fread(f, 1, 1, fp) != 1) return 0;
			if(f[0] == 0xFF) break;
			if(no_discarded) (*no_discarded)++; else return errno = EILSEQ, 0;
		}
		/* Discard up until the last `0xFF`. */
		do { if(fread(f, 1, 1, fp) != 1) return 0; } while(f[0] == 0xFF);
		/* That is the marker type */
		switch(f[0]) {
		case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC5: /* _sic_ */
		case 0xC6: case 0xC7: case 0xC9: /* _sic_ */ case 0xCA: case 0xCB:
		case 0xCD: /* _sic_ */ case 0xCE: case 0xCF:
			/* `SOF` markers. */
			if(fread(f, 8, 1, fp) != 1) return 0;
			/* Check that the size of this block is consistent with the
			 number of components. */
			if((((unsigned)f[0] << 8) | f[1]) != 8u + 3 * f[7])
				return errno = EILSEQ, 0;
			/* It's fundamentally an unsigned operation, stop making it `int`. */
			*width  = (unsigned)(f[5] << 8u) | f[6];
			*height = (unsigned)(f[3] << 8u) | f[4];
			return 1;
		case 0xD8: case 0xD9:
			/* Image data `SOS, EOI` without image size. */
			return errno = EILSEQ, 0;
		default:
			/* Skip the rest by reading it's size. */
			if(fread(f, 2, 1, fp) != 1) return 0;
			if((skip = (unsigned)(f[0] << 8u) | f[1]) < 2)
				return errno = EILSEQ, 0;
			if(fseek(fp, skip - 2, SEEK_CUR) != 0) return 0;
		}
	}
}

/** Attempt to read the size of a `png`.
 [LibPNG specifications](http://www.libpng.org/pub/png/spec/1.2/png-1.2.pdf)
 @param[file] File that has been opened in binary mode and rewound; required.
 @param[width, height] Pointers that get overwritten on success; required.
 @return Success, otherwise `errno` will (probably) be set.
 @throws[EILSEQ] Not valid PNG data.
 @throws[fread] */
static int png_dim(FILE *const fp, unsigned *const width,
	unsigned *const height) {
	unsigned char f[24];
	assert(fp && width && height);
	if(fread(f, 24, 1, fp) != 1) return 0;
	if(f[0] != 0x89 || f[1] != 0x50 || f[2] != 0x4E || f[3] != 0x47
		|| f[4] != 0x0D || f[5] != 0x0A || f[6] != 0x1A || f[7] != 0x0A
		|| f[12] != 0x49 || f[13] != 0x48 || f[14] != 0x44 || f[15] != 0x52)
		return errno = EILSEQ, 0;
	*width  = (unsigned)(f[16] << 24 | f[17] << 16 | f[18] << 8) | f[19];
	*height = (unsigned)(f[20] << 24 | f[21] << 16 | f[22] << 8) | f[23];
	return 1;
}

/*!re2c
re2c:yyfill:enable   = 0;
re2c:define:YYCTYPE  = char;
re2c:define:YYCURSOR = cursor;
re2c:define:YYMARKER = marker; */

int image_dimension(const char *const fn, unsigned *const width,
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
		{ if(!jpeg_dim(fp, width, height, 0)) goto catch; goto end; }
	[^\x00]* ".png" "\x00"
		{ if(!png_dim(fp, width, height)) goto catch; goto end; }
*/
end:
	success = 1;
	goto finally;
catch:
	perror(fn), errno = 0;
finally:
	if(fp) fclose(fp);
	return success;
}
