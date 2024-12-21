/** @license 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT). */

#include "../src/symbol.h"
#include "../src/boxdoc.h"
#include "../src/scanner.h"
#include <stdio.h>  /* .printf */
#include <stdlib.h> /* malloc free */
#include <assert.h> /* assert */
#include <errno.h>  /* errno EILSEQ */


/* This defines `scanner_state`. */
/*!types:re2c*/


/** scanner reads a file and extracts semantic information. Valid to access
 only while underlying pointers do not change. */
struct scanner {
	/* `re2c` variables; these point directly into `buffer`. */
	const char *marker, *ctx_marker, *from, *cursor;
	/* Weird `c2re` stuff: these fields have to come after when >5? */
	const char *label, *buffer, *sub0, *sub1;
	enum scanner_state state;
	enum symbol symbol;
	int indent_level;
	int ignore_block;
	size_t line, doc_line;
};

/** Prints line info in a static buffer, (to be printed?) */
static const char *pos(const struct scanner *const scan) {
	/* fixme: THERE IS NOTHING LINKING THESE TO REAL VALUES, I just pasted. */
	static const char *scanner_state_str[] = {
		"yyccharacter",
		"yyccomment",
		"yycmacro_comment",
		"yycstring",
		"yycdoc",
		"yycem",
		"yycmath",
		"yyccode",
		"yycinclude",
		"yycmacro",
		"yycparam_item",
		"yycparam_more",
		"yycanchor",
		"yycpre",
		"yycparam_begin",
	};
	static char p[128];
	if(!scan) {
		sprintf(p, "No scanner loaded");
	} else {
		const int max_size = 32, from_len = (scan->from + max_size
			< scan->cursor) ? max_size : (int)(scan->cursor - scan->from);
		sprintf(p, "%.32s:%lu, %s \"%.*s\" -> %s", scan->label,
			(unsigned long)scan->line, symbols[scan->symbol], from_len,
			scan->from, scanner_state_str[scan->state]);
	}
	return p;
}

/*!re2c
re2c:yyfill:enable = 0;
re2c:define:YYCTYPE = char;
re2c:define:YYCURSOR = scan->cursor;
re2c:define:YYMARKER = scan->marker;
re2c:define:YYCTXMARKER = scan->ctx_marker;
re2c:define:YYCONDTYPE = 'scanner_state';
re2c:define:YYGETCONDITION = 'scan->state';
re2c:define:YYGETCONDITION:naked = 1;
re2c:define:YYSETCONDITION = 'scan->state = @@;';
re2c:define:YYSETCONDITION:naked = 1;
re2c:flags:tags = 1;

// Eof is marked by null when preparing files for lexing.
eof = "\x00";
whitespace = [ \t\v\f];
newline = "\n" | "\r" "\n"?;

nbthinsp = "\\,";
mathcalo = "\\O";
ctheta = "\\Theta";
comega = "\\Omega";
times = "\\times";
cdot = "\\cdot";
log = "\\log";
escapes = nbthinsp | mathcalo | ctheta | comega | times | cdot | log;

// Words can be composed of anything except:
// " \t\n\v\f\r" whitespace and newline;
// "*" potential end-of-comment;
// "`" or "_" math/em;
// "~" non-breaking space;
// "!" image start;
// "\\" escape;
// "@" for each-block-tag;
// "<>[]" for embedded things;
// "\x00" eof.
// Words in math/code, em, and inside brackets do not nest.
word_base = [^ \t\n\r\v\f*_`~!\\@<>[\]\x00];
escape_asterisk = "*";
escape_lone = "_" | "`" | "~";
escape_multi = "!" | "\\" | "@" | "<" | ">";
escape_brackets = "[" | "]";
escape_except_asterisk = escape_multi | escape_lone | escape_brackets;
escape_inside = escape_asterisk | escape_multi | escape_lone;
escape_outside = escape_asterisk | escape_multi | escape_brackets;
word        = word_base+ | escape_outside;
word_math   = (word_base | [_@<>[\]] )+ | escape_outside;
word_em     = (word_base | [`@<>[\]] )+ | escape_outside;
word_inside = (word_base | [_`@<>] )+ | escape_inside;
begin_doc = "/""*""*"+;
end_doc = "*"+"/";
art = whitespace* "*"? newline " *";
white_inside = whitespace | newline | art [^/[\]\x00];
all_inside = word_inside | "\\" escape_except_asterisk | white_inside
	| "\\*" [^/[\]\x00] | escapes;
qfile = [^"\t\n\r\v\f\x00]+;
include_comment = whitespace*
	"/""*""*"+ whitespace* "@include" whitespace* "*"+"/";
comment_break = "/" "*"+ "*" "/";
begin_comment = "/""*";
end_comment = "*""/";
cxx_comment = "//" [^\n\x00]*;
macro_start = "#" | "%:";
// char_type = "u8"|"u"|"U"|"L"; <- These get caught in id; don't care.
oct = "0" [0-7]*;
dec = [1-9][0-9]*;
hex = '0x' [0-9a-fA-F]+;
frc = [0-9]* "." [0-9]+ | [0-9]+ ".";
exp = 'e' [+-]? [0-9]+;
flt = (frc exp? | [0-9]+ exp) [fFlL]?;
number = (oct | dec | hex | flt) [uUlL]*;
operator = ":" | "::" | "?" | "+" | "-" | "*" | "/" | "%" | "^"
	| "xor" | "&" | "bitand" | "|" | "bitor" | "~" | "compl" | "!" | "not"
	| "<" | ">" | "==" | "+=" | "-=" | "%=" | "^=" | "xor_eq"
	| "&=" | "and_eq" | "|=" | "or_eq" | "<<" | ">>" | ">>=" | "<<="
	| "!=" | "not_eq" | "<=" | ">=" | "&&" | "and" | "||" | "or" | "++"
	| "--" | "." | "->";
assignment = "=";
// Extension (hack) for generic macros; if one names them this way, it will
// be documented nicely; the down side is, these are legal names for
// identifiers; will be confused if you name anything this way that IS an
// identifier. Don't do that.
generic = [A-Za-z]+ "_";
// Supports only C90 ids. That would be complicated. I suppose you could hack
// it to accept a super-set?
id = [a-zA-Z_][a-zA-Z_0-9]*;
macro_id = [A-Z_]+;
generic_id = (("<" [A-Za-z]+ ">")? id)+;
// <https://tools.ietf.org/html/rfc3986#appendix-B> and also
// " \t\n\v\f\r*<>()&\x00" disallowed because then it crashes / escapes comment
// / links [](). ("The special characters "$-_.+!*'()," and
// reserved characters used for their reserved purposes may be used unencoded
// within a URL," RFC 1738, so () is completely braindead.)
// '<' 3C, '>' 3E, '#' 23, '(' %28, ')' %29, '*' %2A, ' ' %20
uri_scheme = [^:/?# \t\n\v\f\r*<>()&\x00]+ ":";
uri_authority = "//" [^/?# \t\n\v\f\r*<>()&\x00]*;
uri_url = [^?# \t\n\v\f\r*<>()&\x00]+;
// It has to have a * or / in it or else it's just a word, which might be a
// uri, but mmm, really?
looks_like_uri_url = (
	("." "."? "/")* (
	([^?# \t\n\v\f\r*<>()&./\x00]* ("/" | ".") [^?# \t\n\v\f\r*<>()&./\x00]+)
	| ([^?# \t\n\v\f\r*<>()&./\x00]+ ("/" | ".") [^?# \t\n\v\f\r*<>()&./\x00]*)
	)+
) | (
	("." "."? "/")+ [^?# \t\n\v\f\r*<>()&\x00]*
);
//uri_query = ("?" [^# \t\n\v\f\r*<>()&\x00]*);
//uri_fragment = ("#" [^ \t\n\v\f\r*<>()&\x00]*);
uri_query = ("?" [^# \t\n\v\f\r\x00]*);
// "The characters slash ("/") and question mark ("?") are allowed to
// represent data within the fragment identifier."
// <https://tools.ietf.org/html/rfc3986>
uri_fragment = ("#" [^ \t\n\v\f\r/?\x00]*);
absolute_uri = uri_scheme uri_authority uri_url uri_query? uri_fragment?;
relative_uri = uri_scheme? uri_authority?
	looks_like_uri_url uri_query? uri_fragment?;
uri = absolute_uri | relative_uri | uri_fragment;
// Citation format. Not authoritative.
// Author is equivalent to <[A-Z][A-Za-z'`-]+>, except accepting all code
// points after ASCII. It supports accents, but includes stuff that it shoudn't.
cap_word = [^\x00-\x40\x5B-\x7F]
	[^\x00-\x26\x28-\x2C\x2E-\x40\x5B-\x5F\x7b-\x7f]*;
author = cap_word;
etalia = "et" whitespace? "al" "."?;
authorsep = ","? (whitespace? "and" whitespace | whitespace? "&" whitespace?
	| whitespace?);
sep = ","? whitespace?;
date = [1-3][0-9][0-9][0-9];
page = ","? whitespace? (("pp" | "p") "."?)? whitespace?
	[0-9]+ (whitespace? "-"{1,2} whitespace? [0-9]+)?;
citation = author (authorsep author)* (sep etalia)?
	(sep date) (sep cap_word)? (sep page)?;
// Lists.
list = " \\* ";
*/

/** Scans. */
static enum symbol scan_next(struct scanner *const scan) {
	const char *sub0, *sub1;
	/*!stags:re2c format = 'const char *@@;'; */

	assert(scan);
	/* fixme: warning: variable 'yyt1' may be uninitialized when used
	 here [-Wconditional-uninitialized] */
	yyt1 = scan->cursor;

	scan->sub0 = scan->sub1 = 0;
	scan->doc_line = scan->line;
reset:
	scan->from = scan->cursor;
scan:
/*!re2c
	// Oops, don't know how to deal with that.
	<*> * { return fprintf(stderr, "%s: unexpected state.\n", pos(scan)),
		errno = EILSEQ, END; }
	<comment, macro_comment, string, character> * { goto scan; }
	// Everything stops at EOF.
	<*> "\x00" {
		if(scan->indent_level) fprintf(stderr,
			"%s: unexpected EOF while %d deep.\n", pos(scan),
			scan->indent_level),
			errno = EILSEQ;
		return END;
	}
	// Return space.
	<doc, em, math> whitespace+ { return SPACE; }
	<code, include> whitespace+ { goto reset; }
	// Newlines are generally ignored but documentation counts for paragraphs.
	<*> newline {
		scan->line++;
		if(scan->state == yycdoc || scan->state == yycanchor)
			return NEWLINE;
		else if(scan->state == yycstring || scan->state == yyccharacter)
			return fprintf(stderr, "%s: string syntax.\n", pos(scan)),
			errno = EILSEQ, END;
		else if(scan->state == yycmacro) scan->state = yyccode;
		goto reset;
	}

	// Continuation.
	<string, character, macro, include> "\\" newline
		{ scan->line++; goto scan; }
	<string> "\"" { scan->state = yyccode; return CONSTANT; }
	<character> "'" { scan->state = yyccode; return CONSTANT; }
	<string, character> "\\". { goto scan; }
	<comment> end_comment :=> code
	<macro_comment> end_comment :=> macro
	// C++ comments we aren't concerned with.
	<code, macro> cxx_comment { goto reset; }
	// With flattening the stack, this is not actually worth
	// the effort; honestly, it's not going to matter.
	<macro> begin_doc / [^/] { return fprintf(stderr,
		"%s: documentation inside macro.\n", pos(scan)), errno = EILSEQ,
		END; }
	// Everything is ignored except comments.
	<macro> ([^\x00\n\r] \ [/\\])+ { goto scan; }
	<macro> [\\] / [^\n\r] { goto scan; }
	<macro> [/] / [^*] { goto scan; }
	<macro> begin_comment :=> macro_comment
	<code> comment_break { goto reset; } // Like this: / ***** /.
	<code> begin_comment :=> comment
	<code> whitespace* macro_start whitespace* "include" whitespace+
		"\"" @sub0 qfile @sub1 "\""
		whitespace* begin_doc whitespace* "\\include" whitespace* end_doc
		{ scan->sub0 = sub0, scan->sub1 = sub1;
		return scan->state = yycmacro, LOCAL_INCLUDE; }
	<code> macro_start :=> macro
	<code> "L"? "\"" :=> string
	<code> "'" :=> character
	<code> number       { return CONSTANT; }
	<code> operator     { return OPERATOR; }
	<code> assignment   { return ASSIGNMENT; }
	<code> generic/"("  { return ID_ONE_GENERIC; }
	<code> generic generic/"(" { return ID_TWO_GENERICS; }
	<code> generic generic generic/"(" { return ID_THREE_GENERICS; }
	<code> macro_id     { return MACRO; }
	<code> "struct"     { return STRUCT; }
	<code> "union"      { return UNION; }
	<code> "enum"       { return ENUM; }
	<code> "typedef"    { return TYPEDEF; }
	<code> "static"     { return STATIC; }
	<code> "void"       { return VOID; }
	<code> ("{" | "<%") { scan->indent_level++; return LBRACE; }
	<code> ("}" | "%>") { scan->indent_level--; return RBRACE; }
	<code> ("[" | "<:") { return LBRACK; }
	<code> ("]" | ":>") { return RBRACK; }
	<code> "("          { return LPAREN; }
	<code> ")"          { return RPAREN; }
	<code> ","          { return COMMA; }
	<code> ";"          { return SEMI; }
	<code> "..."        { return ELLIPSIS; }
	<code> id           { return ID; }

	<code> begin_doc / [^/] { return scan->state = yycdoc, DOC_BEGIN; }
	// Also newlines in comments should be optionally ascii-art.
	<doc, math, em, param_item, param_more> art / [^/] {
		scan->line++;
		if(scan->state == yycdoc) return NEWLINE;
		goto reset;
	}
	<doc, math, em, param_item, param_more, anchor> "\\*""/" { return
		fprintf(stderr, "%s: escape past end of documentation.\n",
		pos(scan)), errno = EILSEQ, END; }
	<math, em, param_item, param_more, anchor, pre> end_doc { return
		fprintf(stderr, "%s: unexpected end of documentation.\n",
		pos(scan)), errno = EILSEQ, END; }
	<doc> end_doc { return scan->state = yyccode, DOC_END; }

	<doc>  word      { return WORD; }
	<math> word_math { return WORD; }
	<em>   word_em   { return WORD; }
	<doc, math, em, anchor> "\\" escape_except_asterisk { return ESCAPE; }
	<doc, math, em, anchor> "\\*" / [^/] { return ESCAPE; }
	<doc, math, em, anchor> nbthinsp { return NBTHINSP; }
	<doc, math, em, anchor> mathcalo { return MATHCALO; }
	<doc, math, em, anchor> ctheta   { return CTHETA; }
	<doc, math, em, anchor> comega   { return COMEGA; }
	<doc, math, em, anchor> times    { return TIMES; }
	<doc, math, em, anchor> cdot     { return CDOT; }
	<doc, math, em, anchor> log      { return LOG; }
	<doc> "_" { return scan->state = yycem, EM_BEGIN; }
	<em> "_" { return scan->state = yycdoc, EM_END; }
	<doc> "`" { return scan->state = yycmath, MATH_BEGIN; }
	<math> "`" { return scan->state = yycdoc, MATH_END; }
	<doc> "~" { return NBSP; }
	<doc> list { return LIST_ITEM; }
	<doc> "\\\"" " "? / [^\n\r\x00] { scan->state = yycpre; goto reset; }
	<doc> "\\\"" " "? [\n\r\x00] { return
		fprintf(stderr, "%s: preformatted cannot be empty.\n", pos(scan)),
		errno = EILSEQ, END; }
	<pre> [^*\n\r\x00]+ | "*"+ / [\n\r\x00]
		{ return scan->state = yycdoc, PREFORMATTED; }
	<pre> [^*\n\r\x00]+ / "*" { goto scan; }
	<pre> "*"+ / [^/\n\r\x00] { goto scan; }

	// These are recognized in the documentation as stuff; parse them further
	// using `sub`.
	<doc> "<" @sub0 uri @sub1 ">"
		{ scan->sub0 = sub0, scan->sub1 = sub1; return URL; }
	<doc> "<" @sub0 citation @sub1 ">"
		{ scan->sub0 = sub0, scan->sub1 = sub1; return CITE; }
	<doc> "<fn:" @sub0 generic_id @sub1 ">"
		{ scan->sub0 = sub0, scan->sub1 = sub1; return SEE_FN; }
	<doc> "<tag:" @sub0 generic_id @sub1 ">"
		{ scan->sub0 = sub0, scan->sub1 = sub1; return SEE_TAG; }
	<doc> "<typedef:" @sub0 generic_id @sub1 ">"
		{ scan->sub0 = sub0, scan->sub1 = sub1; return SEE_TYPEDEF;}
	<doc> "<data:" @sub0 generic_id @sub1 ">"
		{ scan->sub0 = sub0, scan->sub1 = sub1; return SEE_DATA; }
	<doc> "[" whitespace* / all_inside* "](" uri ")"
		{ return scan->state = yycanchor, LINK_START; }
	<doc> "![" whitespace* / all_inside* "](" uri ")"
		{ return scan->state = yycanchor, IMAGE_START; }

	// These are tags.
	<doc> "@subtitle"   { return ATT_SUBTITLE; }
	<doc> "@param"      { return scan->state = yycparam_begin, ATT_PARAM; }
	<doc> "@author"     { return ATT_AUTHOR; }
	<doc> "@std"        { return ATT_STD; }
	<doc> "@depend"     { return ATT_DEPEND; }
	<doc> "@fixme"      { return ATT_FIXME; }
	<doc> "@return"     { return ATT_RETURN; }
	<doc> "@throws"     { return scan->state = yycparam_begin, ATT_THROWS; }
	<doc> "@implements" { return ATT_IMPLEMENTS; }
	<doc> "@order"      { return ATT_ORDER; }
	<doc> "@allow"      { return ATT_ALLOW; }
	<doc> "@license"    { return ATT_LICENSE; }
	<doc> "@cf"         { return ATT_CF; }
	<doc> "@abstract"   { return ATT_ABSTRACT; }

	// Parameter lists.
	<param_begin> "[" { return scan->state = yycparam_item, DOC_LEFT; }
	<param_item>  id  { return scan->state = yycparam_more, DOC_ID; }
	<param_more>  "," { return scan->state = yycparam_item, DOC_COMMA; }
	<param_more>  "]" { return scan->state = yycdoc,        DOC_RIGHT; }
	<param_item, param_more> whitespace+ { goto reset; }
	<param_begin, param_item, param_more> * {
		return fprintf(stderr, "%s: not allowed in parameter list.\n",
		pos(scan)), errno = EILSEQ, END; }

	// Link/image text. Ended by `URL`. MD []() is inconsistent notation. :[
	<anchor>  word_inside { return WORD; }
	<anchor>  whitespace+ { return SPACE; }
	<anchor>  whitespace* "](" @sub0 uri @sub1 ")"
		{ scan->sub0 = sub0, scan->sub1 = sub1;
		return scan->state = yycdoc, URL; }
*/
}

static enum scanner_state scanner_start_to_state(const enum scanner_start start)
{
	switch(start) {
	case START_CODE: return yyccode;
	case START_DOC:  return yycdoc;
	}
	assert(0);
	return yyccode;
}

static void zero_scanner(struct scanner *const scanner) {
	assert(scanner);
	scanner->marker = scanner->ctx_marker = scanner->from = scanner->cursor = 0;
	scanner->label = scanner->buffer = scanner->sub0 = scanner->sub1 = 0;
	scanner->state = yyccode; /* Generated by `re2c`. */
	scanner->symbol = END;
	scanner->indent_level = 0;
	scanner->ignore_block = 0;
	scanner->line = scanner->doc_line = 0;
}

/** Unloads scanner from memory. */
void scanner_(struct scanner **const pscanner) {
	struct scanner *scanner;
	if(!pscanner || !(scanner = *pscanner)) return;
	zero_scanner(scanner);
	free(scanner);
	*pscanner = 0;
}

/** Scans all the `buffer`.
 @param[label] The label of the scanner; must be valid throughout the scanners
 lifetime; if null, returns null.
 @param[buffer] The buffer of the scanner that it goes through; must be valid
 throughout the scanners lifetime; if null, returns null.
 @param[notify] The function that is notified when it gets a match. It
 interprets return of false for error; if null, returns null.
 @return The scanner which must be passed to <fn:scanner_>.
 @throws[malloc, fopen, fread]
 @throws[EILSEQ] File has embedded nulls. */
struct scanner *scanner(const char *const label, const char *const buffer,
	const scanner_predicate notify, const enum scanner_start start) {
	struct scanner *scan = 0;
	const enum scanner_state state = scanner_start_to_state(start);
	if(!label || !buffer || !notify) goto catch;
	if(!(scan = malloc(sizeof *scan))) goto catch;
	zero_scanner(scan);
	scan->label  = label;
	scan->buffer = buffer;
	/* Point these toward the first char; `buffer` is necessarily done
	 growing, or we could not do this. */
	scan->marker = scan->ctx_marker = scan->from = scan->cursor = buffer;
	scan->line = scan->doc_line = 1;
	scan->state = state;
	/* Scans all. */
	errno = 0;
	while((scan->symbol = scan_next(scan)) && notify(scan))
		if(cdoc_get_debug() & DBG_READ) fprintf(stderr, "%s.\n", pos(scan));
	if(errno) goto catch;
	if(scan->state != state) {
		fprintf(stderr, "%s: enexpected mode at end of buffer.\n",
		pos(scan)); errno = EILSEQ; goto catch; }
	goto finally;
catch:
	scanner_(&scan), scan = 0;
finally:
	return scan;
}

enum symbol scanner_symbol(const struct scanner *const scan) {
	if(!scan) return END;
	return scan->symbol;
}
const char *scanner_from(const struct scanner *const scan) {
	if(!scan) return 0;
	if(scan->sub0) return assert(scan->sub0 < scan->sub1), scan->sub0;
	else if(scan->from) return assert(scan->from < scan->cursor), scan->from;
	return 0;
}
const char *scanner_to(const struct scanner *const scan) {
	if(!scan) return 0;
	if(scan->sub1) return scan->sub1;
	else if(scan->cursor) return scan->cursor;
	return 0;
}
const char *scanner_label(const struct scanner *const scan) {
	return scan ? scan->label : 0;
}
size_t scanner_line(const struct scanner *const scan) {
	return scan ? scan->line : 0;
}
int scanner_indent_level(const struct scanner *const scan) {
	return scan ? scan->indent_level : 0;
}
