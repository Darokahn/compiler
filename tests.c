#include <stdarg.h>

#include "token.h"
#include "table/table.h"
#include "stateMachine.h"
#include "scanner.h"
#include "symbols.h"
#include "nodes.h"
#include "stringStreaming.h"
#include "nodeStreaming.h"

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
        token_aprettyPrint(&t, od, (void*) of);
    }
}

void scanAndPrintDebug(char* filename, symbols_t* symbols, void* od, outfunc of) {
    scanner_t s;
    scanner_init(&s, filename, symbols);
    token_t t;
    t.type = 0;
    of(od, "%-3d", s.lineCount);
    while (t.type != eof_token) {
        t = scanner_getNextToken(&s);
        if (t.type == bad_token) break;
        if (t.type == whitespace_token) {
            token_aprettyPrint(&t, od, (void*) of);
        }
        else if (t.type == newline_token) {
            token_aprettyPrint(&t, od, (void*) of);
            of(od, "%-3d", s.lineCount);
        }
        else {
            token_adebugPrettyPrint(&t, od, (void*) of);
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

char* test_11(void* od, outfunc of) {
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

char* test_12(void* od, outfunc of) {
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

char* test_13(void* od, outfunc of) {
    symbols_t symbols;
    initSymbols(&symbols);
    scanAndPrint("stringStreaming.h", &symbols, od, of);
    symbols_destroy(&symbols);
    return NULL;
}

// just goes and patches the function in memory... it works
void reallocHeist(void* patch, void* buffer, int size);
void reallocRestore(void* buffer, int size);
void* myFunc();

char* test_14(void* od, outfunc of) {
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

char* test_15(void* od, outfunc of) {
    char* msg = NULL;
    of(od, "Testing deep parse tree: (x * y) + (z - 10)\n");

    nodeBase b;
    nodeBase_init(&b, 1); // Start small to force reallocs

    symbols_t symbols;
    initSymbols(&symbols);

    // 1. Setup Symbols
    int xVal = 10, yVal = 5, zVal = 100;
    symbol_t symX = { .name = "x", .nameLen = 1, .type = variable_symbol, .v.value = xVal };
    symbol_t symY = { .name = "y", .nameLen = 1, .type = variable_symbol, .v.value = yVal };
    symbol_t symZ = { .name = "z", .nameLen = 1, .type = variable_symbol, .v.value = zVal };
    
    symbols_add(&symbols, symX.name, symX.nameLen, symX);
    symbols_add(&symbols, symY.name, symY.nameLen, symY);
    symbols_add(&symbols, symZ.name, symZ.nameLen, symZ);

    // 2. Define Operators
    plusOperatorNode plus;   plusOperatorNode_init(&plus);
    timesOperatorNode times; timesOperatorNode_init(&times);
    minusOperatorNode minus; minusOperatorNode_init(&minus);

    // 3. Build the Tree
    // Root: [+]
    int plusIdx = nodeBase_addChild(&b, 0, (node*)&plus); // Adding to root index 0

    // Left Side: [*]
    int timesIdx = nodeBase_addChild(&b, plusIdx, (node*)&times);
    
    // Children of [*]: x , y
    identifierNode iNode;
    identifierNode_init(&iNode, "x", 1, &symbols);
    nodeBase_addChild(&b, timesIdx, (node*)&iNode);
    
    identifierNode_init(&iNode, "y", 1, &symbols);
    nodeBase_addChild(&b, timesIdx, (node*)&iNode);

    // Right Side: [-]
    int minusIdx = nodeBase_addChild(&b, plusIdx, (node*)&minus);

    // Children of [-]: z , 10
    identifierNode_init(&iNode, "z", 1, &symbols);
    nodeBase_addChild(&b, minusIdx, (node*)&iNode);

    integerNode tenNode;
    integerNode_init(&tenNode, 10);
    nodeBase_addChild(&b, minusIdx, (node*)&tenNode);

    // 4. Evaluate
    // Note: We use the index returned by the first nodeBase_add (the root)
    expressionNode* root = (expressionNode*) node_from(b.base, plusIdx);
    
    int expected = (xVal * yVal) + (zVal - 10); // (10 * 5) + (100 - 10) = 140
    int result = root->eval(root);

    if (result != expected) {
        // Assuming your 'error' helper formats a heap string for the harness
        msg = error("Deep Eval Failed: Expected %d, got %d", expected, result);
        goto cleanup;
    }

    of(od, "Deep expression (x*y)+(z-10) evaluated to %d successfully\n", result);

cleanup:
    symbols_destroy(&symbols);
    nodeBase_destroy(&b);
    return msg;
}

char* test_16(void* od, outfunc of) {
    char* msg = NULL;
    of(od, "Testing logic gauntlet: ((x << 2) | (y >> 1)) >= (z %% 7) && (x + y == 15)\n");

    nodeBase b;
    nodeBase_init(&b, 1);

    symbols_t symbols;
    initSymbols(&symbols);

    // 1. Setup Symbols
    int xVal = 10, yVal = 5, zVal = 100;
    symbol_t symX = { .name = "x", .nameLen = 1, .type = variable_symbol, .v.value = xVal };
    symbol_t symY = { .name = "y", .nameLen = 1, .type = variable_symbol, .v.value = yVal };
    symbol_t symZ = { .name = "z", .nameLen = 1, .type = variable_symbol, .v.value = zVal };
    
    symbols_add(&symbols, symX.name, symX.nameLen, symX);
    symbols_add(&symbols, symY.name, symY.nameLen, symY);
    symbols_add(&symbols, symZ.name, symZ.nameLen, symZ);

    // 2. Initialize Operator Nodes
    andOperatorNode andNode;           andOperatorNode_init(&andNode);
    geOperatorNode geNode;             geOperatorNode_init(&geNode);
    bitwiseOrOperatorNode orNode;      bitwiseOrOperatorNode_init(&orNode);
    shlOperatorNode shlNode;           shlOperatorNode_init(&shlNode);
    shrOperatorNode shrNode;           shrOperatorNode_init(&shrNode);
    modOperatorNode modNode;           modOperatorNode_init(&modNode);
    eqOperatorNode eqNode;             eqOperatorNode_init(&eqNode);
    plusOperatorNode plusNode;         plusOperatorNode_init(&plusNode);

    // 3. Build the Tree: ((x << 2) | (y >> 1)) >= (z % 7) && (x + y == 15)
    
    // Root: &&
    int andIdx = nodeBase_addChild(&b, 0, (node*)&andNode);

    // Left child of &&: >=
    int geIdx = nodeBase_addChild(&b, andIdx, (node*)&geNode);

    // Left child of >=: |
    int orIdx = nodeBase_addChild(&b, geIdx, (node*)&orNode);
    
    // Left of |: <<
    int shlIdx = nodeBase_addChild(&b, orIdx, (node*)&shlNode);
    identifierNode iNode;
    identifierNode_init(&iNode, "x", 1, &symbols);
    nodeBase_addChild(&b, shlIdx, (node*)&iNode);
    integerNode valNode;
    integerNode_init(&valNode, 2);
    nodeBase_addChild(&b, shlIdx, (node*)&valNode);

    // Right of |: >>
    int shrIdx = nodeBase_addChild(&b, orIdx, (node*)&shrNode);
    identifierNode_init(&iNode, "y", 1, &symbols);
    nodeBase_addChild(&b, shrIdx, (node*)&iNode);
    integerNode_init(&valNode, 1);
    nodeBase_addChild(&b, shrIdx, (node*)&valNode);

    // Right child of >=: %
    int modIdx = nodeBase_addChild(&b, geIdx, (node*)&modNode);
    identifierNode_init(&iNode, "z", 1, &symbols);
    nodeBase_addChild(&b, modIdx, (node*)&iNode);
    integerNode_init(&valNode, 7);
    nodeBase_addChild(&b, modIdx, (node*)&valNode);

    // Right child of &&: ==
    int eqIdx = nodeBase_addChild(&b, andIdx, (node*)&eqNode);
    
    // Left of ==: +
    int plusIdx = nodeBase_addChild(&b, eqIdx, (node*)&plusNode);
    identifierNode_init(&iNode, "x", 1, &symbols);
    nodeBase_addChild(&b, plusIdx, (node*)&iNode);
    identifierNode_init(&iNode, "y", 1, &symbols);
    nodeBase_addChild(&b, plusIdx, (node*)&iNode);

    // Right of ==: 15
    integerNode_init(&valNode, 15);
    nodeBase_addChild(&b, eqIdx, (node*)&valNode);

    // 4. Evaluation
    expressionNode* root = (expressionNode*) node_from(b.base, andIdx);
    
    // Math Check:
    // ((10 << 2) | (5 >> 1)) >= (100 % 7) && (10 + 5 == 15)
    // ((40) | (2)) >= (2) && (15 == 15)
    // 42 >= 2 && 1
    // 1 && 1 => 1
    int expected = 1; 
    int result = root->eval(root);

    if (result != expected) {
        msg = error("Gauntlet Failed: Expected %d, got %d", expected, result);
        goto cleanup;
    }

    of(od, "Logic gauntlet passed: result is %d\n", result);

cleanup:
    symbols_destroy(&symbols);
    nodeBase_destroy(&b);
    return msg;
}

char* test_17(void* od, outfunc of) {
    char* msg = NULL;
    of(od, "Testing node initialization via streaming\n");
    nodeBase b;
    nodeBase_init(&b, 1);
    nodeBaseCursor c;
    nodeBaseCursor_init(&c, &b);

cleanup:
    nodeBase_destroy(&b);
    nodeBaseCursor_destroy(&c);
    return msg;
}
