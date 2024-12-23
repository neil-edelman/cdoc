#ifndef attribute_h
#	define attribute_h
#	include "token_array.h"

/** Representing each-attributes, like "\@param ...". */
struct attribute {
	struct token token;
	struct token_array header, contents;
};
#endif
