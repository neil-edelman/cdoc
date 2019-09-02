/** Selects `token` out of `tokens` and prints it and returns the next token. */
typedef const struct Token *(*OutFn)(const struct TokenArray *const tokens,
	const struct Token *const token);

static struct {
	int level;
	int is_space_before;
	int is_newline_before;
} output;
static void output_reset(void) {
	output.level = 0;
	output.is_space_before = output.is_newline_before = 0;
}



/* `SYMBOL` is declared in `Scanner.h`. */
static const int symbol_lspaces[] = { SYMBOL(PARAM5D) };
static const int symbol_rspaces[] = { SYMBOL(PARAM5E) };


static void whitespace(void) {
	fputc('~', stdout);
}
static void whitespace_if_needed(enum Symbol symbol) {
	if(output.is_space_before && symbol_lspaces[symbol])
		printf("^")/*whitespace()*/;
	output.is_space_before = symbol_rspaces[symbol];
}


/* @implements <Attribute>Predicate */
#define OUT(name) static const struct Token *name(const struct TokenArray \
*const tokens, const struct Token *const token)

OUT(ws) {
	whitespace();
	return TokenArrayNext(tokens, token);
}
OUT(lit) {
	printf("%.*s", token->length, token->from);
	return TokenArrayNext(tokens, token);
}
OUT(gen1) {
	struct Token *const lparen = TokenArrayNext(tokens, token),
	*const param = TokenArrayNext(tokens, lparen),
	*const rparen = TokenArrayNext(tokens, param);
	const char *a, *type;
	int type_size;
	if(!lparen || lparen->symbol != LPAREN || !param || !rparen
	   || rparen->symbol != RPAREN) goto catch;
	type = token->from;
	if(!(a = strchr(type, '_'))) goto catch;
	type_size = (int)(a - type);
	assert(token->length == a + 1 - token->from);
	printf("<%.*s>%.*s",
		   token->length - 1, token->from, param->length, param->from);
	return TokenArrayNext(tokens, rparen);
	catch:
	fprintf(stderr, "Expected: generic(id); %s.\n", pos(token));
	return 0;
}
OUT(gen2) {
	struct Token *const lparen = TokenArrayNext(tokens, token),
	*const param1 = TokenArrayNext(tokens, lparen),
	*const comma = TokenArrayNext(tokens, param1),
	*const param2 = TokenArrayNext(tokens, comma),
	*const rparen = TokenArrayNext(tokens, param2);
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
	printf("<%.*s>%.*s<%.*s>%.*s", type1_size, type1, param1->length,
		   param1->from, type2_size, type2, param2->length, param2->from);
	return TokenArrayNext(tokens, rparen);
	catch:
	fprintf(stderr, "Expected: generic(id,id); %s.\n", pos(token));
	return 0;
}
OUT(gen3) {
	struct Token *const lparen = TokenArrayNext(tokens, token),
	*const param1 = TokenArrayNext(tokens, lparen),
	*const comma1 = TokenArrayNext(tokens, param1),
	*const param2 = TokenArrayNext(tokens, comma1),
	*const comma2 = TokenArrayNext(tokens, param2),
	*const param3 = TokenArrayNext(tokens, comma2),
	*const rparen = TokenArrayNext(tokens, param3);
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
	printf("<%.*s>%.*s<%.*s>%.*s<%.*s>%.*s", type1_size, type1,
		   param1->length, param1->from, type2_size, type2, param2->length,
		   param2->from, type3_size, type3, param3->length, param3->from);
	return TokenArrayNext(tokens, rparen);
	catch:
	fprintf(stderr, "Expected: A_B_C_(id,id,id); %s.\n", pos(token));
	return 0;
}
OUT(esc_bs) {
	printf("\\?");
	return TokenArrayNext(tokens, token);
}
OUT(url) {
	if(!token || token->symbol != URL || token->has_continuation) goto catch;
	printf("<%.*s>", token->length, token->from);
	return TokenArrayNext(tokens, token);
	catch:
	fprintf(stderr, "Expected: \"<[url]>\"; %s.\n", pos(token));
	return 0;
}
OUT(cite) {
	struct Token *const lbr = TokenArrayNext(tokens, token),
	*next = TokenArrayNext(tokens, lbr); /* Variable no. */
	if(!lbr || lbr->symbol != LBRACE || !next) goto catch;
	printf("(");
	while(next->symbol != RBRACE) {
		printf("%.*s", next->length, next->from);
		if(!(next = TokenArrayNext(tokens, next))) goto catch;
	}
	printf(")[https://scholar.google.ca/scholar?q=");
	next = TokenArrayNext(tokens, lbr);
	while(next->symbol != RBRACE) {
		/* fixme: escape url! */
		printf("%.*s_", next->length, next->from);
		if(!(next = TokenArrayNext(tokens, next))) goto catch;
	}
	printf("]");
	return TokenArrayNext(tokens, next);
	catch:
	fprintf(stderr, "Expected: \\cite{<source>}; %s.\n", pos(token));
	return 0;
}
OUT(see_fn) {
	printf("(fixme)\\see<fn>");
	return TokenArrayNext(tokens, token);
}
OUT(see_tag) {
	printf("(fixme)\\see<tag>");
	return TokenArrayNext(tokens, token);
}
OUT(see_typedef) {
	printf("(fixme)\\see<typedef>");
	return TokenArrayNext(tokens, token);
}
OUT(see_data) {
	printf("(fixme)\\see<data>");
	return TokenArrayNext(tokens, token);
}
OUT(math) { /* Math and code. */
	struct Token *next = TokenArrayNext(tokens, token);
	printf("{code:`");
	while(next->symbol != MATH_END) {
		printf("%.*s", next->length, next->from);
		if(!(next = TokenArrayNext(tokens, next))) goto catch;
	}
	printf("`:code}");
	return TokenArrayNext(tokens, next);
	catch:
	fprintf(stderr, "Expected: `<math/code>`; %s.\n", pos(token));
	return 0;
}
OUT(em) {
	struct Token *next = TokenArrayNext(tokens, token);
	printf("{em:`");
	while(next->symbol != EM_END) {
		printf("%.*s", next->length, next->from);
		if(!(next = TokenArrayNext(tokens, next))) goto catch;
	}
	printf("`:em}");
	return TokenArrayNext(tokens, next);
	catch:
	fprintf(stderr, "Expected: _<emphasis>_; %s.\n", pos(token));
	return 0;
}
OUT(par) {
	printf("^\n^\n");
	return TokenArrayNext(tokens, token);
}



/* `SYMBOL` is declared in `Scanner.h`. */
static const OutFn symbol_outs[] = { SYMBOL(PARAM5C) };



static void tokens_print(const struct TokenArray *const tokens) {
	const struct Token *token = TokenArrayNext(tokens, 0);
	OutFn sym_out;
	if(!token) return;
	while((sym_out = symbol_outs[token->symbol])
		&& (whitespace_if_needed(token->symbol),
		token = sym_out(tokens, token)));
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
	output_reset();
	while((segment = SegmentArrayNext(&report, segment))) {
		struct Attribute *attribute = 0;
		if(segment->division != DIV_PREAMBLE) continue;
		while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
		{
			if(attribute->symbol != symbol) continue;
			tokens_print(&attribute->contents);
		}
	}
}

/** Outputs a file when given a `SegmentArray`. */
void ReportOut(void) {
	/* Print header. */
	if(preamble_attribute_exists(ATT_TITLE)) {
		printf("<header:title># ");
		preamble_attribute_print(ATT_TITLE);
		/*SegmentArrayIfEach(&report, &segment_is_header, &segment_print_all_title);*/
		printf(" #\n\n");
	} else {
		printf("<no title>\n");
	}
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
