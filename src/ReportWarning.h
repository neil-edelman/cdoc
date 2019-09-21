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

static void warn_segment(const struct Segment *const segment) {
	const size_t doc_size = TokenArraySize(&segment->doc),
		code_size = TokenArraySize(&segment->code);
	struct Attribute *attribute = 0;
	/* Check for empty (or full, as the case may be) attributes. */
	while((attribute = AttributeArrayNext(&segment->attributes, attribute)))
		if(!attribute_okay(attribute))
		fprintf(stderr, "%s: attribute not okay.\n", pos(&attribute->token));
	switch(segment->division) {
	case DIV_FUNCTION:
			
	case DIV_PREAMBLE:
	case DIV_TAG:
	case DIV_TYPEDEF:
	case DIV_DATA:
		/* It's nothing. */
		if(!TokenArraySize(&segment->doc)
			&& !AttributeArraySize(&segment->attributes)) return;
	}
}

void ReportWarn(void) {
	struct Segment *segment = 0;
	while((segment = SegmentArrayNext(&report, segment)))
		warn_segment(segment);
}
