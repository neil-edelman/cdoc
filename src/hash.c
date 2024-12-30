/** Perform a 32 bit
 [Fowler/Noll/Vo FNV-1a hash](http://www.isthe.com/chongo/tech/comp/fnv/) on a
 string. This assumes that size of `int` is at least 32 bits; if this is not
 true, we may get a different answer, (`stdint.h` is `C99`.) */
static unsigned fnv_32a_str(const char *str) {
	const char *s = str;
	/* 32 bit FNV-1 and FNV-1a non-zero initial basis, FNV1_32A_INIT */
	unsigned hval = 0x811c9dc5;
	assert(str);
	/* FNV magic prime `FNV_32_PRIME 0x01000193`. */
	while(*s) {
		hval ^= (unsigned)*s++;
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
	}
	if(cdoc_get_debug() & DBG_HASH)
		fprintf(stderr, "fnv32: %s -> %u\n", str, hval);
	return hval & 0xffffffff;
}

unsigned hash_str(const char *const label) { return fnv_32a_str(label); }
