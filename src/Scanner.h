#include <stddef.h> /* size_t */
#include "Symbol.h" /* enum Symbol */

struct Scanner;

void Scanner_(struct Scanner *const scanner);
struct Scanner *Scanner(const char *fn, int (*const notify)(void));
int ScannerBrief(const char *string, int (*const notify)(void),
	void (*const print)(void));
enum Symbol ScannerSymbol(void);
const char *ScannerFrom(void);
const char *ScannerTo(void);
const char *ScannerFilename(void);
size_t ScannerLine(void);
int ScannerIndentLevel(void);
