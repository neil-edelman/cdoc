struct Token;
struct TokenArray;

size_t TokensSize(const struct TokenArray *const ta);
struct Token *TokensNext(const struct TokenArray *const a,
	struct Token *const here);
enum Symbol TokenSymbol(const struct Token *const token);

