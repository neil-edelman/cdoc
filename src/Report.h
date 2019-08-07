/* Must include `stddef.h`, `Symbol.h`. */

struct Segment;

enum ReportSegmentWhere { WHERE_DOC, WHERE_CODE, WHERE_ATT_HEADER,
	WHERE_ATT_CONTENTS };

void Report_(void);
struct Segment *ReportNewSegment(void);
struct Attribute *ReportSegmentNewAttribute(struct Segment *const
	segment, const enum Symbol symbol);
int ReportSegmentIsCode(const struct Segment *const segment);
int ReportSegmentIsDoc(const struct Segment *const segment);
int ReportSegmentNewToken(struct Segment *const segment,
	const enum ReportSegmentWhere where, const enum Symbol symbol,
	const char *const from, const char *const to, const size_t line);
