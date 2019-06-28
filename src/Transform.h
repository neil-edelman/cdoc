/** Deletes all new-lines, except where there is a at-least double newline in a
 text-block, which it will replace with a single new-line. */
static void clean_whitespace(struct TokenArray *const sa) {
	const struct TokenArray *replace;
	struct Token *x = 0, *x_start = 0;
	size_t count_nl = 0;
	int is_content = 0;
	assert(sa);
	while((x = TokenArrayNext(sa, x))) {
		if(x->symbol == NEWLINE) {
			if(!x_start) x_start = x;
			count_nl++;
		} else {
			if(x_start) {
				replace = (is_content && count_nl > 1) ? &paragraph : 0;
				count_nl = 0;
				TokenArrayReplace(sa, x_start, (long)(x - x_start), replace);
				x = x_start + TokenArraySize(replace);
				x_start = 0;
			}
			is_content = 1;
		}
	}
	/* Whitespace at end of section. */
	if(x_start) TokenArrayReplace(sa, x_start, -1, 0);
	printf("Parser:Transform:clean_whitespace: %s.\n", TokenArrayToString(sa));
}

/** Keeps only the stuff we care about.
 @implements{Predicate<Segment>} */
static int keep_segment(const struct Segment *const s) {
	/* fixme: and not static or containing @allow tag. */
	if(TokenArraySize(&s->doc) || s->section == FUNCTION) return 1;
	return 0;
}
