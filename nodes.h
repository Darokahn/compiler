#pragma once

#include "symbols.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#define maxalign(ptr) (void*)((intptr_t)(((uint8_t*)ptr) + (__alignof (max_align_t) - 1)) & (~(__alignof (max_align_t) - 1 )))

typedef max_align_t align;

typedef struct node node;

struct node {
    int size;
    int lastChild;
    int nextSibling;
};
    static node* node_from(node* n, int offset) {
        return (node*) ((uint8_t*) n + offset);
    }
    static intptr_t node_between(node* n1, node* n2) {
        return ((intptr_t) n2 - (intptr_t) n1);
    }
    static node* node_firstChild(node* n) {
        return node_from(n, n->size);
    }
    static node* node_nextSibling(node* n) {
        return node_from(n, n->nextSibling);
    }
    static void node_init(node*n, int size) {
        n->size = size;
        n->nextSibling = 0;
        n->lastChild = n->size;
    }


typedef struct {
    node* base;
    node* lastNode;
    int blockCapacity;
} nodeBase;
    static void nodeBase_init(nodeBase* b, int initialSize) {
        *b = (nodeBase){0};
        b->blockCapacity = MAX(initialSize, sizeof (node));
        b->base = malloc(b->blockCapacity); // blockCapacity is in bytes.
        b->lastNode = b->base;
        *b->lastNode = (node) {0};
    }
    static node* nodeBase_add(nodeBase* b, node* newNode) {
        uint8_t* base = (uint8_t*)b->lastNode;
        base += b->lastNode->size;
        base = maxalign(base);
        size_t size = (uint8_t*)base - (uint8_t*)b->base;
        int minimumSize = size + newNode->size;
        if (minimumSize >= b->blockCapacity) {
            b->blockCapacity *= 2;
            b->blockCapacity = MAX(b->blockCapacity, minimumSize);
            b->base = realloc(b->base, b->blockCapacity);
            base = (uint8_t*)b->base + size;
        }
        memcpy(base, newNode, newNode->size);
        b->lastNode = (node*) base;
        return b->lastNode;
    }

    static int nodeBase_addChild(nodeBase* b, int parentIndex, node* newNode) {
        newNode = nodeBase_add(b, newNode);
        node* parent = node_from(b->base, parentIndex);
        node* lastChild = node_from(parent, parent->lastChild);
        lastChild->nextSibling = node_between(lastChild, newNode);
        parent->lastChild = node_between(parent, newNode);
        return node_between(b->base, newNode);
    }

    static void nodeBase_destroy(nodeBase* b) {
        if (b->base != NULL) {
            free(b->base);
        }
    }

typedef struct startNode {
    node node;
    align program[];
} startNode;

typedef struct programNode {
    node node;
    align block[];
} programNode;

typedef struct blockNode {
    node node;
    align statementGroup[];
} blockNode;

typedef struct statementGroupNode {
    node node;
    int lastStatement;
    align statements[];
} statementGroupNode;

typedef struct statementNode {
    node node;
    int next;
} statementNode;

typedef struct declarationStatementNode {
    statementNode node;
    align identifier[];
} declarationStatementNode;

typedef int (*evalFunc)(void*);

typedef struct expressionNode {
    node node;
    evalFunc eval;
} expressionNode;
    static void expressionNode_init(expressionNode* n, evalFunc eval, int size) {
        node_init((node*) n, size);
        n->eval = eval;
    }

typedef struct integerNode {
    expressionNode node;
    int value;
} integerNode;

    static int integerNode_eval(void* nptr) {
        integerNode* n = (integerNode*) nptr;
        return n->value;
    }

    static void integerNode_init(integerNode* n, int inner) {
        expressionNode_init((expressionNode*) n, integerNode_eval, sizeof *n);
        n->value = inner;
    }

typedef struct identifierNode {
    expressionNode node;
    char* lexeme;
    int lexemeLen;
    symbols_t* symbols;
} identifierNode;

    static int identifierNode_eval(void* nptr) {
        identifierNode* n = (identifierNode*) nptr;
        symbol_t* sym = symbols_lookup(n->symbols, n->lexeme, n->lexemeLen);
        return sym->v.value;
    }

    static void identifierNode_assign(identifierNode* n, int val) {
        symbol_t* sym = symbols_lookup(n->symbols, n->lexeme, n->lexemeLen);
        sym->v.value = val;
    }

    static int identifierNode_getIndex(identifierNode* n) {
        return 0;
    }

    static void identifierNode_init(identifierNode* n, char* lexeme, int lexemeLen, symbols_t* symbols) {
        expressionNode_init((expressionNode*) n, identifierNode_eval, sizeof *n);
        n->lexeme = lexeme;
        n->lexemeLen = lexemeLen;
        n->symbols = symbols;
    }

typedef int (*arithmeticFunc)(int, int);

typedef struct {
    expressionNode node;
    arithmeticFunc operator;
    align operands[];
} binaryOperatorNode;
    
    static int binaryOperatorNode_eval(void* nptr) {
        binaryOperatorNode* n = (binaryOperatorNode*) nptr;
        expressionNode* lhs = (expressionNode*) node_firstChild((node*) n);
        expressionNode* rhs = (expressionNode*) node_nextSibling((node*) lhs);
        return n->operator(lhs->eval(lhs), rhs->eval(rhs));
    }

    static void binaryOperatorNode_init(binaryOperatorNode* n, arithmeticFunc operator, int size) {
        expressionNode_init((expressionNode*) n, binaryOperatorNode_eval, size);
        n->operator = operator;
    }

typedef struct {
    binaryOperatorNode node;
} plusOperatorNode;
    static int plusOperatorNode_op(int x, int y) {
        return x + y;
    }
    static void plusOperatorNode_init(plusOperatorNode* n) {
        binaryOperatorNode_init((binaryOperatorNode*) n, plusOperatorNode_op, sizeof *n);
    }

#define DEFINE_BINARY_OP(name, op) \
typedef struct { binaryOperatorNode node; } name##OperatorNode; \
static int name##OperatorNode_op(int x, int y) { return x op y; } \
static void name##OperatorNode_init(name##OperatorNode* n) { \
    binaryOperatorNode_init((binaryOperatorNode*) n, name##OperatorNode_op, sizeof *n); \
}

DEFINE_BINARY_OP(minus, -);
DEFINE_BINARY_OP(times, *);
DEFINE_BINARY_OP(divide, /);
DEFINE_BINARY_OP(mod, %);
DEFINE_BINARY_OP(and, &&);
DEFINE_BINARY_OP(or, ||);
DEFINE_BINARY_OP(bitwiseAnd, &);
DEFINE_BINARY_OP(bitwiseOr, |);
DEFINE_BINARY_OP(xor, ^);
DEFINE_BINARY_OP(shl, <<);
DEFINE_BINARY_OP(shr, >>);
DEFINE_BINARY_OP(eq, ==);
DEFINE_BINARY_OP(neq, !=);
DEFINE_BINARY_OP(ge, >=);
DEFINE_BINARY_OP(gt, >);
DEFINE_BINARY_OP(le, <=);
DEFINE_BINARY_OP(lt, <);
