#pragma once
#include "token.h"
#include "table/table.h"
#include "stateMachine.h"
#include "scanner.h"
#include "symbols.h"
#include "nodes.h"
#include "nodeStreaming.h"
#include "stringStreaming/stringstream.h"
#include "utils.h"

#include <sys/param.h>

#define ANNOUNCE printf("entering %s\n", __func__)

#define SUCCESS printf("%s %s\n", success ? "SUCCEEDED" : "FAILED", __func__)

// will later hold information 
typedef struct {
} errorState_t;

typedef struct {
    scannerState_t scannerState;
} parserState_t;

typedef struct {
    symbols_t symbols;
    scanner_t scanner;
    parserState_t savedState;
    nodeBase_t nodes;
    int nodeCursor;
} parser_t;

static int parser_addNode(parser_t* p, node* n) {
    int predecessorIndex = p->nodeCursor;
    node* predecessor = node_from(p->nodes.base, predecessorIndex);
    if (predecessor->flags & dangling) {
        int newNodeIndex = nodeBase_add(&p->nodes, n);
        // adding to a nodeBase can make pointers taken from it stale
        predecessor = node_from(p->nodes.base, predecessorIndex);
        node* newNode = node_from(p->nodes.base, newNodeIndex);
        newNode->predecessor = node_between(newNode, predecessor);
        p->nodeCursor = newNodeIndex;
    }
    else {
        p->nodeCursor = nodeBase_addSibling(&p->nodes, p->nodeCursor, n);
    }
    return p->nodeCursor;
}

// add a new batch that will be claimed by the next call to `parser_claimNodes`, and links the latest-added as the left sibling of the yet-nonexistent parent.
// next node will be added as an orphan, referring to last-added as its "predecessor"; future nodes will become siblings with that orphan.
// claimNodes claims all orpans as children, and links predecessor as left sibling except when predecessorRepeat > 1
// when predecessorRepeat > 1, parent decrements it and adds itself without a left sibling
static void parser_nestNodes(parser_t* p) {
    node* predecessor = node_from(p->nodes.base, p->nodeCursor);
    if (predecessor->flags & dangling) {
        predecessor->predecessorRepeat++;
    }
    predecessor->flags |= dangling;
}

// claim the last batch of nodes on behalf of the new parent, and restore the last save-state
static void parser_claimNodes(parser_t* p, node* n) {
    node* youngest = node_from(p->nodes.base, p->nodeCursor);
    node* eldest = node_oldestSibling(youngest);
    node* predecessor = node_from(eldest, eldest->predecessor);
    if (predecessor->predecessorRepeat <= 0) {
        predecessor->flags &= ~dangling;
    }
    else {
        predecessor->predecessorRepeat--;
    }
    p->nodeCursor = node_between(p->nodes.base, predecessor);
    int youngestIndex = node_between(p->nodes.base, youngest);
    int newParentIndex = parser_addNode(p, n);
    node* newParent = node_from(p->nodes.base, newParentIndex);
    newParent->lastChild = youngestIndex - newParentIndex;
}

// reject a prior nesting for just one node; do nothing if > 1
static void parser_claimIdentity(parser_t* p) {
    node* youngest = node_from(p->nodes.base, p->nodeCursor);
    node* eldest = node_oldestSibling(youngest);
    if (youngest != eldest) return;
    node* predecessor = node_from(eldest, eldest->predecessor);
    if (predecessor->predecessorRepeat <= 0) {
        predecessor->flags &= ~dangling;
    }
    else {
        predecessor->predecessorRepeat--;
    }
    if (!(predecessor->flags & dangling)) {
        predecessor->nextSibling = node_between(predecessor, youngest);
        youngest->flags = 0;
        youngest->oldestSibling = node_between(youngest, node_oldestSibling(predecessor));
    }
}

static void parser_deNestNodes(parser_t* p) {
    node* predecessor;
    node* youngest = node_from(p->nodes.base, p->nodeCursor);
    if (youngest->flags & dangling) predecessor = youngest;
    else {
        node* eldest = node_oldestSibling(youngest);
        predecessor = node_from(p->nodes.base, p->nodeCursor);
    }
    if (predecessor->predecessorRepeat <= 0) {
        predecessor->flags &= ~dangling;
    }
    else {
        predecessor->predecessorRepeat--;
    }
    if (!(predecessor->flags & dangling)) {
        p->nodeCursor = node_between(p->nodes.base, predecessor);
        p->nodes.lastNode = predecessor;
    }
}

static parser_t* parser_init(parser_t* p, char* filename) {
    *p = (parser_t) {0};
    symbols_init(&p->symbols, 16);
    scanner_init(&p->scanner, filename, &p->symbols);
    nodeBase_init(&p->nodes, 1);
}

static void parser_destroy(parser_t* p) {
    symbols_destroy(&p->symbols);
    scanner_destroy(&p->scanner);
    nodeBase_destroy(&p->nodes);
}

static parserState_t parser_saveAnchor(parser_t* p) {
    parserState_t old = p->savedState;
    p->savedState = (parserState_t){.scannerState=p->scanner.state};
    return old;
}

static void parser_revert(parser_t* p) {
    scanner_revert(&p->scanner, p->savedState.scannerState);
}

static void parser_restoreAnchor(parser_t* p, parserState_t oldState) {
    p->savedState = oldState;
}

static void parser_fail(parser_t* p, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    parser_revert(p);
    va_end(args);
}

static token_t parser_getToken(parser_t* p, bool allowWhitespace) {
    token_t tok = {.type=whitespace_token};
    while (tok.type == whitespace_token || tok.type == newline_token) {
        tok = scanner_getNextToken(&p->scanner);
        if (allowWhitespace) break;
    }
    return tok;
}

static bool parser_boolMatch(parser_t* p, enum tokenType typ) {
    token_t parser_match = {.type=typ, .lexeme="", .lexemeLen=0};
    token_t tok = parser_getToken(p, 0);
    if (tok.type != typ) {
        parser_fail(p, "failed to match token types %s, %s\n", getTokenString(&tok), getTokenString(&parser_match));
        return false;
    }
    printf("matched token type %s\n", getTokenString(&parser_match));
    return true;
}

static token_t parser_tokenMatch(parser_t* p, enum tokenType typ) {
    token_t match = {.type=typ, .lexeme="", .lexemeLen=0};
    token_t tok = parser_getToken(p, 0);
    if (tok.type != typ) {
        parser_fail(p, "failed to match token types %s, %s\n", getTokenString(&tok), getTokenString(&match));
        return (token_t) {.type=bad_token, .lexeme=NULL, .lexemeLen=0};
    }
    printf("matched token type %s\n", getTokenString(&match));
    return tok;
}

static token_t parser_firstMatch(parser_t* p, int typeCount, tokenType_t types[]) {
    token_t tok = parser_getToken(p, 0);
    for (int i = 0; i < typeCount; i++) {
        token_t match = {.type=types[i], .lexeme="", .lexemeLen=0};
        if (tok.type != match.type) {
            continue;
        }
        printf("matched token type %s\n", getTokenString(&match));
        return tok;
    }
    parser_fail(p, "failed to match any token type.\n");
    return (token_t) {.type=bad_token, .lexeme=NULL, .lexemeLen=0};
}

static bool parser_matchIdentifier(parser_t* p, char* lexeme, int lexemeLen) {
    token_t tok = parser_getToken(p, 0);
    if ((tok.type != identifier_token) || (strncmp(tok.lexeme, lexeme, (((tok.lexemeLen)<(lexemeLen))?(tok.lexemeLen):(lexemeLen))) != 0)) {
        parser_fail(p, "failed to match identifier on lexeme %.*s", lexemeLen, lexeme);
        return false;
    }
    printf("matched identifier %.*s\n", lexemeLen, lexeme);
    return true;
}

static bool parser_integer(parser_t* p) {
    ANNOUNCE;
    parserState_t save = parser_saveAnchor(p);
    static tokenType_t integerLiteralTypes[] = {
        decimal_token,
        decimal_u_token,
        decimal_l_token,
        decimal_lu_token,
        decimal_ll_token,
        decimal_ull_token,
        decimal_wb_token,
        octal_token,
        oct_u_token,
        oct_l_token,
        oct_lu_token,
        oct_ll_token,
        oct_ull_token,
        oct_wb_token,
        oct_uwb_token,
        hexadecimal_token,
        hex_u_token,
        hex_l_token,
        hex_lu_token,
        hex_ll_token,
        hex_ull_token,
        hex_wb_token,
        hex_uwb_token
    };

    token_t tok = parser_firstMatch(
        p, 
        sizeof(integerLiteralTypes) / sizeof(integerLiteralTypes[0]), 
        integerLiteralTypes
    );
    bool success = tok.type != bad_token;
    if (success) {
        int val = strtol(tok.lexeme, NULL, 0);

        integerNode* n = integerNode_init(stack(integerNode), val);
        parser_addNode(p, &n->node);
    }

    SUCCESS;
    parser_restoreAnchor(p, save);
    return success;
}

static bool parser_identifier(parser_t* p) {
    ANNOUNCE;
    token_t tok = parser_tokenMatch(p, identifier_token);
    bool success = tok.type != bad_token;
    if (success) {
        identifierNode* n = identifierNode_init(stack(identifierNode), tok.lexeme, tok.lexemeLen, &p->symbols);
        parser_addNode(p, &n->node);
    }
    SUCCESS;
    return success;
}

static bool parser_expression(parser_t* p);

static bool parser_factor(parser_t* p) {
    ANNOUNCE;
    parserState_t save = parser_saveAnchor(p);
    parser_nestNodes(p);
    bool success =
        parser_identifier(p) ||
        parser_integer(p) ||
        (parser_boolMatch(p, lparen_token) && parser_expression(p) && parser_boolMatch(p, rparen_token))
    ;
    if (!success) {
        parser_deNestNodes(p);
    }
    parser_restoreAnchor(p, save);
    SUCCESS;
    return success;
}

typedef enum precedence {
    divmul,
    addsub,
    equality,
    and,
    or,
    bad
} precedence;

node* parser_getNode(parser_t* p, token_t template, nodeAny_t* in) {
    node* storage = (node*) in;
    switch (template.type) {
        case star_token:
            return storage = timesOperatorNode_init(erase storage);
        case div_token:
            return storage = divideOperatorNode_init(erase storage);
        case plus_token:
            return storage = plusOperatorNode_init(erase storage);
        case minus_token:
            return storage = minusOperatorNode_init(erase storage);
        case langle_token:
            return storage = ltOperatorNode_init(erase storage);
        case le_token:
            return storage = leOperatorNode_init(erase storage);
        case rangle_token:
            return storage = gtOperatorNode_init(erase storage);
        case ge_token:
            return storage = geOperatorNode_init(erase storage);
        case eq_token:
            return storage = eqOperatorNode_init(erase storage);
        case noteq_token:
            return storage = neqOperatorNode_init(erase storage);
        case and_token:
            return storage = andOperatorNode_init(erase storage);
        case lor_token:
            return storage = orOperatorNode_init(erase storage);
        case bad_token:
        default:
            return NULL;
    }
}

precedence getPrecedence(tokenType_t operator) {
    switch (operator) {
        case star_token:
        case div_token:
            return divmul;
        case plus_token:
        case minus_token:
            return addsub;
        case langle_token:
        case le_token:
        case rangle_token:
        case ge_token:
        case eq_token:
        case noteq_token:
            return equality;
        case and_token:
            return and;
        case lor_token:
            return or;
        case bad_token:
        default:
            return bad;
    }
}

static bool parser_operator(parser_t* p, token_t* out) {
    bool returnValue = true;
    parserState_t save = parser_saveAnchor(p);
    token_t operator = parser_getToken(p, false);
    precedence prec = getPrecedence(operator.type);
    if (prec >= bad) goto fail;
    goto cleanup;
fail:
    returnValue = false;
    parser_fail(p, "failed to get operator; got %s.\n", getTokenString(&operator));
cleanup:
    *out = operator;
    parser_restoreAnchor(p, save);
    return returnValue;
}

static int parser_operationTail(parser_t* p, token_t* io) {
    int factorsParsed = 0;
    token_t base = *io;
    precedence refPrecedence = getPrecedence(base.type);

    token_t candidate;
    // `parser_factor` automatically adds to the AST if it succeeds;
    // each of _factor and _operator automatically reverts to the last anchor if it fails.
    // _saveAnchor should be called as soon as a taken path can only be defied by a fatal error.
    factorsParsed += parser_factor(p);
    if (factorsParsed == 0) return factorsParsed;
    parserState_t save = parser_saveAnchor(p);
    bool gotOperator = parser_operator(p, &candidate);
    bool success = gotOperator;
    int factorsThisLoop;
    while (getPrecedence(candidate.type) <= refPrecedence) {
        factorsThisLoop = parser_operationTail(p, &candidate);
        if (factorsThisLoop < 1) break;
        factorsParsed += factorsThisLoop;
    }
    // if there is a dangling factor on the AST, `_claimIdentity` de-nests it.
    //parser_claimIdentity(p);
    node* newOperator = parser_getNode(p, base, stack(nodeAny_t));
    parser_claimNodes(p, newOperator);
    *io = candidate;
    // anchors say, "what state do I return to if I fail?". saving/restoring those templates does not revert actual state.
    // reverting to the caller's template unconditionally is good; the calling context is responsible re-save it as soon as they need it to be recent.
    parser_restoreAnchor(p, save);
    return success;
}

static bool parser_expression(parser_t* p) {
    ANNOUNCE;
    parserState_t save = parser_saveAnchor(p);
    int factorsParsed = parser_operationTail(p, stackval(token_t, .type=bad_token));
    bool success = factorsParsed >= 1;
    parser_restoreAnchor(p, save);
    SUCCESS;
    return success;
}

static bool parser_declarationStatement(parser_t* p) {
    ANNOUNCE;
    parser_nestNodes(p);
    bool success =
        parser_matchIdentifier(p, "int", sizeof "int" - 1) && 
        parser_identifier(p) &&
        parser_boolMatch(p, semicolon_token)
    ;
    if (success) {
        declarationStatementNode* n = declarationStatementNode_init(stack(declarationStatementNode));
        parser_claimNodes(p, n);
    }
    else parser_deNestNodes(p);
    SUCCESS;
    return success;
}

static bool parser_assignmentStatement(parser_t* p) {
    ANNOUNCE;
    parser_nestNodes(p);
    bool success =
        parser_identifier(p) &&
        parser_boolMatch(p, assignment_token) &&
        parser_expression(p) &&
        parser_boolMatch(p, semicolon_token)
    ;
    if (success) {
        assignmentStatementNode* n = assignmentStatementNode_init(stack(assignmentStatementNode));
        parser_claimNodes(p, n);
    }
    else parser_deNestNodes(p);
    SUCCESS;
    return success;
}

static bool parser_coutStatement(parser_t* p) {
    ANNOUNCE;
    bool success =
        parser_matchIdentifier(p, "cout", sizeof "cout" - 1) &&
        parser_boolMatch(p, shiftleft_token) &&
        parser_expression(p) &&
        parser_boolMatch(p, semicolon_token)
    ;
    SUCCESS;
    return success;
}

static bool parser_block(parser_t* p);

static bool parser_statement(parser_t* p) {
    ANNOUNCE;
    parserState_t save = parser_saveAnchor(p);
    bool success =
        parser_coutStatement(p) ||
        parser_declarationStatement(p) ||
        //parser_block(p) ||
        parser_assignmentStatement(p)
    ;
    parser_restoreAnchor(p, save);
    SUCCESS;
    return success;
}

static bool parser_statementGroup(parser_t* p) {
    ANNOUNCE;
    parserState_t save = parser_saveAnchor(p);
    bool success =
        (parser_statement(p) && parser_statementGroup(p))
    ;
    parser_restoreAnchor(p, save);
    success = true;
    SUCCESS;
    return success;
}

static bool parser_block(parser_t* p) {
    parser_nestNodes(p);
    ANNOUNCE;
    bool success =
        parser_boolMatch(p, lcurly_token) &&
        parser_statementGroup(p) &&
        parser_boolMatch(p, rcurly_token)
    ;
    SUCCESS;
    if (success) {
        blockNode* b = blockNode_init(stack(blockNode));
        parser_claimNodes(p, b);
    }
    else parser_deNestNodes(p);
    return success;
}

static bool parser_program(parser_t* p) {
    parser_nestNodes(p);
    ANNOUNCE;
    bool success =
        parser_matchIdentifier(p, "int", sizeof "int" - 1) &&
        parser_matchIdentifier(p, "main", sizeof "main" - 1) &&
        parser_boolMatch(p, lparen_token) &&
        parser_boolMatch(p, rparen_token) &&
        parser_block(p)
    ;
    if (success) {
        programNode* program = programNode_init(stack(programNode));
        parser_claimNodes(p, program);
        program = (programNode*) node_from(p->nodes.base, p->nodeCursor);
        node_print(program, erase stdout, erase fprintf);
    }
    else parser_deNestNodes(p);
    SUCCESS;
    return success;
}

static bool parser_start(parser_t* p) {
    ANNOUNCE;
    parserState_t save = parser_saveAnchor(p);
    bool success =
        parser_program(p) &&
        parser_boolMatch(p, eof_token)
    ;
    SUCCESS;
    return success;
}
