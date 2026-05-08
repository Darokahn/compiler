#include <stdarg.h>

#include "token.h"
#include "table/table.h"
#include "stateMachine.h"
#include "scanner.h"
#include "symbols.h"
#include "nodes.h"
#include "nodeStreaming.h"
#include "parser.h"
#include "stringStreaming/stringstream.h"
#include "instructions.h"

char* error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char* newError;
    vasprintf(&newError, fmt, args);
    va_end(args);
    return newError;
}

int scanAndPrint(char* filename, symbols_t* symbols, void* od, aprintf of) {
    scanner_t s;
    int err = scanner_init(&s, filename, symbols);
    if (err != 0) return err;
    token_t t;
    t.type = ~eof_token; // just some value that does not equal it
    while (t.type != eof_token) {
        t = scanner_getNextToken(&s);
        if (t.type == bad_token) break;
        token_aprettyPrint(&t, od, (void*) of);
    }
    return 0;
}

int scanAndPrintDebug(char* filename, symbols_t* symbols, void* od, aprintf of) {
    scanner_t s;
    int err = scanner_init(&s, filename, symbols);
    if (err != 0) return err;
    token_t t;
    t.type = ~eof_token;
    of(od, "%-3d", s.state.lineCount);
    while (t.type != eof_token) {
        t = scanner_getNextToken(&s);
        if (t.type == bad_token) break;
        if (t.type == whitespace_token) {
            token_aprettyPrint(&t, od, (void*) of);
        }
        else if (t.type == newline_token) {
            token_aprettyPrint(&t, od, (void*) of);
            of(od, "%-3d", s.state.lineCount);
        }
        else {
            token_adebugPrettyPrint(&t, od, (void*) of);
        }
    }
    return 0;
}

void initSymbols(symbols_t* t) {
    symbols_init(t, NAMESPACESIZE);
    for (int i = 0; i < LASTKEYWORD; i++) {
        symbol_t sym = {
            .type = keyword_symbol
        };
        symbols_add(t, keywordStrings[i], strlen(keywordStrings[i]), sym);
    }
}

char* test_0(void* od, aprintf of) {
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

char* test_1(void* od, aprintf of) {
    symbols_t symbols;
    initSymbols(&symbols);
    scanAndPrint("main.c", &symbols, od, of);
    symbols_destroy(&symbols);
    return NULL;
}

char* test_2(void* od, aprintf of) {
    symbols_t symbols;
    initSymbols(&symbols);
    scanAndPrintDebug("testfiles/test.c", &symbols, od, of);
    of(od, "\n");
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

char* test_3(void* od, aprintf of) {
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

char* test_4(void* od, aprintf of) {
    of(od, "Testing parse tree resize\n");
    nodeBase_t b;
    nodeBase_init(&b, 1);
    node n;
    node_init(&n, &node_defaultVtable, true);
    nodeBase_add(&b, &n);
    nodeBase_add(&b, &n);
    nodeBase_add(&b, &n);
    of(od, "No segfault, we're good\n");
    nodeBase_destroy(&b);
    return NULL;
}

char* test_5(void* od, aprintf of) {
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

char* test_6(void* od, aprintf of) {
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

char* test_7(void* od, aprintf of) {
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
    char* checkbase;
    for (int i = 0; i < 6; i++) {
        checkbase = hstringbase + (sizeof bigString - 1) * i;
        if (strncmp(checkbase, bigString, sizeof bigString - 1) != 0) {
            msg = error("string %.*s does not match string %s", checkbase, bigString);
            goto cleanup;
        }
    }
    of(od, "Wrote many times; all equal; string is:%s\n", hstringbase);
cleanup:
    free(hstringbase);
    return msg;
}

char* test_8(void* od, aprintf of) {
    char* msg = NULL;
    of(od, "Testing static strings\n");
    char base[1024];
    char* s = base;
    staticstring_init(&s, sizeof base);
    int remaining = staticstring_getRemaining(s);
    int expectedRemaining = sizeof base;
    if (remaining != expectedRemaining) {
        msg = error("remaining %d does not equal expected %d\n", remaining, expectedRemaining);
        goto cleanup;
    }
    of(od, "remaining %d is correct\n", remaining);

    char* testString = "hello, world!\n";

    staticstring_stream(&s, testString);

    if (strcmp(testString, base) != 0) {
        msg = error("string %s does not equal %s\n", base, testString);
        goto cleanup;
    }

    of(od, "string %s matches its template\n", base);

cleanup:
    return msg;
}

char* test_9(void* od, aprintf of) {
    char* msg = NULL;
    of(od, "Testing static strings proper behavior when full\n");
    char testString[] = "hello world";

    char base[sizeof testString];

    char* s = base;
    staticstring_init(&s, sizeof base);

    staticstring_stream(&s, testString);

    if (s != NULL) {
        msg = error("stream did not properly destroy string when buffer was full\n");
        goto cleanup;
    }
    of(od, "stream properly destroyed string since its buffer was full\n");

    if (strcmp(base, testString) != 0) {
        msg = error("string %s does not equal %s\n", base, testString);
        goto cleanup;
    }
    of(od, "string %s properly matches template\n", base);

cleanup:
    return msg;
}

char* test_10(void* od, aprintf of) {
    char* msg = NULL;
    symbols_t symbols;
    initSymbols(&symbols);
    int err = scanAndPrint("stringStreaming/stringstream.h", &symbols, od, of);
    if (err != 0) {
        msg = error("%s", strerror(err));
        goto cleanup;
    }
cleanup:
    symbols_destroy(&symbols);
    return msg;
}

// just goes and patches the function in memory... it works
void reallocHeist(void* patch, void* buffer, int size);
void reallocRestore(void* buffer, int size);
void* myFunc();

char* test_11(void* od, aprintf of) {
    char* msg = NULL;
    of(od, "Testing heapstring proper behavior when allocation fails\n");

    char* longString = "This string is at least a few words long, but I'm not sure how many. I will be done typing it soon, but not quite yet.";

    char* s;
    heapstring_init(&s, 32);
    char* base = heapstring_getBase(s);
    uint8_t buffer[32];
    reallocHeist(myFunc, buffer, sizeof buffer);
    char* attemptMessage = "\nAttempting reallocating operation again\n\n";
    char* newBase;
    for (int i = 0; i < 2; i++) {
        heapstring_stream(&s, "%s%s\n", longString, longString);

        newBase = heapstring_getBase(s);

        if (newBase != base) {
            msg = error("base has changed from %p to %p despite a failed reallocation\n", base, newBase);
            goto cleanup;
        }
        of(od, "base has correctly not moved\n");

        int remaining = heapstring_getRemaining(s);

        if (remaining != heapstring_minsize) {
            msg = error("remaining %d does not equal %d\n", remaining, heapstring_minsize);
            goto cleanup;
        }
        of(od, "remaining is correctly %d\n", remaining);

        of(od, attemptMessage);
        attemptMessage = "";
    }

cleanup:
    reallocRestore(buffer, sizeof buffer);
    free(newBase);
    return msg;
}

char* test_12(void* od, aprintf of) {
    char* msg = NULL;
    parser_t parser;
    parser_init(&parser, "testfiles/test1.c");
    of(od, "Parsing testfiles/test1.c...\n");
    bool success = parser_start(&parser);
    parser_interpret(&parser);
    parser_execute(&parser);
    if (!success) {
        msg = error("Parse failed: parser_start returned failure for testfiles/test1.c\n");
        goto cleanup;
    }
    of(od, "Parse succeeded: testfiles/test1.c accepted by grammar\n");
cleanup:
    parser_destroy(&parser);
    return msg;
}

char* test_13(void* od, aprintf of) {
    char* msg = NULL;
    parser_t parser;
    parser_init(&parser, "testfiles/test2.c");
    of(od, "Parsing testfiles/test2.c...\n");
    bool success = parser_start(&parser);
    if (success) {
        msg = error("Parse succeeded: parser_start should have rejected malformed testfiles/test2.c\n");
        goto cleanup;
    }
    of(od, "Parse correctly rejected malformed testfiles/test2.c\n");
cleanup:
    parser_destroy(&parser);
    return msg;
}

char* test_14(void* od, aprintf of) {
    char* msg = NULL;
    parser_t parser;
    parser_init(&parser, "testfiles/test3.c");
    of(od, "Parsing testfiles/test3.c...\n");
    bool success = parser_start(&parser);
    parser_interpret(&parser);
    parser_execute(&parser);
    if (!success) {
        msg = error("Parse failed: parser_start returned failure for testfiles/test3.c\n");
        goto cleanup;
    }
    of(od, "Parse succeeded: testfiles/test3.c accepted by grammar\n");
cleanup:
    parser_destroy(&parser);
    return msg;
}
