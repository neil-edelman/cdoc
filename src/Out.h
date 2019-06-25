/**
 * Outputs a file when given a `SegmentArray`.
 */

/** @implements <Token>Action */
static void token_print(struct Token *const token) {
	printf("%.*s\n", token->length, token->from);
}

/** @implements <Tag>Action */
static void print_tag_contents(struct Tag *const tag) {
	TokenArrayForEach(&tag->contents, &token_print);
}

/** @implements <Tag>Action */
static void print_tag_header(struct Tag *const tag) {
	TokenArrayForEach(&tag->header, &token_print);
}

/** @implements <Tag>Action */
static void print_tag_header_contents(struct Tag *const tag) {
	printf("*");
	print_tag_header(tag);
	printf("* ");
	print_tag_contents(tag);
	printf("\n");
}

/* @implements <Tag>Predicate */
#define TAG_IS(lc, uc) static int tag_is_ ## lc (const struct Tag *const tag) \
	{ return tag->token.symbol == uc; }
TAG_IS(title, TAG_TITLE)
TAG_IS(param, TAG_PARAM)
TAG_IS(author, TAG_AUTHOR)
TAG_IS(std, TAG_STD)
TAG_IS(depend, TAG_DEPEND)
TAG_IS(version, TAG_VERSION)
TAG_IS(since, TAG_SINCE)
TAG_IS(fixme, TAG_FIXME)
TAG_IS(depricated, TAG_DEPRICATED)
TAG_IS(return, TAG_RETURN)
TAG_IS(throws, TAG_THROWS)
TAG_IS(implements, TAG_IMPLEMENTS)
TAG_IS(order, TAG_ORDER)
TAG_IS(allow, TAG_ALLOW)

/** @implements <Segment>Action */
static void segment_print_doc(struct Segment *const segment) {
	TokenArrayForEach(&segment->doc, &token_print);
}

/** @implements <Segment>Action */
static void segment_print_code(struct Segment *const segment) {
	TokenArrayForEach(&segment->code, &token_print);
	printf("\n");
}

/** @implements <Segment>Action */
static void segment_print_all(struct Segment *const segment) {
	segment_print_doc(segment);
	TagArrayIfEach(&segment->tags, &tag_is_author, &print_tag_contents);
	TagArrayIfEach(&segment->tags, &tag_is_std, &print_tag_contents);
	TagArrayIfEach(&segment->tags, &tag_is_depend, &print_tag_contents);
	TagArrayIfEach(&segment->tags, &tag_is_param, &print_tag_header_contents);
	segment_print_code(segment);
	printf("\n\n***\n\n");
}

/** @implements <Segment>Action */
static void segment_print_all_title(struct Segment *const segment) {
	TagArrayIfEach(&segment->tags, &tag_is_title, &print_tag_contents);
}

/** @implements <Segment>Predictate */
static int segment_is_header(const struct Segment *const segment) {
	return segment->section == HEADER;
}

/** @implements <Segment>Predictate */
static int segment_is_declaration(const struct Segment *const segment) {
	return segment->section == DECLARATION;
}

/** @implements <Segment>Predictate */
static int segment_is_function(const struct Segment *const segment) {
	return segment->section == FUNCTION;
}

static void out(struct SegmentArray *const sa) {
	assert(sa);
	printf("# ");
	SegmentArrayIfEach(sa, &segment_is_header, &segment_print_all_title);
	printf(" #\n\n");
	SegmentArrayIfEach(sa, &segment_is_header, &segment_print_doc);
	printf("\n\n## Declarations ##\n\n");
	SegmentArrayIfEach(sa, &segment_is_declaration, &segment_print_all);
	printf("\n\n## Functions ##\n\n");
	SegmentArrayIfEach(sa, &segment_is_function, &segment_print_code);
	printf("\n\n## Function Detail ##\n\n");
	SegmentArrayIfEach(sa, &segment_is_function, &segment_print_all);
}
