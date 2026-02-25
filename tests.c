#include <stdarg.h>

#include "token.h"
#include "table/table.h"
#include "stateMachine.h"
#include "scanner.h"
#include "symbols.h"
#include "nodes.h"
#include "stringStreaming.h"

char* error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char* newError;
    vasprintf(&newError, fmt, args);
    va_end(args);
    return newError;
}

void scanAndPrint(char* filename, symbols_t* symbols, void* od, outfunc of) {
    scanner_t s;
    scanner_init(&s, filename, symbols);
    token_t t;
    t.type = 0;
    while (t.type != eof_token) {
        t = scanner_getNextToken(&s);
        if (t.type == bad_token) break;
        char line[1024];
        token_sprettyPrint(&t, line, sizeof line);
        of(od, "%s", line);
    }
}

void scanAndPrintDebug(char* filename, symbols_t* symbols, void* od, outfunc of) {
    scanner_t s;
    scanner_init(&s, filename, symbols);
    token_t t;
    t.type = 0;
    of(od, "%-3d", s.lineCount);
    char line[1024];
    while (t.type != eof_token) {
        t = scanner_getNextToken(&s);
        if (t.type == bad_token) break;
        if (t.type == whitespace_token) {
            token_sprettyPrint(&t, line, sizeof line);
            of(od, "%s", line);
        }
        else if (t.type == newline_token) {
            token_sprettyPrint(&t, line, sizeof line);
            of(od, "%s", line);
            of(od, "%-3d", s.lineCount);
        }
        else {
            token_sdebugPrettyPrint(&t, line, sizeof line);
            of(od, "%s", line);
        }
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
}

char* test_0(void* od, outfunc of) {
    char* msg = NULL;
    symbols_t symbols;
    initSymbols(&symbols);
    symbols_adebug(&symbols, od, (void*)of, "\t", "\n");
    symbol_t* sym = symbols_lookup(&symbols, "while", sizeof "while" - 1);
    if (sym->type != keyword_symbol) {
        msg = error("\"%s\" not properly placed in symbol table", "while");
        goto cleanup;
    }
cleanup:
    symbols_destroy(&symbols);
    return msg;
}

char* test_1(void* od, outfunc of) {
    symbols_t symbols;
    initSymbols(&symbols);
    scanAndPrint("main.c", &symbols, od, of);
    symbols_destroy(&symbols);
    return NULL;
}

char* test_2(void* od, outfunc of) {
    symbols_t symbols;
    initSymbols(&symbols);
    scanAndPrintDebug("testfiles/test.c", &symbols, od, of);
    symbols_destroy(&symbols);
    return NULL;
}

typedef struct {
    char* base;
    char* end;
} textCursor;

int tc_getChar(textCursor* c) {
    int ch;
    if (c->base >= c->end) ch = EOF;
    else ch = c->base[0];
    c->base++;
    return ch;
}

char* test_3(void* od, outfunc of) {
    char* msg = NULL;
    char lex[] = "while + 3";
    textCursor c = {lex, lex + sizeof lex - 1};
    enum tokenType tokType;
    int lexemeLen = stateMachine_getToken(&c, tc_getChar, &tokType);
    int targetLen = 5;
    of(od, "tokenization tests for '%s'\n", lex);
    if (lexemeLen != 5) {
        msg =  error("lexeme length %d does not match %d for '%s'", lexemeLen, targetLen, lex);
        goto cleanup;
    }
    of(od, "length is correct\n");
    if (tokType != identifier_token) {
        msg = error("token type %d does not match identifier (%d) for %s", tokType, identifier_token, lex);
        goto cleanup;
    }
    of(od, "token type is correct\n");
    symbols_t symbols;
    initSymbols(&symbols);
    token_t tok;
    token_init(&tok, lex, lexemeLen, tokType, &symbols);
    if (tok.symbolType != keyword_symbol) {
        msg = error("token symbol type %d does not match keyword (%d) for '%s'", tok.symbolType, keyword_symbol, lex);
        goto cleanup;
    }
    of(od, "symbol type is correct\n");
cleanup:
    symbols_destroy(&symbols);
    return msg;
}

char* test_4(void* od, outfunc of) {
    of(od, "Testing parse tree resize\n");
    nodeBase b;
    nodeBase_init(&b, 1);
    node n = {.size = sizeof n};
    nodeBase_add(&b, &n);
    nodeBase_add(&b, &n);
    nodeBase_add(&b, &n);
    of(od, "No segfault, we're good\n");
    nodeBase_destroy(&b);
    return NULL;
}

char* test_5(void* od, outfunc of) {
    char* msg = NULL;
    of(od, "Testing parse tree with the expression '(5 + 10) * 10' manually inserted\n");
    nodeBase b;
    nodeBase_init(&b, 1);

    timesOperatorNode times;
    timesOperatorNode_init(&times);
    plusOperatorNode plus;
    plusOperatorNode_init(&plus);
    nodeBase_add(&b, (node*)&times);
    int timesIndex = 0;
    int plusIndex = nodeBase_addChild(&b, timesIndex, (node*)&plus);
    integerNode i;
    integerNode_init(&i, 5);
    nodeBase_addChild(&b, plusIndex, (node*)&i);
    integerNode_init(&i, 10);
    nodeBase_addChild(&b, plusIndex, (node*)&i);
    nodeBase_addChild(&b, timesIndex, (node*)&i);
    expressionNode* n = (expressionNode*) node_from(b.base, 0);
    int result = n->eval(n);
    if (result != 150) {
        msg = error("result %d is not 150\n", result);
        goto cleanup;
    }
    of(od, "result %d is correct\n", result);
cleanup:
    nodeBase_destroy(&b);
    return msg;
}

char* test_6(void* od, outfunc of) {
    char* msg = NULL;
    of(od, "Testing parse tree with only one identifier\n");

    nodeBase b;
    nodeBase_init(&b, 1);

    symbols_t symbols;
    initSymbols(&symbols);

    int symbolValue = 1;

    symbol_t x = {
        .name = "x",
        .nameLen = 1,
        .type = variable_symbol,
        .v = symbolValue,
    };

    symbols_add(&symbols, "x", 1, x);

    identifierNode i;
    identifierNode_init(&i, "x", 1, &symbols);
    nodeBase_add(&b, (node*)&i);
    expressionNode* n = (expressionNode*) node_from(b.base, 0);
    int result = n->eval(n);
    if (result != symbolValue) {
        msg = error("result %d is not %d\n", result, symbolValue);
        goto cleanup;
    }
    of(od, "result %d is correct\n", result);
cleanup:
    nodeBase_destroy(&b);
    symbols_destroy(&symbols);
    return msg;
}

char* test_7(void* od, outfunc of) {
    char* msg = NULL;

    of(od, "Testing parse tree with an addition between two identifiers\n");

    nodeBase b;
    nodeBase_init(&b, 1);

    symbols_t symbols;
    initSymbols(&symbols);

    int xValue = 6;
    int yValue = 8;

    symbol_t x = {
        .name = "x",
        .nameLen = 1,
        .type = variable_symbol,
        .v = xValue,
    };

    symbol_t y = {
        .name = "y",
        .nameLen = 1,
        .type = variable_symbol,
        .v = yValue,
    };

    symbols_add(&symbols, x.name, x.nameLen, x);
    symbols_add(&symbols, y.name, y.nameLen, y);

    plusOperatorNode p;
    plusOperatorNode_init(&p);

    nodeBase_add(&b, (node*)&p);

    int plusIndex = 0;

    identifierNode i;
    identifierNode_init(&i, x.name, x.nameLen, &symbols);

    nodeBase_addChild(&b, plusIndex, (node*)&i);

    identifierNode_init(&i, y.name, y.nameLen, &symbols);
    nodeBase_addChild(&b, plusIndex, (node*)&i);

    expressionNode* n = (expressionNode*) node_from(b.base, 0);
    int expectedResult = y.v.value + x.v.value;

    int result = n->eval(n);

    if (result != expectedResult) {
        msg = error("result %d is not %d\n", result, expectedResult);
        goto cleanup;
    }
    of(od, "result %d is correct\n", result);

cleanup:
    symbols_destroy(&symbols);
    nodeBase_destroy(&b);
    return msg;
}

char* test_8(void* od, outfunc of) {
    char* msg = NULL;
    char* hstring;
    of(od, "Testing heapstring initialization\n");
    int initialSize = 30;
    heapstring_init(&hstring, initialSize);
    char* base = heapstring_getBase(hstring);
    int remaining = heapstring_getRemaining(hstring);
    char* expectedBase = hstring;
    of(od, "base: %p, remaining: %d ", base, remaining);
    if (base != expectedBase) {
        msg = error("base %p does not match %p\n", base, expectedBase);
        goto cleanup;
    }
    if (remaining != initialSize) {
        msg = error("remaining %d does not match %d\n", remaining, initialSize);
    }
    of(od, "is correct\n");
cleanup:
    free(heapstring_getBase(hstring));
    return msg;
}

char* test_9(void* od, outfunc of) {
    char* msg = NULL;
    char* hstring;
    of(od, "Testing heapstring streaming\n");
    int initialSize = 16;
    heapstring_init(&hstring, initialSize);

    char* testString = "hello!";

    of(od, "writing %s to string.\n", testString);
    heapstring_stream(&hstring, testString);

    char* base = heapstring_getBase(hstring);

    if (strcmp(base, testString) != 0) {
        msg = error("string %s does not match %s\n", hstring, testString);
        goto cleanup;
    }
    of(od, "%s correctly matches %s\n", base, testString);
cleanup:
    free(heapstring_getBase(hstring));
    return msg;
}

char* test_10(void* od, outfunc of) {
    char* msg = NULL;
    char* hstring;
    of(od, "Testing heapstring streaming, lots of text\n");
    heapstring_init(&hstring, 0);
    char* hstringbase;

    heapstring_bind(hstring, &hstringbase);

    char bigString[] = "\nHello, world! I have lots of other work to do right now, but I'm working on my string API. Why? Because I'm silly.\n";

    of(od, "writing large string: %s", bigString);
    heapstring_stream(&hstring, bigString);

    if (strcmp(hstringbase, bigString) != 0) {
        msg = error("string %s does not match %s\n", hstringbase, bigString);
        goto cleanup;
    }
    of(od, "%scorrectly matches%s\n", hstringbase, bigString);
    heapstring_stream(&hstring, bigString);
    heapstring_stream(&hstring, bigString);
    heapstring_stream(&hstring, bigString);
    heapstring_stream(&hstring, bigString);
    heapstring_stream(&hstring, bigString);
    of(od, "Wrote many times. No crash; string is:%s\n", hstringbase);
cleanup:
    free(hstringbase);
    return msg;
}

char* test_11(void* od, outfunc of) {
    char* msg = NULL;
    return msg;
}
