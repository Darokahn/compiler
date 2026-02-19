#pragma once

#include <stdio.h>
#include "table/table.h"
#include <stdio.h>
#include <string.h>
#include "table/table.h"
#include "stateMachineDefs.h"
#define NAMESPACESIZE 4096

struct token {
    int type;
    char* lexeme;
    int lexemeLen;
};

typedef struct token token_t;

table_t reservedWords = {0};

char* generalKeywords[] = {
    "auto",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "int",
    "long",
    "register",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "unsigned",
    "void",
    "volatile",
    "while"
};

enum tokentypeExtension {
    KEYWORD = TOKENCOUNT
};

void initReserved() {
    table_init(&reservedWords, NAMESPACESIZE);
    for (int i = 0; i < sizeof generalKeywords / sizeof *generalKeywords; i++) {
        *table_insert(&reservedWords, generalKeywords[i], strlen(generalKeywords[i])) = KEYWORD;
    }
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
    /*
    int* tokEntry = table_lookup(&reservedWords, lexeme, lexemeLen);
    if (tokEntry != TABLE_NULL) type = KEYWORD;
    */
    t->lexeme = lexeme;
    t->lexemeLen = lexemeLen;
    t->type = type;
}
