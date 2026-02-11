#pragma once

#include <stdio.h>
#include "table/table.h"
#include <stdio.h>
#include <string.h>
#include "table/table.h"
#define NAMESPACESIZE 4096


enum tokenType {
    VOID,
    MAIN,
    WHITESPACE,
    MOD,
    MODEQ,
    MINUS,
    MINUSEQ,
    XOR,
    XOREQ,
    BNOT,
    BNOTEQ,
    QUESTION,
    LBRACKET,
    RBRACKET,
    BOREQ,
    COLON,
    PLUSEQ,
    LT,
    LTEQ,
    GT,
    GTEQ,
    SHLEFT,
    SHRIGHT,
    BANDEQ,
    LAND,
    NEWLINE,
    STRING,
    INT,
    COUT,
    INSERTION,
    ASSIGNMENT,
    PLUS,
    DIVIDE,
    STAR,
    NOT,
    NOTEQ,
    EQ,
    AMP,
    DOT,
    SUB,
    COMMA,
    BOR,
    LOR,
    SEMICOLON,
    LPAREN,
    RPAREN,
    LCURLY,
    RCURLY,
    IDENTIFIER,
    INTEGER,
    FRACTION,
    LINECOMMENT,
    BLOCKCOMMENT,
    PREPROC,
    BAD,
    END,
    SENTINEL,
};

extern const char *tokenTypeStrings[SENTINEL] = {
    "VOID",
    "MAIN",
    "WHITESPACE",
    "MOD",
    "MODEQ",
    "MINUS",
    "MINUSEQ",
    "XOR",
    "XOREQ",
    "BNOT",
    "BNOTEQ",
    "QUESTION",
    "LBRACKET",
    "RBRACKET",
    "BOREQ",
    "COLON",
    "PLUSEQ",
    "LT",
    "LTEQ",
    "GT",
    "GTEQ",
    "SHLEFT",
    "SHRIGHT",
    "BANDEQ",
    "LAND",
    "NEWLINE",
    "STRING",
    "INT",
    "COUT",
    "INSERTION",
    "ASSIGNMENT",
    "PLUS",
    "DIVIDE",
    "STAR",
    "NOT",
    "NOTEQ",
    "EQ",
    "AMP",
    "DOT",
    "SUB",
    "COMMA",
    "BOR",
    "LOR",
    "SEMICOLON",
    "LPAREN",
    "RPAREN",
    "LCURLY",
    "RCURLY",
    "IDENTIFIER",
    "INTEGER",
    "FRACTION",
    "LINECOMMENT",
    "BLOCKCOMMENT",
    "PREPROC",
    "BAD",
    "END",
};

extern const char* tokenTypeColors[SENTINEL] = {
    "\033[38;2;86;156;214m",   // VOID - blue (keyword)
    "\033[38;2;86;156;214m",   // MAIN - blue (keyword)
    "\033[38;2;212;212;212m",  // WHITESPACE - light gray
    "\033[38;2;212;212;212m",  // MOD - light gray (operator)
    "\033[38;2;212;212;212m",  // MODEQ - light gray (operator)
    "\033[38;2;212;212;212m",  // MINUS - light gray (operator)
    "\033[38;2;212;212;212m",  // MINUSEQ - light gray (operator)
    "\033[38;2;212;212;212m",  // XOR - light gray (operator)
    "\033[38;2;212;212;212m",  // XOREQ - light gray (operator)
    "\033[38;2;212;212;212m",  // BNOT - light gray (operator)
    "\033[38;2;212;212;212m",  // BNOTEQ - light gray (operator)
    "\033[38;2;212;212;212m",  // QUESTION - light gray (operator)
    "\033[38;2;255;215;0m",    // LBRACKET - gold
    "\033[38;2;255;215;0m",    // RBRACKET - gold
    "\033[38;2;212;212;212m",  // BOREQ - light gray (operator)
    "\033[38;2;212;212;212m",  // COLON - light gray
    "\033[38;2;212;212;212m",  // PLUSEQ - light gray (operator)
    "\033[38;2;212;212;212m",  // LT - light gray (operator)
    "\033[38;2;212;212;212m",  // LTEQ - light gray (operator)
    "\033[38;2;212;212;212m",  // GT - light gray (operator)
    "\033[38;2;212;212;212m",  // GTEQ - light gray (operator)
    "\033[38;2;212;212;212m",  // SHLEFT - light gray (operator)
    "\033[38;2;212;212;212m",  // SHRIGHT - light gray (operator)
    "\033[38;2;212;212;212m",  // BANDEQ - light gray (operator)
    "\033[38;2;212;212;212m",  // LAND - light gray (operator)
    "\033[38;2;212;212;212m",  // NEWLINE - light gray
    "\033[38;2;106;153;85m",   // STRING - green (comment)
    "\033[38;2;86;156;214m",   // INT - blue (keyword)
    "\033[38;2;220;220;170m",  // COUT - yellow (standard library)
    "\033[38;2;212;212;212m",  // INSERTION - light gray (operator)
    "\033[38;2;212;212;212m",  // ASSIGNMENT - light gray (operator)
    "\033[38;2;212;212;212m",  // PLUS - light gray (operator)
    "\033[38;2;212;212;212m",  // DIVIDE - light gray (operator)
    "\033[38;2;212;212;212m",  // STAR - light gray (operator)
    "\033[38;2;212;212;212m",  // NOT - light gray (operator)
    "\033[38;2;212;212;212m",  // NOTEQ - light gray (operator)
    "\033[38;2;212;212;212m",  // EQ - light gray (operator)
    "\033[38;2;212;212;212m",  // AMP - light gray (operator)
    "\033[38;2;212;212;212m",  // DOT - light gray
    "\033[38;2;212;212;212m",  // SUB - light gray
    "\033[38;2;212;212;212m",  // COMMA - light gray
    "\033[38;2;212;212;212m",  // BOR - light gray
    "\033[38;2;212;212;212m",  // LOR - light gray
    "\033[38;2;212;212;212m",  // SEMICOLON - light gray
    "\033[38;2;255;215;0m",    // LPAREN - gold
    "\033[38;2;255;215;0m",    // RPAREN - gold
    "\033[38;2;255;215;0m",    // LCURLY - gold
    "\033[38;2;255;215;0m",    // RCURLY - gold
    "\033[38;2;156;220;254m",  // IDENTIFIER - light blue
    "\033[38;2;181;206;168m",  // INTEGER - light green
    "\033[38;2;181;206;168m",  // FRACTION - light green
    "\033[38;2;106;153;85m",   // LINECOMMENT - green (comment)
    "\033[38;2;106;153;85m",   // BLOCKCOMMENT - green (comment)
    "\033[38;2;106;153;85m",   // PREPROC - green
    "\033[38;2;244;71;71m",    // BAD - red (error)
    "\033[38;2;128;128;128m",  // END - gray
};

struct token {
    int type;
    char* lexeme;
    int lexemeLen;
};

typedef struct token token_t;

table_t reservedWords = {0};

void initReserved() {
    table_init(&reservedWords, NAMESPACESIZE);
    *table_insert(&reservedWords, "void", 4) = VOID;
    *table_insert(&reservedWords, "main", 4) = MAIN;
    *table_insert(&reservedWords, "cout", 4) = COUT;
    *table_insert(&reservedWords, "int", 3) = INT;
}

static int token_sdebug(token_t* t, char* output, int length) {
    return snprintf(output, length, "%s \"%.*s\"\n", tokenTypeStrings[t->type], t->lexemeLen, t->lexeme);
}

static int token_fdebug(token_t* t, FILE* out) {
    return fprintf(out, "%s \"%.*s\"\n", tokenTypeStrings[t->type], t->lexemeLen, t->lexeme);
}

static int token_debug(token_t* t) {
    return printf("%s \"%.*s\"\n", tokenTypeStrings[t->type], t->lexemeLen, t->lexeme);
}

static int token_debugPrettyPrint(token_t* t) {
    printf("(%s)%s%.*s\033[0m", tokenTypeStrings[t->type], tokenTypeColors[t->type], t->lexemeLen, t->lexeme);
    fflush(stdout);
}

static int token_prettyPrint(token_t* t) {
    printf("%s%.*s\033[0m", tokenTypeColors[t->type], t->lexemeLen, t->lexeme);
    fflush(stdout);
}

static void token_init(token_t* t, char* lexeme, int lexemeLen, int type) {
    if (reservedWords.entries == NULL) initReserved();
    int* tokEntry = table_lookup(&reservedWords, lexeme, lexemeLen);
    if (tokEntry != TABLE_NULL) type = *tokEntry;
    t->lexeme = lexeme;
    t->lexemeLen = lexemeLen;
    t->type = type;
}
