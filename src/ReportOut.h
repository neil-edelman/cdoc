/** Selects `token` out of `ta` and prints it and returns the next token. */
typedef const struct Token *(*OutFn)(const struct TokenArray *const ta,
	const struct Token *const token);

static struct {
	int level;
	int is_space;
} output;

/*static struct SymbolDetails {
} symbol_details[] = { SYMBOL(PARAM2_B) };*/

/* @implements <Attribute>Predicate */
#define OUT(name) static const struct Token *name(const struct TokenArray \
*const ta, const struct Token *const token)

OUT(lit) {
	printf("%.*s", token->length, token->from);
	return TokenArrayNext(ta, token);
}
OUT(lit_both) {
	if(output.is_space) fputc(' ', stdout);
	output.is_space = 1;
	return lit(ta, token);
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
	fprintf(stderr, "Expected: generic(id);\n%s.\n", pos(token));
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
	fprintf(stderr, "Expected: generic(id,id);\n%s.\n", pos(token));
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
	fprintf(stderr, "Expected: generic(id,id,id);\n%s.\n", pos(token));
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
	fprintf(stderr, "Expected: \\url{<cat url>};\n%s.\n", pos(token));
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
	fprintf(stderr, "Expected: \\cite{<source>};\n%s.\n", pos(token));
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
	printf("`:code}");
	return TokenArrayNext(ta, next);
	catch:
	fprintf(stderr, "Expected: `<math/code>`;\n%s.\n", pos(token));
	return 0;
}
OUT(em) {
	struct Token *next = TokenArrayNext(ta, token);
	printf("{em:`");
	while(next->symbol != EM_END) {
		printf("%.*s~", next->length, next->from);
		if(!(next = TokenArrayNext(ta, next))) goto catch;
	}
	printf("`:em}");
	return TokenArrayNext(ta, next);
	catch:
	fprintf(stderr, "Expected: _<emphasis>_;\n%s.\n", pos(token));
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

/** @implements <Attribute>Action */
static void print_att_contents(struct Attribute *const att) {
	tokens_print(&att->contents);
}

/** @implements <Attribute>Action */
static void print_att_header(struct Attribute *const att) {
	tokens_print(&att->header);
}

/** @implements <Attribute>Action */
static void print_att_header_contents(struct Attribute *const att) {
	printf("<att:%s # ", symbols[att->symbol]);
	print_att_header(att);
	printf(" #\n");
	print_att_contents(att);
	printf(">\n");
}

/* @implements <Attribute>Predicate */
#define ATT_IS(lc, uc) static int att_is_ ## lc \
(const struct Attribute *const att) { return att->symbol == uc; }
ATT_IS(title, ATT_TITLE)
ATT_IS(param, ATT_PARAM)
ATT_IS(author, ATT_AUTHOR)
ATT_IS(std, ATT_STD)
ATT_IS(depend, ATT_DEPEND)
/*ATT_IS(version, ATT_VERSION)
 ATT_IS(since, ATT_SINCE)
 ATT_IS(fixme, ATT_FIXME)
 ATT_IS(depricated, ATT_DEPRICATED)
 ATT_IS(return, ATT_RETURN)
 ATT_IS(throws, ATT_THROWS)
 ATT_IS(implements, ATT_IMPLEMENTS)
 ATT_IS(order, ATT_ORDER)
 ATT_IS(allow, ATT_ALLOW)*/

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
	AttributeArrayIfEach(&segment->attributes, &att_is_author, &print_att_contents);
	AttributeArrayIfEach(&segment->attributes, &att_is_std, &print_att_contents);
	AttributeArrayIfEach(&segment->attributes, &att_is_depend, &print_att_contents);
	AttributeArrayIfEach(&segment->attributes, &att_is_param, &print_att_header_contents);
	printf("\n\n***\n\n");
}

/** @implements <Segment>Action */
static void segment_print_all_title(struct Segment *const segment) {
	AttributeArrayIfEach(&segment->attributes, &att_is_title, &print_att_contents);
}

/** @implements <Segment>Predictate */
static int segment_is_header(const struct Segment *const segment) {
	return segment->division == DIV_PREAMBLE;
}

/** @implements <Segment>Predictate */
static int segment_is_declaration(const struct Segment *const segment) {
	return segment->division == DIV_TAG || segment->division == DIV_TYPEDEF;
}

/** @implements <Segment>Predictate */
static int segment_is_function(const struct Segment *const segment) {
	return segment->division == DIV_FUNCTION;
}

void ReportDebug(void) {
	struct Segment *segment = 0;
	struct Attribute *att = 0;
	/*struct Segment {
	 enum Division division;
	 struct TokenArray doc, code;
	 struct TokenRefArray params;
	 struct AttributeArray attributes;
	 };
	 struct Attribute {
	 enum Symbol symbol;
	 struct TokenArray header;
	 struct TokenArray contents;
	 };	
	 struct Token {
	 enum Symbol symbol;
	 const char *from;
	 int length;
	 size_t line;
	 };*/
	while((segment = SegmentArrayNext(&report, segment))) {
		printf("Segment: %s;\n"
			   "code: %s;\n"
			   "params: %s;\n"
			   "doc: %s.\n",
			   divisions[segment->division],
			   TokenArrayToString(&segment->code),
			   TokenRefArrayToString(&segment->params),
			   TokenArrayToString(&segment->doc));
		while((att = AttributeArrayNext(&segment->attributes, att)))
			printf("%s{%s} %s.\n", symbols[att->symbol],
				   TokenArrayToString(&att->header),
				   TokenArrayToString(&att->contents));
		fputc('\n', stdout);
	}
}

static int preamble_attribute_exists(const enum Symbol symbol) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment))) {
		struct Attribute *attribute = 0;
		if(segment->division != DIV_PREAMBLE) continue;
		while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
			if(attribute->symbol == symbol) return 1;
	}
	return 0;
}

static void preamble_attribute_print(const enum Symbol symbol) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment))) {
		struct Attribute *attribute = 0;
		if(segment->division != DIV_PREAMBLE) continue;
		while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
		{
			if(attribute->symbol != symbol) continue;
		}
	}
}

/** Outputs a file when given a `SegmentArray`. */
void ReportOut(void) {
	/* Print header. */
	printf("<header:title># ");
	SegmentArrayIfEach(&report, &segment_is_header, &segment_print_all_title);
	printf(" #\n\n");
	printf("<header:doc>\n");
	SegmentArrayIfEach(&report, &segment_is_header, &segment_print_doc);
	/* Print typedefs. */
	printf("\n\n## Typedefs ##\n");
	/* Print tags. */
	printf("\n\n## Tags ##\n");
	/* Print general declarations. */
	printf("\n\n## Declarations ##\n\n");
	SegmentArrayIfEach(&report, &segment_is_declaration, &segment_print_all);
	printf("\n\n## Functions ##\n\n");
	SegmentArrayIfEach(&report, &segment_is_function, &segment_print_code);
	printf("\n\n## Function Detail ##\n\n");
	SegmentArrayIfEach(&report, &segment_is_function, &segment_print_all);
}
