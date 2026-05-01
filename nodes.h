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

#define MAXNODESIZE 64

#define maxalign(ptr) (void*)((intptr_t)(((uint8_t*)ptr) + (__alignof (max_align_t) - 1)) & (~(__alignof (max_align_t) - 1 )))
typedef void* erased;

#define indexScale (__alignof(max_align_t))

typedef max_align_t align;

typedef struct node node;

typedef struct {
    char* name;
    int size;
    int (*format)(node*, linePrinter*);
    int (*evaluate)(void*);
} node_vtable;

typedef enum relationshipFlags {
    eldest=1,
    orphan=1<<1,
    dangling=1<<2,
} relationshipFlags;

// indices are relative
// indices are scaled up by `indexScale` before use because an index must be aligned with `max_align_t`.
struct node {
    node_vtable* vtable; // could later become an index (though pointer is likely the right tradeoff here. Index needs to answer "index into what?" using either global state or context that the node becomes useless without.)
    int16_t lastChild;
    // a node's size is inside its vtable. If a node cannot have siblings and does not care about its parent, it can refrain from allocating enough size for these members.
    uint16_t flags;
    union {
        int16_t nextSibling; // active if (flags & !dangling)
        uint16_t predecessorRepeat; // inverse of above
    };
    union {
        int16_t parent; // active if (eldest & flags) && !(orphan & flags)
        int16_t oldestSibling; // active if !(eldest & flags)
        int16_t predecessor; // active if (eldest & flags) && (orphan & flags)
    };
};
    static node* node_from(node* n, int16_t offset) {
        return (node*) ((uint8_t*) n + (offset * indexScale));
    }
    static int16_t node_between(node* n1, node* n2) {
        return (int16_t)(((intptr_t) n2 - (intptr_t) n1) / indexScale);
    }
    static node* node_firstChild(node* n) {
        if (n->lastChild == 0) return NULL;
        n = node_from(n, n->lastChild);
        if (n->flags & eldest) return n;
        else return node_from(n, n->oldestSibling);
    }
    static node* node_nextSibling(node* n) {
        if (n->nextSibling == 0) return NULL;
        return node_from(n, n->nextSibling);
    }
    static node* node_oldestSibling(node* n) {
        if (n->flags & eldest) return n;
        return node_from(n, n->oldestSibling);
    }
    static node* node_parent(node* n) {
        if (!(n->flags & eldest)) {
            n = node_from(n, n->oldestSibling);
        }
        if (n->flags & orphan) return NULL;
        if (n->parent == 0) return NULL;
        return node_from(n, n->parent);
    }
    static int node_format(node* n, linePrinter* printer) {
        int total = 0;
        total += linePrinter_stream(printer, "# %s(size: %d) # {", n->vtable->name, n->vtable->size);
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

    static int node_evaluate(void* v) {
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
    static node* node_init(node* n, node_vtable* vtable, bool repoint) {
        n->vtable = vtable;
        if (repoint) {
            n->nextSibling = 0;
            n->lastChild = 0;
            n->parent = 0;
        }
        return n;
    }

enum nodeBase_direction {
    CHILDFIRST=-1,  // demands offset from child to parent is positive
    PARENTFIRST=1,  // demands offset from child to parent is negative
    ABSOLUTE        // offset is absolute from beginning of node storage
};

typedef struct {
    node* base;
    node* lastNode;
    int blockCapacity;
} nodeBase_t;
    static void nodeBase_init(nodeBase_t* b, int initialSize) {
        *b = (nodeBase_t){0};
        b->blockCapacity = MAX(initialSize, sizeof (node) * 2);
        b->base = malloc(b->blockCapacity); // blockCapacity is in bytes.
        b->lastNode = b->base;
        *b->lastNode = (node) {0};
        node n;
        node_init(&n, &node_defaultVtable, true);
        memmove(b->base, &n, n.vtable->size);
    }

    static int nodeBase_nextOffset(nodeBase_t* b) {
        uint8_t* base = (uint8_t*)b->lastNode;
        uint8_t* newNodeSlot = base + b->lastNode->vtable->size;
        newNodeSlot = maxalign(newNodeSlot);
        int offset = (uint8_t*)newNodeSlot - (uint8_t*)b->base;
        return offset;
    }

    static int nodeBase_thisOffset(nodeBase_t* b) {
        uint8_t* base = (uint8_t*)b->lastNode;
        int offset = base - (uint8_t*)b->base;
        return offset;
    }

    static int nodeBase_nextIndex(nodeBase_t* b) {
        return nodeBase_nextOffset(b) / indexScale;
    }

    static int nodeBase_thisIndex(nodeBase_t* b) {
        return nodeBase_thisOffset(b) / indexScale;
    }

    static int nodeBase_add(nodeBase_t* b, node* newNode) {
        int predecessorIndex = nodeBase_thisIndex(b);
        int offset = nodeBase_nextOffset(b);
        int index = nodeBase_nextIndex(b);
        int minimumSize = offset + newNode->vtable->size;
        if (minimumSize >= b->blockCapacity) {
            b->blockCapacity *= 2;
            b->blockCapacity = MAX(b->blockCapacity, minimumSize);
            void* newBase = realloc(b->base, b->blockCapacity);
            b->base = newBase;
        }
        uint8_t* newNodeSlot = (uint8_t*)b->base + offset;
        newNode->flags = eldest | orphan;
        newNode->predecessor = predecessorIndex - index;
        memmove(newNodeSlot, newNode, newNode->vtable->size);
        b->lastNode = (node*) newNodeSlot;
        return index;
    }

    static int nodeBase_addSibling(nodeBase_t* b, int siblingIndex, node* newNode) {
        int newNodeIndex = nodeBase_add(b, newNode);
        newNode = node_from(b->base, newNodeIndex);
        node* sibling = node_from(b->base, siblingIndex);
        newNode->flags &= ~(eldest | orphan);
        int offset = newNodeIndex - siblingIndex;
        sibling->nextSibling = offset;
        if (sibling->flags & eldest) newNode->oldestSibling = -offset;
        else newNode->oldestSibling = sibling->oldestSibling - offset;
        return newNodeIndex;
    }

    static int nodeBase_addChild(nodeBase_t* b, int parentIndex, node* newNode) {
        node* parent = node_from(b->base, parentIndex);
        if (parent->lastChild == 0) {
            int newNodeIndex = nodeBase_add(b, newNode);
            parent = node_from(b->base, parentIndex);
            newNode = node_from(b->base, newNodeIndex);
            newNode->flags &= ~orphan;
            newNode->parent = parentIndex - newNodeIndex;
            parent->lastChild = newNodeIndex - parentIndex;
            newNode->flags |= eldest;
            return newNodeIndex;
        }
        else {
            int lastChildIndex = parentIndex + parent->lastChild;
            return nodeBase_addSibling(b, lastChildIndex, newNode);
        }
    }

    static node* nodeBase_getRoot(nodeBase_t* b) {
        return node_firstChild(b->base);
    }

    static void nodeBase_destroy(nodeBase_t* b) {
        if (b->base != NULL) {
            free(b->base);
        }
    }
#define init(nodename, formatfunc) node_vtable nodename##_vtable = {.name=#nodename, .size=sizeof(nodename), .format=formatfunc}; nodename* nodename##_init(nodename* n) {return node_init((node*) n, &nodename##_vtable, true); return n;}

#define VTABLEDEFAULTS .size=sizeof(node), .format=node_format, .evaluate=node_evaluate


typedef node startNode;
init(startNode, node_format);

typedef node programNode;
init(programNode, node_format);

typedef node blockNode;
init(blockNode, node_format);

typedef node statementGroupNode;
init(statementGroupNode, node_format);

typedef node statementNode;
init(statementNode, node_format);

typedef node declarationStatementNode;
init(declarationStatementNode, node_format);

typedef node assignmentStatementNode;
init(assignmentStatementNode, node_format);

typedef node coutStatementNode;
init(coutStatementNode, node_format);

typedef int (*evalFunc)(void*);

typedef struct {
    node_vtable node;
    evalFunc eval;
} expressionNode_vtable;

typedef node expressionNode;
    static expressionNode* expressionNode_init(expressionNode* n, expressionNode_vtable* vtable) {
        return node_init((node*) n, (node_vtable*) vtable, true);
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
        integerNode* n = (integerNode*) node;
        return linePrinter_stream(printer, "integer(%d)", n->value);
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

    static integerNode* integerNode_init(integerNode* n, int inner) {
        expressionNode_init((expressionNode*) n, &integerNode_vtable);
        n->value = inner;
    }

typedef struct identifierNode {
    expressionNode node;
    symbols_t* symbols;
    uint32_t symbolIndex;
    char* lexeme;
    int lexemeLen;
} identifierNode;

    static int identifierNode_eval(void* nptr) {
        identifierNode* n = (identifierNode*) nptr;
        symbol_t* sym = symbols_index(n->symbols, n->symbolIndex);
        return sym->v.value;
    }

    static int identifierNode_format(node* node, linePrinter* printer) {
        identifierNode* n = (identifierNode*) node;
        return linePrinter_stream(printer, "%.*s (%d)", n->lexemeLen, n->lexeme, identifierNode_eval(n));
        /*
        return node_format(node, printer);
        */
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

    static identifierNode* identifierNode_init(identifierNode* n, char* lexeme, int lexemeLen, symbols_t* symbols) {
        expressionNode_init((expressionNode*) n, &identifierNode_vtable);
        int* indexPtr = symbols_getIndex(symbols, lexeme, lexemeLen);
        n->symbols = symbols;
        n->lexeme = lexeme;
        n->lexemeLen = lexemeLen;
        if (indexPtr == TABLE_NULL) {
            symbols_add(n->symbols, lexeme, lexemeLen, (symbol_t) {.type=variable_symbol, .v.initialized = false});
            indexPtr = symbols_getIndex(symbols, lexeme, lexemeLen);
        }
        n->symbolIndex = *indexPtr;
        return n;
    }

typedef int (*arithmeticFunc)(int, int);
typedef struct {
    expressionNode_vtable node;
    arithmeticFunc operator;
} binaryOperatorNode_vtable;

typedef expressionNode binaryOperatorNode;
    static int binaryOperatorNode_format(node* n, linePrinter* printer) {
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
    }
    
    static int binaryOperatorNode_eval(void* nptr) {
        binaryOperatorNode* n = (binaryOperatorNode*) nptr;
        binaryOperatorNode_vtable* table = (binaryOperatorNode_vtable*) n->vtable;
        expressionNode* lhs = (expressionNode*) node_firstChild((node*) n);
        expressionNode* rhs = (expressionNode*) node_nextSibling((node*) lhs);
        return table->operator(((expressionNode_vtable*)lhs->vtable)->eval(lhs), ((expressionNode_vtable*)rhs->vtable)->eval(rhs));
    }

    static binaryOperatorNode* binaryOperatorNode_init(binaryOperatorNode* n, binaryOperatorNode_vtable* vtable) {
        vtable->node.eval = binaryOperatorNode_eval;
        return (binaryOperatorNode*) expressionNode_init((expressionNode*) n, (expressionNode_vtable*) vtable);
    }

#define DEFINE_BINARY_OP(opname, op) \
typedef binaryOperatorNode opname##OperatorNode; \
static int opname##OperatorNode_op(int x, int y) { return x op y; } \
binaryOperatorNode_vtable opname##OperatorNode_vtable = {.node={{.name=#opname,.size=sizeof(opname##OperatorNode),.format=binaryOperatorNode_format}}, .operator=opname##OperatorNode_op}; \
static opname##OperatorNode* opname##OperatorNode_init(opname##OperatorNode* n) { \
    return (opname##OperatorNode*) binaryOperatorNode_init((binaryOperatorNode*) n, &opname##OperatorNode_vtable); \
}

DEFINE_BINARY_OP(plus, +);
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
    desc(lt),
};

// TODO: This union is mainly used because it is guaranteed to have the largest size
// among any of these nodes. New nodes must perpetually be added. There is no native
// C method for ensuring this happens. Leaf nodes that do not have additional members
// (node types that are a typedef of another type) do not have to be added.
typedef union {
    node node;
    expressionNode expressionNode;
    binaryOperatorNode binaryOperatorNode;
    programNode programNode;
    blockNode blockNode;
    statementGroupNode statementGroupNode;
    statementNode statementNode;
    declarationStatementNode declarationStatementNode;
    integerNode integerNode;
    identifierNode identifierNode;
} nodeAny_t;

#undef init
#undef desc
#undef DEFINE_BINARY_OP
