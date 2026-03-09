#pragma once

#include "symbols.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <stdalign.h>

#define maxalign(ptr) (void*)((intptr_t)(((uint8_t*)ptr) + (__alignof (max_align_t) - 1)) & (~(__alignof (max_align_t) - 1 )))
typedef void* erased;

#define indexScale (__alignof(max_align_t))


typedef max_align_t align;

typedef struct node node;

typedef struct {
    char* name;
    int size;
    int (*format)(node*, linePrinter*);
} node_vtable;

// indices are scaled up by `indexScale` before use because an index must be aligned with `max_align_t`.
struct node {
    node_vtable* vtable;
    uint16_t firstChild;
    uint16_t lastChild;
    uint16_t nextSibling;
    uint16_t parent; // implicitly negative
};
    static node* node_from(node* n, uint16_t offset) {
        return (node*) ((uint8_t*) n + (offset * indexScale));
    }
    static uint16_t node_between(node* n1, node* n2) {
        return (uint16_t)(((intptr_t) n2 - (intptr_t) n1) / indexScale);
    }
    static node* node_firstChild(node* n) {
        if (n->firstChild == 0) return NULL;
        return node_from(n, n->firstChild);
    }
    static node* node_nextSibling(node* n) {
        if (n->nextSibling == 0) return NULL;
        return node_from(n, n->nextSibling);
    }
    static int node_format(node* n, linePrinter* printer) {
        int total = 0;
        total += linePrinter_stream(printer, "# %s(size: %d) #\nfirstChild: %d\nnextSibling: %d\nparent: %d\nchildren: {", n->vtable->name, n->vtable->size, n->firstChild, n->nextSibling, n->parent);
        printer->tabCount++;
        char* initialNewline = "\n";
        for (node* child = node_firstChild(n); child != NULL; child = node_nextSibling(child)) {
            total += linePrinter_stream(printer, initialNewline);
            initialNewline = "";
            total += child->vtable->format(child, printer);
            if (!printer->lineEdge) total += linePrinter_stream(printer, "\n");
        }
        printer->tabCount--;
        total += linePrinter_stream(printer, "}\n");
        return total;
    }

    static int node_print(node* n, void* od, aprintf of) {
        linePrinter printer;
        linePrinter_init(&printer, "\n", "  ", od, of);
        return n->vtable->format(n, &printer);
    }

    node_vtable node_defaultVtable = {
        .name="node",
        .size=sizeof(node),
        .format=node_format,
    };
    static void node_init(node* n, node_vtable* vtable) {
        n->vtable = vtable;
        n->nextSibling = 0;
        n->lastChild = 0;
        n->firstChild = 0;
        n->parent = 0;
    }

typedef struct {
    node* base;
    node* lastNode;
    int blockCapacity;
} nodeBase;
    static void nodeBase_init(nodeBase* b, int initialSize) {
        *b = (nodeBase){0};
        b->blockCapacity = MAX(initialSize, sizeof (node) * 2);
        b->base = malloc(b->blockCapacity); // blockCapacity is in bytes.
        b->lastNode = b->base;
        *b->lastNode = (node) {0};
        node n;
        node_init(&n, &node_defaultVtable);
        memmove(b->base, &n, n.vtable->size);
    }
    static int nodeBase_add(nodeBase* b, node* newNode) {
        uint8_t* base = (uint8_t*)b->lastNode;
        uint8_t* newNodeSlot = base + b->lastNode->vtable->size;
        newNodeSlot = maxalign(newNodeSlot);
        size_t offset = (uint8_t*)newNodeSlot - (uint8_t*)b->base;
        int minimumSize = offset + newNode->vtable->size;
        if (minimumSize >= b->blockCapacity) {
            b->blockCapacity *= 2;
            b->blockCapacity = MAX(b->blockCapacity, minimumSize);
            void* newBase = realloc(b->base, b->blockCapacity);
            b->base = newBase;
            newNodeSlot = (uint8_t*)b->base + offset;
        }
        memmove(newNodeSlot, newNode, newNode->vtable->size);
        b->lastNode = (node*) newNodeSlot;
        return offset / indexScale;
    }

    static int nodeBase_addChild(nodeBase* b, int parentIndex, node* newNode) {
        int newNodeIndex = nodeBase_add(b, newNode);
        newNode = node_from(b->base, newNodeIndex);
        node* parent = node_from(b->base, parentIndex);
        int newNodeOffset = node_between(parent, newNode);
        if (parent->firstChild == 0) {
            parent->firstChild = newNodeOffset;
            parent->lastChild = parent->firstChild;
        }
        node* lastChild = node_from(parent, parent->lastChild);
        lastChild->nextSibling = node_between(lastChild, newNode);
        parent->lastChild = node_between(parent, newNode);
        newNode->parent = parent->lastChild; // looks odd, but their offset to one another is equal, and the `parent` field is implicitly negative
        return node_between(b->base, newNode);
    }

    static int nodeBase_addSibling(nodeBase* b, int siblingIndex, node* newNode) {
        node* sibling = node_from(b->base, siblingIndex);
        int parentIndex = siblingIndex - sibling->parent;
        return nodeBase_addChild(b, parentIndex, newNode);
    }

    static node* nodeBase_getRoot(nodeBase* b) {
        return node_from(b->base, b->base->firstChild);
    }

    static void nodeBase_destroy(nodeBase* b) {
        if (b->base != NULL) {
            free(b->base);
        }
    }
#define init(nodename) node_vtable nodename##_vtable = {.name=#nodename, .size=sizeof(nodename), .format=nodename##_format} nodename* nodename##_init(nodename* n) {node_init((node*) n, &nodename##_vtable); return n;}
#define initf(nodename, formatfunc) node_vtable nodename##_vtable = {.name=#nodename, .size=sizeof(nodename), .format=formatfunc}; nodename* nodename##_init(nodename* n) {node_init((node*) n, &nodename##_vtable); return n;}


typedef struct startNode {
    node node;
} startNode;
    initf(startNode, node_format);

typedef struct programNode {
    node node;
} programNode;
    initf(programNode, node_format);

typedef struct blockNode {
    node node;
} blockNode;
    initf(blockNode, node_format);

typedef struct statementGroupNode {
    node node;
    int lastStatement;
} statementGroupNode;
    initf(statementGroupNode, node_format);

typedef struct statementNode {
    node node;
    int next;
} statementNode;
    initf(statementNode, node_format);

typedef struct declarationStatementNode {
    statementNode node;
} declarationStatementNode;
    initf(declarationStatementNode, node_format);

typedef int (*evalFunc)(void*);

typedef struct {
    node_vtable node;
    evalFunc eval;
} expressionNode_vtable;

typedef node expressionNode;
    static void expressionNode_init(expressionNode* n, expressionNode_vtable* vtable) {
        node_init((node*) n, (node_vtable*) vtable);
    }

    static inline int expressionNode_eval(expressionNode* n) {
        expressionNode_vtable* vtable = (expressionNode_vtable*) n->vtable;
        return vtable->eval(n);
    }

typedef struct {
    expressionNode node;
    int value;
} integerNode;
    int integerNode_format(node* node, linePrinter* printer) {
        /*
        integerNode* n = (integerNode*) node;
        linePrinter_stream(printer, "integer(%d)", n->value);
        */
        return node_format(node, printer);
    }

    static int integerNode_eval(void* nptr) {
        integerNode* n = (integerNode*) nptr;
        return n->value;
    }

    expressionNode_vtable integerNode_vtable = {
        .node = {
            .name="integerNode",
            .size=sizeof(integerNode),
            .format=integerNode_format,
        },
        .eval = integerNode_eval,
    };

    static void integerNode_init(integerNode* n, int inner) {
        expressionNode_init((expressionNode*) n, &integerNode_vtable);
        n->value = inner;
    }

typedef struct identifierNode {
    expressionNode node;
    symbols_t* symbols;
    uint32_t symbolIndex;
} identifierNode;

    static int identifierNode_eval(void* nptr) {
        identifierNode* n = (identifierNode*) nptr;
        symbol_t* sym = symbols_index(n->symbols, n->symbolIndex);
        return sym->v.value;
    }

    static int identifierNode_format(node* node, linePrinter* printer) {
        /*
        identifierNode* n = (identifierNode*) node;
        linePrinter_stream(printer, "%.*s (%d)", n->lexemeLen, n->lexeme, identifierNode_eval(n));
        */
        return node_format(node, printer);
    }

    expressionNode_vtable identifierNode_vtable = {
        .node = {
            .name="identifierNode",
            .size=sizeof(identifierNode),
            .format=identifierNode_format,
        },
        .eval = identifierNode_eval,
    };

    static void identifierNode_assign(identifierNode* n, int val) {
        symbol_t* sym = symbols_index(n->symbols, n->symbolIndex);
        sym->v.value = val;
    }

    static int identifierNode_getIndex(identifierNode* n) {
        (void) n;
        return 0;
    }

    static void identifierNode_declare(identifierNode* n) {
        symbols_index(n->symbols, n->symbolIndex)->v.initialized = true;
    }

    static void identifierNode_setValue(identifierNode* n, int v) {
        symbols_index(n->symbols, n->symbolIndex)->v.value = v;
    }

    static void identifierNode_init(identifierNode* n, char* lexeme, int lexemeLen, symbols_t* symbols) {
        expressionNode_init((expressionNode*) n, &identifierNode_vtable);
        int* indexPtr = symbols_getIndex(symbols, lexeme, lexemeLen);
        if (indexPtr == TABLE_NULL) {
            symbols_add(n->symbols, lexeme, lexemeLen, (symbol_t) {.type=variable_symbol, .v.initialized = false});
            indexPtr = symbols_getIndex(symbols, lexeme, lexemeLen);
        }
        n->symbolIndex = *indexPtr;
        n->symbols = symbols;
    }

typedef int (*arithmeticFunc)(int, int);
typedef struct {
    expressionNode_vtable node;
    arithmeticFunc operator;
} binaryOperatorNode_vtable;

typedef expressionNode binaryOperatorNode;

    static int binaryOperatorNode_format(node* n, linePrinter* printer) {
        /*
        binaryOperatorNode_vtable* table = (binaryOperatorNode_vtable*) n->vtable;
        linePrinter_stream(printer, "%s > (", table->node.node.name);
        expressionNode* lhs = (expressionNode*) node_firstChild((node*) n);
        expressionNode* rhs = (expressionNode*) node_nextSibling((node*) lhs);
        if (lhs != NULL) lhs->vtable->format(lhs, printer);
        else linePrinter_stream(printer, "(null)");
        linePrinter_stream(printer, ", ");
        if (rhs != NULL) rhs->vtable->format(rhs, printer);
        else linePrinter_stream(printer, "(null)");
        linePrinter_stream(printer, ")");
        */
        return node_format(n, printer);
    }
    
    static int binaryOperatorNode_eval(void* nptr) {
        binaryOperatorNode* n = (binaryOperatorNode*) nptr;
        binaryOperatorNode_vtable* table = (binaryOperatorNode_vtable*) n->vtable;
        expressionNode* lhs = (expressionNode*) node_firstChild((node*) n);
        expressionNode* rhs = (expressionNode*) node_nextSibling((node*) lhs);
        return table->operator(((expressionNode_vtable*)lhs->vtable)->eval(lhs), ((expressionNode_vtable*)rhs->vtable)->eval(rhs));
    }

    static void binaryOperatorNode_init(binaryOperatorNode* n, binaryOperatorNode_vtable* vtable) {
        vtable->node.eval = binaryOperatorNode_eval;
        expressionNode_init((expressionNode*) n, (expressionNode_vtable*) vtable);
    }

typedef struct {
    binaryOperatorNode node;
} plusOperatorNode;
    static int plusOperatorNode_op(int x, int y) {
        return x + y;
    }
    
    binaryOperatorNode_vtable plusOperatorNode_vtable = {
        .node={{.name="plus", .size=sizeof(plusOperatorNode), .format=binaryOperatorNode_format}},
        .operator=plusOperatorNode_op,
    };
    
    static void plusOperatorNode_init(plusOperatorNode* n) {
        binaryOperatorNode_init((binaryOperatorNode*) n, &plusOperatorNode_vtable);
    }

#define DEFINE_BINARY_OP(opname, op) \
typedef struct { binaryOperatorNode node; } opname##OperatorNode; \
static int opname##OperatorNode_op(int x, int y) { return x op y; } \
binaryOperatorNode_vtable opname##OperatorNode_vtable = {.node={{.name=#opname,.size=sizeof(opname##OperatorNode),.format=binaryOperatorNode_format}}, .operator=opname##OperatorNode_op}; \
static void opname##OperatorNode_init(opname##OperatorNode* n) { \
    binaryOperatorNode_init((binaryOperatorNode*) n, &opname##OperatorNode_vtable); \
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

struct nodeDescriptor {
    node* (*initFunc)(node*, ...);
    char* initFuncSchema;
    char* name;
};

#define desc(nodename, schema) (struct nodeDescriptor) {erase nodename##Node_init, schema, #nodename}
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
#define desc(nodename) (struct nodeDescriptor) {erase nodename##OperatorNode_init, "", #nodename}
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

