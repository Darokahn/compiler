#include "table/table.h"
#include "nodes.h"
#include "stateMachine.h"

// this entire implementation is a human interface feature meant to make initializing nodes easy.

struct nodeDescriptor {
    void* initFunc;
    char* initFuncSchema;
    char* name;
};

#define desc(nodename, schema) (struct nodeDescriptor) {nodename##Node_init, schema, #nodename}

struct nodeDescriptor nodeTable[] = {
    desc(start, ""),
    desc(program, ""),
    desc(block, ""),
    desc(statementGroup, ""),
    desc(statement, ""),
    desc(declarationStatement, ""),
    desc(integer, "%d"),
    desc(identifier, "%s%d%p"),
#undef desc
#define desc(nodename) (struct nodeDescriptor) {nodename##OperatorNode_init, "", #nodename}
    desc(plus),
    desc(minus),
    desc(times),
    desc(divide),
    desc(mod),
    desc(and),
    desc(or),
    desc(bitwiseAnd),
    desc(bitwiseOr),
    desc(xor),
    desc(shl),
    desc(shr),
    desc(eq),
    desc(neq),
    desc(ge),
    desc(gt),
    desc(le),
    desc(lt)
};

typedef struct {
    table_t nodeIndex;
    nodeBase* base;
    int current;
    node* (*currentFunc)(node*, ...);
    char* currentFuncSchema;
    int* currentArgs[8];
    int argCount;
    int state;
    bool addChildOrSibling;
} nodeBaseCursor;

void nodeBaseCursor_init(nodeBaseCursor* cursor, nodeBase* base) {
    int tableLen = sizeof nodeTable / sizeof *nodeTable;
    table_init(&cursor->nodeIndex, tableLen * 2);
    for (int i = 0; i < tableLen; i++) {
        struct nodeDescriptor n = nodeTable[i];
        *table_insert(&cursor->nodeIndex, n.name, strlen(n.name)) = i;
    }
    cursor->base = base;
    cursor->current = 0;
}

void nodeBaseCursor_addArg(nodeBaseCursor* cursor, int* arg) {
    cursor->currentArgs[cursor->argCount] = arg;
    cursor->argCount++;
}

void nodeBaseCursor_addNode(nodeBaseCursor* cursor) {
    char* buffer[1024];
    node* node; 
    int c = cursor->argCount;
    int** a = cursor->currentArgs;
    if (c==0) node = cursor->currentFunc((void*)buffer);
    else if (c==1) node = cursor->currentFunc((void*)buffer, a[0]);
    else if (c==2) node = cursor->currentFunc((void*)buffer, a[0], a[1]);
    else if (c==3) node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2]);
    else if (c==4) node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2], a[3]);
    else if (c==5) node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2], a[3], a[4]);
    else if (c==6) node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2], a[3], a[4], a[5]);
    else if (c==7) node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
    else if (c==8) node = cursor->currentFunc((void*)buffer, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
}

void nodeBaseCursor_destroy(nodeBaseCursor* cursor) {
    table_destroy(&cursor->nodeIndex);
}

int nodeBaseCursor_getChar(char** base) {
    int c = **base;
    if (c == 0) return EOF;
    *base++;
    return c;
}

void nodeBaseCursor_addNodeStream(nodeBaseCursor* cursor, char* fmt, ...) {
/*
    enum readState {
        expectingChild,
        expectingChildOrChildren,
        funcExpectingArgsOrNext,
        funcExpectingArg,
        funcExpectingCommaOrEnd,
    };
    va_list args;
    va_start(args, fmt);
    node* n;
    int state = expectingChildOrChildren;
    stateMachine_t m;
    stateMachine_init(&m);
    enum tokenType tok;

    stateMachine_getToken(&fmt, nodeBaseCursor_getChar, &tok);
start:
    switch (state) {
        case expectingChild:
            if (tok == identifier_token) {
                state = funcExpectingArgsOrNext;
                goto start;
            }
            else goto cleanup;
        case expectingChildOrChildren:
            if (tok == identifier_token) {
                state = funcExpectingArgsOrNext;
                goto start;
            }
            if (tok == lparen_token) {
                state = expectingChild;
                goto start;
            }
            else goto cleanup;
        case funcExpectingArgsOrNext:
            if (tok == rangle_token) {
                state = expectingChildOrChildren;
                goto start;
            }
            if (tok == lparen_token) {
                state = funcExpectingArg;
                goto start;
            }
            else goto cleanup;
        case funcExpectingArg:
            if (tok == decimal_token) {
            }
    }


cleanup:
    va_end(args);
*/
}

void nodeBaseCursor_addNodeStreamStr(nodeBaseCursor* cursor, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    node* n;

    va_end(args);
}
