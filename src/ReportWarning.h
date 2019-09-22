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
		case ATT_VERSION:
		case ATT_FIXME:
		case ATT_RETURN:
		case ATT_IMPLEMENTS:
		case ATT_ORDER: return attribute_use(attribute, 0, 1);
		case ATT_ALLOW: return attribute_use(attribute, 0, 0); /* Or full. */
		default: return 0;
	}
}

/** Searches for `match` in the `params` supplied by the parser. */
static int match_function_params(const struct Token *const match,
	const struct TokenRefArray *const params) {
	struct Token **param = TokenRefArrayNext(params, 0); /* The name. */
	char a[12];
	assert(match && params);
	token_to_string(match, &a);
	while((param = TokenRefArrayNext(params, param)))
		if(!token_compare(match, *param)) return 1;
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

static void unused_attribute(const struct AttributeArray *const attributes,
	const enum Symbol symbol) {
	struct Attribute *attribute = 0;
	assert(attributes && symbol);
	while((attribute = AttributeArrayNext(attributes, attribute))) {
		if(attribute->token.symbol != symbol) continue;
		fprintf(stderr, "%s: attribute not used.\n", pos(&attribute->token));
	}
}

static void warn_segment(const struct Segment *const segment) {
	const size_t doc_size = TokenArraySize(&segment->doc),
		code_size = TokenArraySize(&segment->code),
		attribute_size = AttributeArraySize(&segment->attributes);
	struct Attribute *attribute;
	struct Token **param;
	const struct Token *const fallback = TokenRefArraySize(&segment->params)
		? *TokenRefArrayGet(&segment->params) : code_size
		? TokenArrayGet(&segment->code) : doc_size
		? TokenArrayGet(&segment->doc) : attribute_size
		? &AttributeArrayGet(&segment->attributes)->token : 0;
	assert(segment);
	/* Check for empty (or full, as the case may be) attributes. */
	attribute = 0;
	while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
		if(!attribute_okay(attribute)) fprintf(stderr,
		"%s: attribute not used correctly.\n", pos(&attribute->token));
	/* Check for different things depending on the division. */
	switch(segment->division) {
	case DIV_FUNCTION:
		/* Check for code. This one will never be triggered unless one fiddles
		 with the parser. */
		if(!code_size)
			fprintf(stderr, "%s: function with no code?\n", pos(fallback));
		/* Check for public methods without documentation. */
		if(!doc_size && !attribute_size && !is_static(&segment->code))
			fprintf(stderr, "%s: no documentation.\n", pos(fallback));
		/* No function title? */
		if(TokenRefArraySize(&segment->params) < 1) fprintf(stderr,
			"%s: unable to extract function name.\n", pos(fallback));
		/* Unused in function. */
		unused_attribute(&segment->attributes, ATT_TITLE);
		if(!is_static(&segment->code))
			unused_attribute(&segment->attributes, ATT_ALLOW);
		/* Check for extraneous params. */
		attribute = 0;
		while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
		{
			struct Token *match = 0;
			if(attribute->token.symbol != ATT_PARAM) continue;
			while((match = TokenArrayNext(&attribute->header, match)))
				if(!match_function_params(match, &segment->params))
				fprintf(stderr, "%s: extraneous parameter.\n", pos(match));
		}
		/* Check for params that are undocumented. */
		param = TokenRefArrayNext(&segment->params, 0);
		while((param = TokenRefArrayNext(&segment->params, param)))
			if(!match_param_attributes(*param, &segment->attributes)
			&& !match_tokens(*param, &segment->doc)
			&& !match_attribute_contents(*param, &segment->attributes,
			ATT_RETURN)) fprintf(stderr,
			"%s: parameter may be undocumented.\n", pos(*param));
		break;
	case DIV_PREAMBLE:
		/* Should not have params. */
		if(TokenRefArraySize(&segment->params)) fprintf(stderr,
			"%s: params useless in preamble.\n", pos(fallback));
		/* Unused in preamble. */
		unused_attribute(&segment->attributes, ATT_RETURN);
		unused_attribute(&segment->attributes, ATT_THROWS);
		unused_attribute(&segment->attributes, ATT_IMPLEMENTS);
		unused_attribute(&segment->attributes, ATT_ALLOW);
		break;
	case DIV_TAG:
		/* Should have one. */
		if(TokenRefArraySize(&segment->params) != 1) fprintf(stderr,
			"%s: unable to extract one tag name.\n", pos(fallback));
		/* Unused in tags. */
		unused_attribute(&segment->attributes, ATT_TITLE);
		unused_attribute(&segment->attributes, ATT_RETURN);
		unused_attribute(&segment->attributes, ATT_THROWS);
		unused_attribute(&segment->attributes, ATT_IMPLEMENTS);
		unused_attribute(&segment->attributes, ATT_ORDER);
		if(!is_static(&segment->code))
			unused_attribute(&segment->attributes, ATT_ALLOW);
		break;
	case DIV_TYPEDEF:
		/* Should have one. */
		if(TokenRefArraySize(&segment->params) != 1) fprintf(stderr,
			"%s: unable to extract one typedef name.\n", pos(fallback));
		/* Unused in typedefs. */
		unused_attribute(&segment->attributes, ATT_TITLE);
		unused_attribute(&segment->attributes, ATT_PARAM);
		unused_attribute(&segment->attributes, ATT_RETURN);
		unused_attribute(&segment->attributes, ATT_THROWS);
		unused_attribute(&segment->attributes, ATT_IMPLEMENTS);
		unused_attribute(&segment->attributes, ATT_ORDER);
		unused_attribute(&segment->attributes, ATT_ALLOW);
		break;
	case DIV_DATA:
		/* Should have one. */
		if(TokenRefArraySize(&segment->params) != 1) fprintf(stderr,
			"%s: unable to extract one data name.\n", pos(fallback));
		/* Unused in data. */
		unused_attribute(&segment->attributes, ATT_TITLE);
		unused_attribute(&segment->attributes, ATT_PARAM);
		unused_attribute(&segment->attributes, ATT_RETURN);
		unused_attribute(&segment->attributes, ATT_THROWS);
		unused_attribute(&segment->attributes, ATT_IMPLEMENTS);
		unused_attribute(&segment->attributes, ATT_ORDER);
		if(!is_static(&segment->code))
			unused_attribute(&segment->attributes, ATT_ALLOW);
		break;
	}
}

void ReportWarn(void) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment)))
		warn_segment(segment);
	/* fixme: warn if segments with the same data have the same name? */
}
