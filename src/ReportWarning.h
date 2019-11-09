/* Warn if:
 -* Empty (or @allow full) attributes,
 -* Don't match the varibles.
 -* Haven't documented all the variables.
 */

static int attribute_use(const struct Attribute *const attribute,
	const int is_header, const int is_contents) {
	return is_header ^ !!TokenArraySize(&attribute->header)
		|| is_contents ^ !!TokenArraySize(&attribute->contents) ? 0 : 1;
}

static int attribute_okay(const struct Attribute *const attribute) {
	assert(attribute);
	switch(attribute->token.symbol) {
		case ATT_PARAM: /* `Scanner.c.re_c` has a state change. */
		case ATT_THROWS: return attribute_use(attribute, 1, 1);
		case ATT_TITLE: /* Otherwise, warn if empty. */
		case ATT_AUTHOR:
		case ATT_STD:
		case ATT_DEPEND:
		case ATT_FIXME:
		case ATT_RETURN:
		case ATT_IMPLEMENTS:
		case ATT_ORDER:
		case ATT_LICENSE: return attribute_use(attribute, 0, 1);
		case ATT_ALLOW: return attribute_use(attribute, 0, 0); /* Or full. */
		default: assert((fprintf(stderr, "Not recognised.\n"), 0)); return 0;
	}
}

/** Searches for `match` in the `params` supplied by the parser. */
static int match_function_params(const struct Token *const match,
	const struct Segment *const segment) {
	size_t no = 0;
	const struct Token *param;
	assert(match && segment);
	while((param = param_no(segment, no++)))
		if(!token_compare(match, param)) return 1;
	return 0;
}

/** Searches for `match` in the param header of `attributes`
 (eg, `@param[here, var]`) of the documentation. */
static int match_param_attributes(const struct Token *const match,
	const struct AttributeArray *const attributes) {
	struct Attribute *attribute = 0;
	assert(match && attributes);
	while((attribute = AttributeArrayNext(attributes, attribute))) {
		struct Token *param = 0;
		if(attribute->token.symbol != ATT_PARAM) continue;
		while((param = TokenArrayNext(&attribute->header, param)))
			if(!token_compare(match, param)) return 1;
	}
	return 0;
}

/** Searches for `match` inside of `tokens` surrounded by math/code blocks. */
static int match_tokens(const struct Token *const match,
	const struct TokenArray *const tokens) {
	struct Token *t0 = 0, *t1, *t2;
	assert(match && tokens);
	while((t0 = TokenArrayNext(tokens, t0))) {
		if(t0->symbol != MATH_BEGIN) continue;
		if(!(t1 = TokenArrayNext(tokens, t0))) break;
		if(t1->symbol != WORD) { t0 = t1; continue; }
		if(!(t2 = TokenArrayNext(tokens, t1))) break;
		/* Sic.; could have "``". */
		if(t2->symbol != MATH_END) { t0 = t1; continue; }
		if(!token_compare(match, t1)) return 1;
	}
	return 0;
}

/** Searches for `match` inside all `attributes` of type `symbol` contents'. */
static int match_attribute_contents(const struct Token *const match,
	const struct AttributeArray *const attributes, const enum Symbol symbol) {
	struct Attribute *attribute = 0;
	assert(match && attributes && symbol);
	while((attribute = AttributeArrayNext(attributes, attribute))) {
		if(attribute->token.symbol != symbol) continue;
		if(match_tokens(match, &attribute->contents)) return 1;
	}
	return 0;
}

static void unused_attribute(const struct Segment *const segment,
	const enum Symbol symbol) {
	const struct AttributeArray *const attributes = &segment->attributes;
	struct Attribute *attribute = 0;
	char a[12], b[12];
	assert(segment && attributes && symbol);
	segment_to_string(segment, &a);
	while((attribute = AttributeArrayNext(attributes, attribute))) {
		if(attribute->token.symbol != symbol) continue;
		attribute_to_string(attribute, &b);
		fprintf(stderr, "%s: %s attribute not used in %s.\n",
			pos(&attribute->token), b, a);
	}
}

static void warn_internal_link(const struct Token *const token) {
	enum Division division;
	char a[256], b[256];
	size_t *fun_index;
	struct Segment *segment = 0;
	assert(token);
	switch(token->symbol) {
		case SEE_FN:      division = DIV_FUNCTION; break;
		case SEE_TAG:     division = DIV_TAG;      break;
		case SEE_TYPEDEF: division = DIV_TYPEDEF;  break;
		case SEE_DATA:    division = DIV_DATA;     break;
		default: return;
	}
	/* Encode the link text. */
	encode_s(token->length, token->from, &a);
	/* Search for it. fixme: this builds up labels from scratch, then discards
	 them, over and over. */
	while((segment = SegmentArrayNext(&report, segment))) {
		const struct Token *compare;
		if(segment->division != division
			|| !(fun_index = IndexArrayNext(&segment->code_params, 0))
			|| *fun_index >= TokenArraySize(&segment->code)) continue;
		compare = TokenArrayGet(&segment->code) + *fun_index;
		/* fixme: returns false error at end of tokens! */
		if(!print_token_s(&segment->code, compare, &b))
			{ fprintf(stderr,
			"%s: length was too long or end of tokens to compare with %s.\n",
			pos(token), a); continue; }
		if(!strcmp(a, b))
			{ /*fprintf(stderr, "%s: link okay.\n", pos(token));*/ return; }
	}
	fprintf(stderr, "%s: link broken.\n", pos(token));
}

/** @fixme Should be like Report. */
static void warn_segment(const struct Segment *const segment) {
	struct Attribute *attribute = 0;
	const size_t *code_param;
	const struct Token *const fallback = segment_fallback(segment);
	struct Token *token = 0;
	assert(segment);
	/* Check for empty (or full, as the case may be) attributes. */
	while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
		if(!attribute_okay(attribute)) fprintf(stderr,
		"%s: attribute not used correctly.\n", pos(&attribute->token));
	/* Check all text for undefined references. */
	while((token = TokenArrayNext(&segment->doc, token)))
		warn_internal_link(token);
	while((attribute = AttributeArrayNext(&segment->attributes, attribute))) {
		while((token = TokenArrayNext(&attribute->header, token)))
			warn_internal_link(token);
		while((token = TokenArrayNext(&attribute->contents, token)))
			warn_internal_link(token);
	}
	/* Check for different things depending on the division. */
	switch(segment->division) {
	case DIV_FUNCTION:
		/* Check for code. This one will never be triggered unless one fiddles
		 with the parser. */
		if(!TokenArraySize(&segment->code))
			fprintf(stderr, "%s: function with no code?\n", pos(fallback));
		/* Check for public methods without documentation. */
		if(!TokenArraySize(&segment->doc)
			&& !AttributeArraySize(&segment->attributes)
			&& !is_static(&segment->code))
			fprintf(stderr, "%s: no documentation.\n", pos(fallback));
		/* No function title? */
		if(IndexArraySize(&segment->code_params) < 1) fprintf(stderr,
			"%s: unable to extract function name.\n", pos(fallback));
		/* Unused in function. */
		unused_attribute(segment, ATT_TITLE);
		if(!is_static(&segment->code))
			unused_attribute(segment, ATT_ALLOW);
		/* Check for extraneous params. */
		attribute = 0;
		while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
		{
			struct Token *match = 0;
			if(attribute->token.symbol != ATT_PARAM) continue;
			while((match = TokenArrayNext(&attribute->header, match)))
				if(!match_function_params(match, segment))
				fprintf(stderr, "%s: extraneous parameter.\n", pos(match));
		}
		/* Check for params that are undocumented. */
		code_param = IndexArrayNext(&segment->code_params, 0);
		while((code_param = IndexArrayNext(&segment->code_params, code_param))) {
			const struct Token *param = TokenArrayGet(&segment->code)
				+ *code_param;
			assert(*code_param <= TokenArraySize(&segment->code));
			if(!match_param_attributes(param, &segment->attributes)
				&& !match_tokens(param, &segment->doc)
				&& !match_attribute_contents(param, &segment->attributes,
				ATT_RETURN)) fprintf(stderr,
				"%s: parameter may be undocumented.\n", pos(param));
		}
		break;
	case DIV_PREAMBLE:
		/* Should not have params. */
		if(IndexArraySize(&segment->code_params)) fprintf(stderr,
			"%s: params useless in preamble.\n", pos(fallback));
		/* Unused in preamble. */
		unused_attribute(segment, ATT_RETURN);
		unused_attribute(segment, ATT_THROWS);
		unused_attribute(segment, ATT_IMPLEMENTS);
		unused_attribute(segment, ATT_ORDER);
		unused_attribute(segment, ATT_ALLOW);
		break;
	case DIV_TAG:
		/* Should have one. */
		if(IndexArraySize(&segment->code_params) != 1) fprintf(stderr,
			"%s: unable to extract one tag name.\n", pos(fallback));
		/* Unused in tags. */
		unused_attribute(segment, ATT_TITLE);
		unused_attribute(segment, ATT_RETURN);
		unused_attribute(segment, ATT_THROWS);
		unused_attribute(segment, ATT_IMPLEMENTS);
		unused_attribute(segment, ATT_ORDER);
		if(!is_static(&segment->code))
			unused_attribute(segment, ATT_ALLOW);
		break;
	case DIV_TYPEDEF:
		/* Should have one. */
		if(IndexArraySize(&segment->code_params) != 1) fprintf(stderr,
			"%s: unable to extract one typedef name.\n", pos(fallback));
		/* Unused in typedefs. */
		unused_attribute(segment, ATT_TITLE);
		unused_attribute(segment, ATT_PARAM);
		unused_attribute(segment, ATT_RETURN);
		unused_attribute(segment, ATT_THROWS);
		unused_attribute(segment, ATT_IMPLEMENTS);
		unused_attribute(segment, ATT_ORDER);
		unused_attribute(segment, ATT_ALLOW);
		break;
	case DIV_DATA:
		/* Should have one. */
		if(IndexArraySize(&segment->code_params) != 1) fprintf(stderr,
			"%s: unable to extract one data name.\n", pos(fallback));
		/* Unused in data. */
		unused_attribute(segment, ATT_TITLE);
		unused_attribute(segment, ATT_PARAM);
		unused_attribute(segment, ATT_RETURN);
		unused_attribute(segment, ATT_THROWS);
		unused_attribute(segment, ATT_IMPLEMENTS);
		unused_attribute(segment, ATT_ORDER);
		if(!is_static(&segment->code))
			unused_attribute(segment, ATT_ALLOW);
		break;
	default:
		assert((fprintf(stderr, "Can not happen.\n"), 0));
	}
}

void ReportWarn(void) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment)))
		warn_segment(segment);
	/* fixme: warn if segments with the same data have the same name?
	 It would be better to merge them into a hash. */
}
