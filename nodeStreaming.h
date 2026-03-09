#include "table/table.h"
#include "nodes.h"
#include "stateMachine.h"

// this entire implementation is a human interface feature meant to make initializing nodes easy.

enum cursorDirection {
    cursor_child,
    cursor_sibling
};

// nasty macro trick for defining these in one place without putting them in the namespace
#define declarestates \
enum nodeBaseCursor_parseState { \
    expectingChild, \
    expectingCursorCommand, \
    funcExpectingArgOrCommand, \
    funcExpectingArg, \
    funcExpectingComma, \
    funcParsingArg, \
    bad, \
}

typedef node* (*nodeInitFunc)(node*, ...);

typedef struct {
    table_t nodeIndex;
    nodeBase* base;
    int current;
    nodeInitFunc currentFunc;
    char* currentFuncSchema;
    char* currentFuncName;
    int currentFuncNameLen;
    erased currentArgs[8];
    int argCount;
    int parseState;
    enum cursorDirection cursorDirection;
} nodeBaseCursor;

static void nodeBaseCursor_init(nodeBaseCursor* cursor, nodeBase* base) {
    *cursor = (nodeBaseCursor) {0};
    int tableLen = sizeof nodeTable / sizeof *nodeTable;
    table_init(&cursor->nodeIndex, tableLen * 2);
    for (int i = 0; i < tableLen; i++) {
        struct nodeDescriptor n = nodeTable[i];
        *table_insert(&cursor->nodeIndex, n.name, strlen(n.name)) = i;
    }
    cursor->base = base;
}

static bool nodeBaseCursor_setInitFunc(nodeBaseCursor* cursor, char* name, int nameLen) {
    int* indexPtr = table_lookup(&cursor->nodeIndex, name, nameLen);
    if (indexPtr == TABLE_NULL) return false;
    int index = *indexPtr;
    struct nodeDescriptor d = nodeTable[index];
    cursor->currentFunc = d.initFunc;
    cursor->currentFuncSchema = d.initFuncSchema;
    cursor->currentFuncName = name;
    cursor->currentFuncNameLen = nameLen;
    return true;
}

static void nodeBaseCursor_addArg(nodeBaseCursor* cursor, erased arg) {
    cursor->currentArgs[cursor->argCount] = arg;
    cursor->argCount++;
}

static void nodeBaseCursor_setCursorDirection(nodeBaseCursor* cursor, enum cursorDirection d) {
    cursor->cursorDirection = d;
}

static node* nodeBaseCursor_addNode(nodeBaseCursor* cursor, node* node) {
    int currentIndex = cursor->current;
    if (cursor->cursorDirection == cursor_child) {
        nodeBase_addChild(cursor->base, currentIndex, node);
    }
    else if (cursor->cursorDirection == cursor_sibling) {
        nodeBase_addSibling(cursor->base, currentIndex, node);
    }
    else return NULL;
    return node;
}

static node* nodeBaseCursor_initNode(nodeBaseCursor* cursor) {
    if (cursor->currentFunc == NULL) {
        return NULL;
    }
    // schema walks forward as it consumes elements; it should be spent by the time the init func is called.
    if (*(cursor->currentFuncSchema) != '\0') {
        //return NULL;
    }
    char* buffer[1024];
    node* node; 
    int c = cursor->argCount;
    erased* a = cursor->currentArgs;
    // C has no better alternative short of a custom va_args utility which destroys the ergonomics of the user-facing function declaration
    switch (c) {
        case 0: node = cursor->currentFunc((void*)buffer); break;
        case 1: node = cursor->currentFunc((void*)buffer, a[0]); break;
        case 2: node = cursor->currentFunc((void*)buffer, a[0], a[1]); break;
        case 3: node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2]); break;
        case 4: node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2], a[3]); break;
        case 5: node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2], a[3], a[4]); break;
        case 6: node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2], a[3], a[4], a[5]); break;
        case 7: node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2], a[3], a[4], a[5], a[6]); break;
        case 8: node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]); break;
        default: return NULL;
    }
    cursor->currentFunc = NULL;
    cursor->argCount = 0;
    return nodeBaseCursor_addNode(cursor, node);
}

static node* nodeBaseCursor_advance(nodeBaseCursor* cursor) {
    int currentIndex = cursor->current;
    node* currentNode = node_from(cursor->base->base, currentIndex);
    if (cursor->cursorDirection == cursor_child) {
        currentIndex += currentNode->lastChild;
    }
    else if (cursor->cursorDirection == cursor_sibling) {
        currentIndex += currentNode->nextSibling;
    }
    else return NULL;
    cursor->current = currentIndex;
    return node_from(cursor->base->base, currentIndex);
}

static node* nodeBaseCursor_retreat(nodeBaseCursor* cursor) {
    int currentIndex = cursor->current;
    node* currentNode = node_from(cursor->base->base, currentIndex);
    currentIndex -= currentNode->parent;
    cursor->current = currentIndex;
    return node_from(cursor->base->base, currentIndex);
}

static void nodeBaseCursor_destroy(nodeBaseCursor* cursor) {
    table_destroy(&cursor->nodeIndex);
}

static int nodeBaseCursor_getChar(char** base) {
    int c = *base[0];
    if (c == 0) return EOF;
    *base = *base + 1;
    return c;
}

static int nodeBaseCursor_handleEC(nodeBaseCursor* cursor, enum tokenType tok, char* lexeme, int lexemeLen) {
    declarestates;
    switch (tok) {
        case identifier_token:
            bool success = nodeBaseCursor_setInitFunc(cursor, lexeme, lexemeLen);
            if (!success) return bad;
            return funcExpectingArgOrCommand;
        default:
            return bad;
    }
}

static int nodeBaseCursor_handleECC(nodeBaseCursor* cursor, enum tokenType tok, char* lexeme, int lexemeLen) {
    (void) lexeme;
    (void) lexemeLen;
    declarestates;
    switch (tok) {
        case rangle_token:
            nodeBaseCursor_setCursorDirection(cursor, cursor_child);
            return expectingChild;
        case comma_token:
            nodeBaseCursor_setCursorDirection(cursor, cursor_sibling);
            return expectingChild;
        case semicolon_token:
            nodeBaseCursor_retreat(cursor);
            nodeBaseCursor_setCursorDirection(cursor, cursor_sibling);
            return expectingChild;
        case rparen_token:
            nodeBaseCursor_retreat(cursor);
            return expectingCursorCommand;
        default:
            return bad;
    }
}

static int nodeBaseCursor_handleFEAC(nodeBaseCursor* cursor, enum tokenType tok, char* lexeme, int lexemeLen) {
    declarestates;
    switch (tok) {
        case rangle_token:
            nodeBaseCursor_initNode(cursor);
            nodeBaseCursor_advance(cursor);
            nodeBaseCursor_setCursorDirection(cursor, cursor_child);
            return expectingChild;
        case comma_token:
            nodeBaseCursor_initNode(cursor);
            nodeBaseCursor_advance(cursor);
            nodeBaseCursor_setCursorDirection(cursor, cursor_sibling);
            return expectingChild;
        case semicolon_token:
            nodeBaseCursor_initNode(cursor);
            nodeBaseCursor_advance(cursor);
            nodeBaseCursor_setCursorDirection(cursor, cursor_sibling);
            return expectingChild;
        case rparen_token:
            nodeBaseCursor_initNode(cursor);
            nodeBaseCursor_advance(cursor);
            return expectingCursorCommand;
        case decimal_token:
            int val = atoi(lexeme);
            nodeBaseCursor_addArg(cursor, erase val);
            return funcExpectingComma;
        case charliteral_token:
            // avoid the quotes on the outside
            nodeBaseCursor_addArg(cursor, erase (lexeme + 1));
            nodeBaseCursor_addArg(cursor, erase (lexemeLen - 2));
            return funcExpectingComma;
        case mod_token:
            return funcParsingArg;
        default:
            return bad;
    }
}

static int nodeBaseCursor_handleFEA(nodeBaseCursor* cursor, enum tokenType tok, char* lexeme, int lexemeLen) {
    declarestates;
    switch (tok) {
        case decimal_token:
            int val = atoi(lexeme);
            nodeBaseCursor_addArg(cursor, erase val);
            return funcExpectingComma;
        case charliteral_token:
            nodeBaseCursor_addArg(cursor, erase lexeme);
            nodeBaseCursor_addArg(cursor, erase lexemeLen);
            return funcExpectingComma;
        case mod_token:
            return funcParsingArg;
        default:
            return bad;
    }
}

static int nodeBaseCursor_handleFEC(nodeBaseCursor* cursor, enum tokenType tok, char* lexeme, int lexemeLen) {
    (void) lexeme;
    (void) lexemeLen;
    declarestates;
    switch (tok) {
        case comma_token:
            return funcExpectingArg;
        case semicolon_token:
            nodeBaseCursor_initNode(cursor);
            nodeBaseCursor_advance(cursor);
            nodeBaseCursor_setCursorDirection(cursor, cursor_sibling);
            return expectingChild;
        case rparen_token:
            nodeBaseCursor_initNode(cursor);
            nodeBaseCursor_advance(cursor);
            return expectingCursorCommand;

        default:
            return bad;
    }
}

static int nodeBaseCursor_addNodeStream(nodeBaseCursor* cursor, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char* base = fmt;

    declarestates;
    stateMachine_t m;
    stateMachine_init(&m);
    enum tokenType tok;
    char* lexeme;
    int lexemeLen;
    int parenDepth = 0;

    for (; true; fmt = lexeme + lexemeLen) {
        lexeme = fmt;
        lexemeLen = stateMachine_getToken(&fmt, nodeBaseCursor_getChar, &tok);
        if (tok == whitespace_token) continue;
        else if (tok == lparen_token) {
            parenDepth++;
            continue;
        }
        else if (tok == rparen_token) {
            if (parenDepth == 0) break;
            parenDepth--;
        }
        if (lexemeLen == 0) break;
        switch (cursor->parseState) {
            case expectingChild: 
                cursor->parseState = nodeBaseCursor_handleEC(cursor, tok, lexeme, lexemeLen);
                break;
            case expectingCursorCommand: 
                cursor->parseState = nodeBaseCursor_handleECC(cursor, tok, lexeme, lexemeLen);
                break;
            case funcExpectingArgOrCommand: 
                cursor->parseState = nodeBaseCursor_handleFEAC(cursor, tok, lexeme, lexemeLen);
                break;
            case funcExpectingArg:
                cursor->parseState = nodeBaseCursor_handleFEA(cursor, tok, lexeme, lexemeLen);
                break;
            case funcExpectingComma:
                cursor->parseState = nodeBaseCursor_handleFEC(cursor, tok, lexeme, lexemeLen);
                break;
            case funcParsingArg: 
                if (tok != identifier_token) {
                    cursor->parseState = bad;
                    break;
                }
                if (*lexeme != 'p') {
                    cursor->parseState = bad;
                    break;
                }
                erased arg = va_arg(args, erased);
                nodeBaseCursor_addArg(cursor, arg);
                cursor->parseState = funcExpectingComma;
                break;
        }
        if (cursor->parseState == bad) break;
        fmt = lexeme + lexemeLen;
    }
    goto cleanup; // used so the compiler doesn't complain about the label
cleanup:
    va_end(args);
    return fmt - base;
}
