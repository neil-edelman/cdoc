#include <stddef.h> /* size_t */
#include "Symbol.h" /* enum Symbol */

void Scanner_(void);
int Scanner(void);
enum Symbol ScannerSymbol(void);
const char *ScannerFrom(void);
const char *ScannerTo(void);
size_t ScannerLine(void);
int ScannerIndentLevel(void);
