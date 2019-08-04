/** Selects `token` out of `ta` and prints it and returns the next token. */
typedef const struct Token *(*OutFn)(const struct TokenArray *const ta,
	const struct Token *const token);

/* @implements <Tag>Predicate */
#define OUT(name) static const struct Token *name(const struct TokenArray \
	*const ta, const struct Token *const token)
OUT(lit) {
	printf("%.*s~", token->length, token->from);
	return TokenArrayNext(ta, token);
}
OUT(gen1) {
	struct Token *const lparen = TokenArrayNext(ta, token),
		*const param = TokenArrayNext(ta, lparen),
		*const rparen = TokenArrayNext(ta, param);
	const char *a, *type;
	int type_size;
	if(!lparen || lparen->symbol != LPAREN || !param || !rparen
		|| rparen->symbol != RPAREN) goto catch;
	type = token->from;
	if(!(a = strchr(type, '_'))) goto catch;
	type_size = (int)(a - type);
	assert(token->length == a + 1 - token->from);
	printf("<%.*s>%.*s~",
		token->length - 1, token->from, param->length, param->from);
	return TokenArrayNext(ta, rparen);
catch:
	fprintf(stderr, "Expected: generic(id);\n%s.\n", pos());
	return 0;
}
OUT(gen2) {
	struct Token *const lparen = TokenArrayNext(ta, token),
		*const param1 = TokenArrayNext(ta, lparen),
		*const comma = TokenArrayNext(ta, param1),
		*const param2 = TokenArrayNext(ta, comma),
		*const rparen = TokenArrayNext(ta, param2);
	const char *a, *type1, *type2;
	int type1_size, type2_size;
	if(!lparen || lparen->symbol != LPAREN || !param1 || !comma
		|| comma->symbol != COMMA || !param2 || !rparen
		|| rparen->symbol != RPAREN) goto catch;
	type1 = token->from;
	if(!(a = strchr(type1, '_'))) goto catch;
	type1_size = (int)(a - type1);
	type2 = a + 1;
	if(!(a = strchr(type2, '_'))) goto catch;
	type2_size = (int)(a - type2);
	assert(token->length == a + 1 - token->from);
	printf("<%.*s>%.*s<%.*s>%.*s~", type1_size, type1, param1->length,
		param1->from, type2_size, type2, param2->length, param2->from);
	return TokenArrayNext(ta, rparen);
catch:
	fprintf(stderr, "Expected: generic(id,id);\n%s.\n", pos());
	return 0;
}
OUT(gen3) {
	struct Token *const lparen = TokenArrayNext(ta, token),
		*const param1 = TokenArrayNext(ta, lparen),
		*const comma1 = TokenArrayNext(ta, param1),
		*const param2 = TokenArrayNext(ta, comma1),
		*const comma2 = TokenArrayNext(ta, param2),
		*const param3 = TokenArrayNext(ta, comma2),
		*const rparen = TokenArrayNext(ta, param3);
	const char *a, *type1, *type2, *type3;
	int type1_size, type2_size, type3_size;
	if(!lparen || lparen->symbol != LPAREN || !param1 || !comma1
		|| comma1->symbol != COMMA || !param2 || !comma2 ||
		comma2->symbol != COMMA || !param3 || !rparen
		|| rparen->symbol != RPAREN) goto catch;
	type1 = token->from;
	if(!(a = strchr(type1, '_'))) goto catch;
	type1_size = (int)(a - type1);
	type2 = a + 1;
	if(!(a = strchr(type2, '_'))) goto catch;
	type2_size = (int)(a - type2);
	type3 = a + 1;
	if(!(a = strchr(type3, '_'))) goto catch;
	type3_size = (int)(a - type3);
	assert(token->length == a + 1 - token->from);
	printf("<%.*s>%.*s<%.*s>%.*s<%.*s>%.*s~", type1_size, type1,
		param1->length, param1->from, type2_size, type2, param2->length,
		param2->from, type3_size, type3, param3->length, param3->from);
	return TokenArrayNext(ta, rparen);
	catch:
	fprintf(stderr, "Expected: generic(id,id,id);\n%s.\n", pos());
	return 0;
}
OUT(esc_bs) {
	printf("\\~");
	return TokenArrayNext(ta, token);
}
OUT(url) {
	struct Token *const lbr = TokenArrayNext(ta, token),
		*next = TokenArrayNext(ta, lbr); /* Variable no. */
	if(!lbr || lbr->symbol != LBRACE || !next) goto catch;
	printf("(");
	while(next->symbol != RBRACE) {
		/* We don't care about the symbol's meaning in the url. */
		printf("%.*s", next->length, next->from);
		if(!(next = TokenArrayNext(ta, next))) goto catch;
	}
	printf(")~");
	return TokenArrayNext(ta, next);
catch:
	fprintf(stderr, "Expected: \\url{<cat url>};\n%s.\n", pos());
	return 0;
}
OUT(cite) {
	struct Token *const lbr = TokenArrayNext(ta, token),
		*next = TokenArrayNext(ta, lbr); /* Variable no. */
	if(!lbr || lbr->symbol != LBRACE || !next) goto catch;
	printf("(");
	while(next->symbol != RBRACE) {
		printf("%.*s~", next->length, next->from);
		if(!(next = TokenArrayNext(ta, next))) goto catch;
	}
	printf(")[https://scholar.google.ca/scholar?q=");
	next = TokenArrayNext(ta, lbr);
	while(next->symbol != RBRACE) {
		/* fixme: escape url! */
		printf("%.*s_", next->length, next->from);
		if(!(next = TokenArrayNext(ta, next))) goto catch;
	}
	printf("]~");
	return TokenArrayNext(ta, next);
catch:
	fprintf(stderr, "Expected: \\cite{<source>};\n%s.\n", pos());
	return 0;
}
OUT(see) { /* fixme: Have a new field in segment. */
	printf("(fixme)\\see");
	return TokenArrayNext(ta, token);
}
OUT(math) { /* Math and code. */
	struct Token *next = TokenArrayNext(ta, token);
	printf("{code:`");
	while(next->symbol != MATH_END) {
		printf("%.*s", next->length, next->from);
		if(!(next = TokenArrayNext(ta, next))) goto catch;
	}
	printf("`:code}~");
	return TokenArrayNext(ta, next);
catch:
	fprintf(stderr, "Expected: `<math/code>`;\n%s.\n", pos());
	return 0;
}
OUT(em) {
	struct Token *next = TokenArrayNext(ta, token);
	printf("{em:`");
	while(next->symbol != EM_END) {
		printf("%.*s~", next->length, next->from);
		if(!(next = TokenArrayNext(ta, next))) goto catch;
	}
	printf("`:em}~");
	return TokenArrayNext(ta, next);
	catch:
	fprintf(stderr, "Expected: _<emphasis>_;\n%s.\n", pos());
	return 0;
}
OUT(par) {
	printf("^\n^\n");
	return TokenArrayNext(ta, token);
}

/* `SYMBOL` is declared in `Scanner.h` and `PARAM3_C` is one of the preceding
 functions. */
static const OutFn symbol_out[] = { SYMBOL(PARAM3_C) };

static void tokens_print(const struct TokenArray *const ta) {
	const struct Token *token = TokenArrayNext(ta, 0);
	OutFn sym_out;
	if(!token) return;
	while((sym_out = symbol_out[token->symbol])
		&& (token = sym_out(ta, token)));
	fputc('\n', stdout);
}

/** @implements <Tag>Action */
static void print_tag_contents(struct Tag *const tag) {
	tokens_print(&tag->contents);
}

/** @implements <Tag>Action */
static void print_tag_header(struct Tag *const tag) {
	tokens_print(&tag->header);
}

/** @implements <Tag>Action */
static void print_tag_header_contents(struct Tag *const tag) {
	printf("<tag:%s # ", symbols[tag->symbol]);
	print_tag_header(tag);
	printf(" #\n");
	print_tag_contents(tag);
	printf(">\n");
}

/* @implements <Tag>Predicate */
#define TAG_IS(lc, uc) static int tag_is_ ## lc (const struct Tag *const tag) \
	{ return tag->symbol == uc; }
TAG_IS(title, TAG_TITLE)
TAG_IS(param, TAG_PARAM)
TAG_IS(author, TAG_AUTHOR)
TAG_IS(std, TAG_STD)
TAG_IS(depend, TAG_DEPEND)
/*TAG_IS(version, TAG_VERSION)
TAG_IS(since, TAG_SINCE)
TAG_IS(fixme, TAG_FIXME)
TAG_IS(depricated, TAG_DEPRICATED)
TAG_IS(return, TAG_RETURN)
TAG_IS(throws, TAG_THROWS)
TAG_IS(implements, TAG_IMPLEMENTS)
TAG_IS(order, TAG_ORDER)
TAG_IS(allow, TAG_ALLOW)*/

/** @implements <Segment>Action */
static void segment_print_doc(struct Segment *const segment) {
	tokens_print(&segment->doc);
}

/** @implements <Segment>Action */
static void segment_print_code(struct Segment *const segment) {
	tokens_print(&segment->code);
	printf("\n");
}

/** @implements <Segment>Action */
static void segment_print_all(struct Segment *const segment) {
	segment_print_code(segment);
	segment_print_doc(segment);
	TagArrayIfEach(&segment->tags, &tag_is_author, &print_tag_contents);
	TagArrayIfEach(&segment->tags, &tag_is_std, &print_tag_contents);
	TagArrayIfEach(&segment->tags, &tag_is_depend, &print_tag_contents);
	TagArrayIfEach(&segment->tags, &tag_is_param, &print_tag_header_contents);
	printf("\n\n***\n\n");
}

/** @implements <Segment>Action */
static void segment_print_all_title(struct Segment *const segment) {
	TagArrayIfEach(&segment->tags, &tag_is_title, &print_tag_contents);
}

/** @implements <Segment>Predictate */
static int segment_is_header(const struct Segment *const segment) {
	return segment->namespace == NAME_PREAMBLE;
}

/** @implements <Segment>Predictate */
static int segment_is_declaration(const struct Segment *const segment) {
	return segment->namespace == NAME_TAG || segment->namespace == NAME_TYPEDEF;
}

/** @implements <Segment>Predictate */
static int segment_is_function(const struct Segment *const segment) {
	return segment->namespace == NAME_FUNCTION;
}

/** Outputs a file when given a `SegmentArray`. */
static void out(struct SegmentArray *const sa) {
	assert(sa);

	/* Print header. */
	printf("->Header:\n");
	printf(" # ");
	SegmentArrayIfEach(sa, &segment_is_header, &segment_print_all_title);
	printf(" #\n\n");
	SegmentArrayIfEach(sa, &segment_is_header, &segment_print_doc);

	/* Print typedefs. */
	printf("->Typedefs:\n");

	/* Print tags. */
	printf("->Tags:\n");

	/* Print general declarations. */
	printf("->Declarations:\n");
	printf("\n\n## Declarations ##\n\n");
	SegmentArrayIfEach(sa, &segment_is_declaration, &segment_print_all);

	printf("->Function summary:\n");
	printf("\n\n## Functions ##\n\n");
	SegmentArrayIfEach(sa, &segment_is_function, &segment_print_code);

	printf("->Function detail:\n");
	printf("\n\n## Function Detail ##\n\n");
	SegmentArrayIfEach(sa, &segment_is_function, &segment_print_all);
}
