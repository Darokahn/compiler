#include <unistd.h>

#include "token.h"
#include "table/table.h"
#include "stateMachine.h"
#include "scanner.h"
#include "symbols.h"

void scanAndPrint(char* filename, symbols_t* symbols) {
    scanner_t s;
    scanner_init(&s, filename, symbols);
    token_t t;
    t.type = 0;
    while (t.type != eof_token) {
        t = scanner_getNextToken(&s);
        if (t.type == bad_token) break;
        token_prettyPrint(&t);
    }
}

void scanAndPrintDebug(char* filename, symbols_t* symbols) {
    scanner_t s;
    scanner_init(&s, filename, symbols);
    token_t t;
    t.type = 0;
    printf("%-3d", s.lineCount);
    while (t.type != eof_token) {
        t = scanner_getNextToken(&s);
        if (t.type == bad_token) break;
        if (t.type == whitespace_token) {
            token_prettyPrint(&t);
        }
        else if (t.type == newline_token) {
            token_prettyPrint(&t);
            printf("%-3d", s.lineCount);
        }
        else token_debugPrettyPrint(&t);
    }
}

void initSymbols(symbols_t* t) {
    symbols_init(t, NAMESPACESIZE);
    for (int i = 0; i < LASTKEYWORD; i++) {
        symbol_t sym = {
            .type = keyword_symbol
        };
        symbols_add(t, keywordStrings[i], strlen(keywordStrings[i]), sym);
    }
    for (int i = 0; i < LASTTYPE; i++) {
        symbol_t sym = {
            .type = type_symbol
        };
        symbols_add(t, builtinTypeStrings[i], strlen(builtinTypeStrings[i]), sym);
    }
    symbols_debug(t);
}

int main() {
    symbols_t symbols;
    initSymbols(&symbols);
    scanAndPrint("main.c", &symbols);
    scanAndPrintDebug("test.c", &symbols);
    printf("\n");
}
