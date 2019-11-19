#include "Division.h"
#include "Symbol.h"

struct TokenArray;

const char *TokensFirstFilename(const struct TokenArray *const tokens);
size_t TokensFirstLine(const struct TokenArray *const tokens);
size_t TokensMarkSize(const struct TokenArray *const tokens);
void TokensMark(const struct TokenArray *const tokens, char *const marks);

struct Token;

/*enum Symbol TokenSymbol(const struct Token *const token);*/

int ReportCurrentDivision(const enum Division division);
int ReportCurrentParam(const struct Token *const token);
void ReportCurrentReset(void);

void Report_(void);
void ReportDivision(const enum Division division);
void ReportLastSegmentDebug(void);
int ReportNotify(void);
void ReportCull(void);
void ReportWarn(void);
int ReportOut(void);
