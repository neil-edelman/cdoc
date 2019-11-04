#include <stddef.h> /* size_t */
#include "Symbol.h" /* enum Symbol */

struct Scanner;

void Scanner_(struct Scanner *const scanner);
struct Scanner *Scanner(const char *fn, int (*const notify)(void));
enum Symbol ScannerSymbol(void);
const char *ScannerFrom(void);
const char *ScannerTo(void);
size_t ScannerLine(void);
int ScannerIndentLevel(void);
