#pragma once

#include <stdio.h>
#include "table/table.h"
#include <stdio.h>
#include <string.h>
#include "table/table.h"
#include "stateMachineDefs.h"
#include "symbols.h"
#include "keywords.h"
#define NAMESPACESIZE 4096

struct token {
    enum tokenType type;
    union {
        int i;
        float f;
        enum symbolType symbolType
    };
    char* lexeme;
    int lexemeLen;
};

extern const char* symbolStrings[] = {
    "variable",
    "keyword",
    "type"
};

extern const char* symbolColors[] = {
    "\033[38;2;156;220;254m", // Light Blue (#9CDCFE) - Variables
    "\033[38;2;197;134;192m", // Pink/Magenta (#C586C0) - Keywords (Control flow)
    "\033[38;2;78;201;176m",  // Teal (#4EC9B0) - Types/Classes
};

typedef struct token token_t;

static const char* getTokenString(token_t* t) {
    if (t->type == identifier_token) {
        return symbolStrings[t->symbolType];
    }
    return tokenTypeStrings[t->type];
}

static const char* getTokenColor(token_t* t) {
    if (t->type == identifier_token) {
        return symbolColors[t->symbolType];
    }
    return tokenTypeColors[t->type];
}

static int token_sdebug(token_t* t, char* output, int length) {
    return snprintf(output, length, "%s \"%.*s\"\n", getTokenString(t), t->lexemeLen, t->lexeme);
}

static int token_fdebug(token_t* t, FILE* out) {
    return fprintf(out, "%s \"%.*s\"\n", getTokenString(t), t->lexemeLen, t->lexeme);
}

static int token_debug(token_t* t) {
    return token_fdebug(t, stdout);
}

static int token_debugPrettyPrint(token_t* t) {
    printf("(%s)%s%.*s\033[0m", getTokenString(t), getTokenColor(t), t->lexemeLen, t->lexeme);
    fflush(stdout);
}

static int token_prettyPrint(token_t* t) {
    printf("%s%.*s\033[0m", getTokenColor(t), t->lexemeLen, t->lexeme);
    fflush(stdout);
}

static void token_init(token_t* t, char* lexeme, int lexemeLen, int type, symbols_t* symbols) {
    if (type == identifier_token) {
        symbol_t* symbol = symbols_lookup(symbols, lexeme, lexemeLen);
        if (symbol != NULL) {
            t->symbolType = symbol->type;
        }
        else t->symbolType = VARIABLE;
    }
    t->lexeme = lexeme;
    t->lexemeLen = lexemeLen;
    t->type = type;
}
