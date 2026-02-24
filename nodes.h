#pragma once

#include "symbols.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define maxalign(ptr) (void*)((intptr_t)(((uint8_t*)ptr) + (__alignof (max_align_t) - 1)) & (~(__alignof (max_align_t) - 1 )))

typedef max_align_t align;

typedef struct node node;

struct node {
    int size;
};
    node* node_next(void* n) {
        n = (node*)((uint8_t*) n + ((node*)n)->size);
        n = maxalign(n);
        return n;
    }


typedef struct {
    node* base;
    node* lastNode;
    int blockCapacity;
} nodeBase;
    static void nodeBase_init(nodeBase* b, int initialSize) {
        *b = (nodeBase){0};
        b->blockCapacity = initialSize;
        b->base = malloc(b->blockCapacity); // blockCapacity is in bytes.
        b->lastNode = b->base;
    }
    static void nodeBase_add(nodeBase* b, void* newNodePtr) {
        node* newNode = (node*) newNodePtr;
        uint8_t* base = (uint8_t*)b->lastNode;
        base += b->lastNode->size;
        base = maxalign(base);
        size_t size = (uint8_t*)base - (uint8_t*)b->base;
        if (size + newNode->size >= b->blockCapacity) {
            b->blockCapacity *= 2;
            b->base = realloc(b->base, b->blockCapacity);
            base = (uint8_t*)b->base + size;
        }
        memcpy(base, newNode, newNode->size);
        b->lastNode = (node*) base;
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
    int statementCount;
    align statements[];
} statementGroupNode;

typedef struct statementNode {
    node node;
    int size;
} statementNode;

typedef struct declarationStatementNode {
    statementNode node;
    align identifier[];
} declarationStatementNode;

typedef int (*evalFunc)(void*);

typedef struct expressionNode {
    int size;
    evalFunc eval;
} expressionNode;

typedef struct integerNode {
    expressionNode node;
    int value;
} integerNode;

    static int integerNode_eval(integerNode* n) {
        return n->value;
    }

    static void integerNode_init(integerNode* n, int inner) {
        n->node.size = sizeof (integerNode);
        n->node.eval = (evalFunc) integerNode_eval;
        n->value = inner;
    }

typedef struct identifierNode {
    expressionNode node;
    char* lexeme;
    int lexemeLen;
    symbols_t* symbols;
} identifierNode;

    static int identifierNode_eval(identifierNode* n) {
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
        n->node.size = sizeof (identifierNode);
        n->node.eval = (evalFunc) identifierNode_eval;
        n->lexeme = lexeme;
        n->lexemeLen = lexemeLen;
        n->symbols = symbols;
    }

typedef struct {
    expressionNode node;
    align operands[];
} binaryOperatorNode;
    
    static int binaryOperatorNode_eval(binaryOperatorNode* n) {
        return 0;
    }

    static void binaryOperatorNode_init(binaryOperatorNode* n) {
        n->node.size = sizeof (binaryOperatorNode);
        n->node.eval = (evalFunc) binaryOperatorNode_eval;
    }

typedef struct {
    expressionNode node;
} plusOperatorNode;
    static int plusOperatorNode_eval(plusOperatorNode* n) {
        expressionNode* lhs = (expressionNode*) node_next(n);
        expressionNode* rhs = (expressionNode*) node_next(lhs);
        return lhs->eval(lhs) + rhs->eval(rhs);
    }
    static void plusOperatorNode_init(plusOperatorNode* n) {
        n->node.size = sizeof (plusOperatorNode);
        n->node.eval = (evalFunc) plusOperatorNode_eval;
    }

typedef struct {
    expressionNode node;
} minusOperatorNode;

    static int minusOperatorNode_eval(minusOperatorNode* n) {
        expressionNode* lhs = (expressionNode*) node_next(n);
        expressionNode* rhs = (expressionNode*) node_next(lhs);
        return lhs->eval(lhs) - rhs->eval(rhs);
    }

    static void minusOperatorNode_init(minusOperatorNode* n) {
        n->node.size = sizeof (minusOperatorNode);
        n->node.eval = (int (*)(void*)) minusOperatorNode_eval;
    }

typedef struct {
    expressionNode node;
} timesOperatorNode;

    static int timesOperatorNode_eval(timesOperatorNode* n) {
        expressionNode* lhs = (expressionNode*) node_next(n);
        expressionNode* rhs = (expressionNode*) node_next(lhs);
        return lhs->eval(lhs) * rhs->eval(rhs);
    }

    static void timesOperatorNode_init(timesOperatorNode* n) {
        n->node.size = sizeof (timesOperatorNode);
        n->node.eval = (int (*)(void*)) timesOperatorNode_eval;
    }

typedef struct {
    expressionNode node;
} divideOperatorNode;

    static int divideOperatorNode_eval(divideOperatorNode* n) {
        expressionNode* lhs = (expressionNode*) node_next(n);
        expressionNode* rhs = (expressionNode*) node_next(lhs);
        int divisor = rhs->eval(rhs);
        if (divisor == 0) return 0;
        return lhs->eval(lhs) / divisor;
    }

    static void divideOperatorNode_init(divideOperatorNode* n) {
        n->node.size = sizeof (divideOperatorNode);
        n->node.eval = (int (*)(void*)) divideOperatorNode_eval;
    }

typedef struct {
    expressionNode node;
} equalOperatorNode;
    static int equalOperatorNode_eval(equalOperatorNode* n) {
        expressionNode* lhs = (expressionNode*) node_next(n);
        expressionNode* rhs = (expressionNode*) node_next(lhs);
        return lhs->eval(lhs) == rhs->eval(rhs);
    }
    static void equalOperatorNode_init(equalOperatorNode* n) {
        n->node.size = sizeof (equalOperatorNode);
        n->node.eval = (int (*)(void*)) equalOperatorNode_eval;
    }

typedef struct {
    expressionNode node;
} notEqualOperatorNode;
    static int notEqualOperatorNode_eval(notEqualOperatorNode* n) {
        expressionNode* lhs = (expressionNode*) node_next(n);
        expressionNode* rhs = (expressionNode*) node_next(lhs);
        return lhs->eval(lhs) != rhs->eval(rhs);
    }
    static void notEqualOperatorNode_init(notEqualOperatorNode* n) {
        n->node.size = sizeof (notEqualOperatorNode);
        n->node.eval = (int (*)(void*)) notEqualOperatorNode_eval;
    }

typedef struct {
    expressionNode node;
} lessThanOperatorNode;
    static int lessThanOperatorNode_eval(lessThanOperatorNode* n) {
        expressionNode* lhs = (expressionNode*) node_next(n);
        expressionNode* rhs = (expressionNode*) node_next(lhs);
        return lhs->eval(lhs) < rhs->eval(rhs);
    }
    static void lessThanOperatorNode_init(lessThanOperatorNode* n) {
        n->node.size = sizeof (lessThanOperatorNode);
        n->node.eval = (int (*)(void*)) lessThanOperatorNode_eval;
    }

typedef struct {
    expressionNode node;
} greaterThanOperatorNode;
    static int greaterThanOperatorNode_eval(greaterThanOperatorNode* n) {
        expressionNode* lhs = (expressionNode*) node_next(n);
        expressionNode* rhs = (expressionNode*) node_next(lhs);
        return lhs->eval(lhs) > rhs->eval(rhs);
    }
    static void greaterThanOperatorNode_init(greaterThanOperatorNode* n) {
        n->node.size = sizeof (greaterThanOperatorNode);
        n->node.eval = (int (*)(void*)) greaterThanOperatorNode_eval;
    }

typedef struct {
    expressionNode node;
} lessEqualOperatorNode;
    static int lessEqualOperatorNode_eval(lessEqualOperatorNode* n) {
        expressionNode* lhs = (expressionNode*) node_next(n);
        expressionNode* rhs = (expressionNode*) node_next(lhs);
        return lhs->eval(lhs) <= rhs->eval(rhs);
    }
    static void lessEqualOperatorNode_init(lessEqualOperatorNode* n) {
        n->node.size = sizeof (lessEqualOperatorNode);
        n->node.eval = (int (*)(void*)) lessEqualOperatorNode_eval;
    }

typedef struct {
    expressionNode node;
} greaterEqualOperatorNode;
    static int greaterEqualOperatorNode_eval(greaterEqualOperatorNode* n) {
        expressionNode* lhs = (expressionNode*) node_next(n);
        expressionNode* rhs = (expressionNode*) node_next(lhs);
        return lhs->eval(lhs) >= rhs->eval(rhs);
    }
    static void greaterEqualOperatorNode_init(greaterEqualOperatorNode* n) {
        n->node.size = sizeof (greaterEqualOperatorNode);
        n->node.eval = (int (*)(void*)) greaterEqualOperatorNode_eval;
    }
