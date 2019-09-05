/** Selects `token` out of `tokens` and prints it and returns the next token. */
typedef int (*OutFn)(const struct TokenArray *const tokens,
	const struct Token **const ptoken);
/* @param[ptoken] Is an [in/out] variable, it should get updated unless the
 return value is false.
 @return Success.
 @implements <Attribute>Predicate */
#define OUT(name) static int name(const struct TokenArray *const tokens, \
	const struct Token **ptoken)

static struct {
	int level;
	int is_before;
	void (*fn)(const char);
} spaces;
static void spaces_reset(void (*fn)(const char)) {
	assert(fn);
	spaces.level = 0;
	spaces.is_before = 0;
	spaces.fn = fn;
}
static void spaces_force(void) { spaces.is_before = 1; }



/* `SYMBOL` is declared in `Scanner.h`. */
static const int symbol_lspaces[] = { SYMBOL(PARAM5D) };
static const int symbol_rspaces[] = { SYMBOL(PARAM5E) };


static void whitespace(const char debug) {
	fputc(debug/*' '*/, stdout);
}
static void newline(const char debug) {
	printf("(%c\n\n%c)", debug, debug);
}
static void whitespace_if_needed(enum Symbol symbol) {
	assert(spaces.fn);
	if(spaces.is_before && symbol_lspaces[symbol]) spaces.fn('^');
	spaces.is_before = symbol_rspaces[symbol];
}



OUT(ws) {
	const struct Token *const space = *ptoken;
	assert(tokens && space && space->symbol == SPACE);
	whitespace('~');
	*ptoken = TokenArrayNext(tokens, space);
	return 1;
}
OUT(par) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == NEWLINE);
	newline('~');
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(lit) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->length > 0);
	printf("%.*s", t->length, t->from);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(gen1) {
	const struct Token *const t = *ptoken,
		*const lparen = TokenArrayNext(tokens, t),
		*const param = TokenArrayNext(tokens, lparen),
		*const rparen = TokenArrayNext(tokens, param);
	const char *a, *type;
	int type_size;
	assert(tokens && t && t->symbol == ID_ONE_GENERIC);
	if(!lparen || lparen->symbol != LPAREN || !param || !rparen
	   || rparen->symbol != RPAREN) goto catch;
	type = t->from;
	if(!(a = strchr(type, '_'))) goto catch;
	type_size = (int)(a - type);
	assert(t->length == a + 1 - t->from);
	printf("<%.*s>%.*s",
		t->length - 1, t->from, param->length, param->from);
	*ptoken = TokenArrayNext(tokens, rparen);
	return 1;
	catch:
	fprintf(stderr, "Expected: generic(id); %s.\n", pos(t));
	return 0;
}
OUT(gen2) {
	const struct Token *const t = *ptoken,
		*const lparen = TokenArrayNext(tokens, t),
		*const param1 = TokenArrayNext(tokens, lparen),
		*const comma = TokenArrayNext(tokens, param1),
		*const param2 = TokenArrayNext(tokens, comma),
		*const rparen = TokenArrayNext(tokens, param2);
	const char *a, *type1, *type2;
	int type1_size, type2_size;
	assert(tokens && t && t->symbol == ID_TWO_GENERICS);
	if(!lparen || lparen->symbol != LPAREN || !param1 || !comma
		|| comma->symbol != COMMA || !param2 || !rparen
		|| rparen->symbol != RPAREN) goto catch;
	type1 = t->from;
	if(!(a = strchr(type1, '_'))) goto catch;
	type1_size = (int)(a - type1);
	type2 = a + 1;
	if(!(a = strchr(type2, '_'))) goto catch;
	type2_size = (int)(a - type2);
	assert(t->length == a + 1 - t->from);
	printf("<%.*s>%.*s<%.*s>%.*s", type1_size, type1, param1->length,
		   param1->from, type2_size, type2, param2->length, param2->from);
	*ptoken = TokenArrayNext(tokens, rparen);
	return 1;
	catch:
	fprintf(stderr, "Expected: generic2(id,id); %s.\n", pos(t));
	return 0;
}
OUT(gen3) {
	const struct Token *const t = *ptoken,
		*const lparen = TokenArrayNext(tokens, t),
		*const param1 = TokenArrayNext(tokens, lparen),
		*const comma1 = TokenArrayNext(tokens, param1),
		*const param2 = TokenArrayNext(tokens, comma1),
		*const comma2 = TokenArrayNext(tokens, param2),
		*const param3 = TokenArrayNext(tokens, comma2),
		*const rparen = TokenArrayNext(tokens, param3);
	const char *a, *type1, *type2, *type3;
	int type1_size, type2_size, type3_size;
	assert(tokens && t && t->symbol == ID_THREE_GENERICS);
	if(!lparen || lparen->symbol != LPAREN || !param1 || !comma1
	   || comma1->symbol != COMMA || !param2 || !comma2 ||
	   comma2->symbol != COMMA || !param3 || !rparen
	   || rparen->symbol != RPAREN) goto catch;
	type1 = t->from;
	if(!(a = strchr(type1, '_'))) goto catch;
	type1_size = (int)(a - type1);
	type2 = a + 1;
	if(!(a = strchr(type2, '_'))) goto catch;
	type2_size = (int)(a - type2);
	type3 = a + 1;
	if(!(a = strchr(type3, '_'))) goto catch;
	type3_size = (int)(a - type3);
	assert(t->length == a + 1 - t->from);
	printf("<%.*s>%.*s<%.*s>%.*s<%.*s>%.*s", type1_size, type1,
		param1->length, param1->from, type2_size, type2, param2->length,
		param2->from, type3_size, type3, param3->length, param3->from);
	*ptoken = TokenArrayNext(tokens, rparen);
	return 1;
	catch:
	fprintf(stderr, "Expected: A_B_C_(id,id,id); %s.\n", pos(t));
	return 0;
}
OUT(escape) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == ESCAPE && t->length == 2);
	printf("[\\%c]", t->from[1]);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(url) {
	const struct Token *const t = *ptoken;
	assert(tokens && t && t->symbol == URL);
	printf("<%.*s>", t->length, t->from);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
}
OUT(cite) {
	const struct Token *const t = *ptoken;
	static char url_encoded[64];
	size_t source = 0, target = 0;
	char ch;
	assert(tokens && t && t->symbol == CITE);
	while(target < sizeof url_encoded - 4 && source < (size_t)t->length) {
		ch = t->from[source++];
		if(!ch) goto catch;
		url_encoded[target] = ch;
	}
	url_encoded[target] = '\0';
	printf("(%.*s)<https://scholar.google.ca/scholar?q=%s>",
		t->length, t->from, url_encoded);
	*ptoken = TokenArrayNext(tokens, t);
	return 1;
catch:
	fprintf(stderr, "Expected: <source>; %s.\n", pos(t));
	return 0;
}
OUT(see_fn) {
	const struct Token *const fn = *ptoken;
	assert(tokens && fn && fn->symbol == SEE_FN);
	printf("(fixme)<fn:%.*s>", fn->length, fn->from);
	*ptoken = TokenArrayNext(tokens, fn);
	return 1;
}
OUT(see_tag) {
	const struct Token *const tag = *ptoken;
	assert(tokens && tag && tag->symbol == SEE_FN);
	printf("(fixme)<tag:%.*s>", tag->length, tag->from);
	*ptoken = TokenArrayNext(tokens, tag);
	return 1;
}
OUT(see_typedef) {
	const struct Token *const def = *ptoken;
	assert(tokens && def && def->symbol == SEE_FN);
	printf("(fixme)<typedef:%.*s>", def->length, def->from);
	*ptoken = TokenArrayNext(tokens, def);
	return 1;
}
OUT(see_data) {
	const struct Token *const data = *ptoken;
	assert(tokens && data && data->symbol == SEE_FN);
	printf("(fixme)<data:%.*s>", data->length, data->from);
	*ptoken = TokenArrayNext(tokens, data);
	return 1;
}
OUT(math) { /* Math and code. */
	const struct Token *const begin = *ptoken;
	struct Token *next = TokenArrayNext(tokens, begin);
	assert(tokens && begin && begin->symbol == MATH_BEGIN);
	printf("{code:`");
	while(next->symbol != MATH_END) {
		printf("%.*s", next->length, next->from);
		if(!(next = TokenArrayNext(tokens, next))) goto catch;
	}
	printf("`:code}");
	*ptoken = TokenArrayNext(tokens, next);
	return 1;
catch:
	fprintf(stderr, "Expected: `<math/code>`; %s.\n", pos(begin));
	return 0;
}
OUT(em) {
	const struct Token *const begin = *ptoken;
	struct Token *next = TokenArrayNext(tokens, begin);
	assert(tokens && begin && begin->symbol == EM_BEGIN);
	printf("{em:`");
	while(next->symbol != EM_END) {
		printf("%.*s", next->length, next->from);
		if(!(next = TokenArrayNext(tokens, next))) goto catch;
	}
	printf("`:em}");
	*ptoken = TokenArrayNext(tokens, next);
	return 1;
catch:
	fprintf(stderr, "Expected: _<emphasis>_; %s.\n", pos(begin));
	return 0;
}
OUT(link) {
	const struct Token *const t = *ptoken,
		*const desc = TokenArrayNext(tokens, t);
	assert(tokens && t && t->symbol == LINK);
	if(!desc || desc->symbol != LINK) goto catch;
	printf("[%.*s](%.*s)", desc->length, desc->from, t->length, t->from);
	*ptoken = TokenArrayNext(tokens, desc);
	return 1;
catch:
	fprintf(stderr, "Expected: `[description](url)`; %s.\n", pos(t));
	return 0;
}



/* `SYMBOL` is declared in `Scanner.h`. */
static const OutFn symbol_outs[] = { SYMBOL(PARAM5C) };



static void tokens_print(const struct TokenArray *const tokens) {
	const struct Token *token = TokenArrayNext(tokens, 0);
	OutFn sym_out;
	if(!token) return;
	/* fixme: This does not stop on error. */
	while(token) {
		sym_out = symbol_outs[token->symbol];
		if(!sym_out) {
			printf("<<%s fn undefined>>", symbols[token->symbol]);
			token = TokenArrayNext(tokens, token);
			continue;
		}
		assert(sym_out);
		whitespace_if_needed(token->symbol);
		if(!sym_out(tokens, &token)) { errno = EILSEQ; return /* fixme */; }
	}
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
		printf("Segment %s: %s;\n"
			   "code: %s;\n"
			   "params: %s;\n"
			   "doc: %s.\n",
			   segment->name,
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
	spaces_reset(&whitespace);
	while((segment = SegmentArrayNext(&report, segment))) {
		struct Attribute *attribute = 0;
		if(segment->division != DIV_PREAMBLE) continue;
		while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
		{
			if(attribute->symbol != symbol) continue;
			tokens_print(&attribute->contents);
		}
		spaces_force();
	}
}

static void preamble_print(void) {
	struct Segment *segment = 0;
	spaces_reset(&newline);
	while((segment = SegmentArrayNext(&report, segment))) {
		if(segment->division != DIV_PREAMBLE) continue;
		tokens_print(&segment->doc);
		spaces_force();
	}
}

/** Outputs a file when given a `SegmentArray`. */
void ReportOut(void) {
	/* Header. */
	if(preamble_attribute_exists(ATT_TITLE)) {
		printf("<preamble:title># ");
		preamble_attribute_print(ATT_TITLE);
		printf(" #\n\n");
	} else {
		printf("<no title>\n\n");
	}
	/* Preamble contents. */
	printf("<preamble:contents>");
	preamble_print();
	/*printf("<header:doc>\n");
	SegmentArrayIfEach(&report, &segment_is_header, &segment_print_doc);*/
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
