#ifndef SCANNER_H /* <!-- scan */
#define SCANNER_H

#include <stddef.h> /* size_t */
#include "Symbol.h" /* enum Symbol */

/** Where the beginning and the ending states will be. */
enum ScannerState { SSCODE, SSDOC };

struct Scanner;

typedef int (*ScannerPredicate)(const struct Scanner *);

void Scanner_(struct Scanner **const scanner);
struct Scanner *Scanner(const char *const label, const char *const buffer,
	const ScannerPredicate notify, const enum ScannerState state);
enum Symbol ScannerSymbol(const struct Scanner *const scan);
const char *ScannerFrom(const struct Scanner *const scan);
const char *ScannerTo(const struct Scanner *const scan);
const char *ScannerLabel(const struct Scanner *const scan);
size_t ScannerLine(const struct Scanner *const scan);
int ScannerIndentLevel(const struct Scanner *const scan);

#endif /* scan --> */
