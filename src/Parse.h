/*enum Namespace Semantic(const char *const marks);*/

void ParseTrace(FILE *TraceFILE, char *zTracePrompt);
void ParseInit(void *yypRawParser);
void *ParseAlloc(void *(*)());
void ParseFinalize(void *p);
void ParseFree(void *p, void (*freeProc)(void*));
void Parse(void *yyp, int yymajor, int yyminor);
int ParseFallback(int iToken);
