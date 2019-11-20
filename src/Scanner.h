#ifndef SCANNER_H /* <!-- scan */
#define SCANNER_H

#include <stddef.h> /* size_t */
#include "Symbol.h" /* enum Symbol */

struct Scanner;

typedef int (*ScannerPredicate)(const struct Scanner *);

void Scanner_(struct Scanner *const scanner);
struct Scanner *ScannerFile(const char *fn, const ScannerPredicate notify);
struct Scanner *ScannerString(const char *string,
	const ScannerPredicate notify);
enum Symbol ScannerSymbol(const struct Scanner *const scan);
const char *ScannerFrom(const struct Scanner *const scan);
const char *ScannerTo(const struct Scanner *const scan);
const char *ScannerFilename(const struct Scanner *const scan);
size_t ScannerLine(const struct Scanner *const scan);
int ScannerIndentLevel(const struct Scanner *const scan);

#endif /* scan --> */
