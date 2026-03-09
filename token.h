#pragma once

#include <stdio.h>
#include "table/table.h"
#include <stdio.h>
#include <string.h>
#include "table/table.h"
#include "stateMachineDefs.h"
#include "symbols.h"
#include "keywords.h"
#include "stringStreaming/stringstream.h"
#define NAMESPACESIZE 4096

struct token {
    enum tokenType type;
    union {
        int i;
        float f;
        enum symbolType symbolType;
    };
    char* lexeme;
    int lexemeLen;
};

static const char* symbolColors[] = {
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

static int token_adebug(token_t* t, void* od, aprintf of) {
    return of(od, "%s \"%.*s\"\n", getTokenString(t), t->lexemeLen, t->lexeme);
}

static int token_sdebug(token_t* t, char* output, int length) {
    char* s = output;
    staticstring_init(&s, length);
    return token_adebug(t, s, (void*) staticstring_stream);
}

static int token_fdebug(token_t* t, FILE* out) {
    return token_adebug(t, out, (void*) fprintf);
}

static int token_adebugPrettyPrint(token_t* t, void* od, aprintf of) {
    return of(od, "(%s)%s%.*s\033[0m", getTokenString(t), getTokenColor(t), t->lexemeLen, t->lexeme);
}

static int token_sdebugPrettyPrint(token_t* t, char* output, int length) {
    char* s = output;
    staticstring_init(&s, length);
    return token_adebugPrettyPrint(t, &s, (void*) staticstring_stream);
}

static int token_fdebugPrettyPrint(token_t* t, FILE* out) {
    return token_adebugPrettyPrint(t, out, (void*) fprintf);
}

static int token_aprettyPrint(token_t* t, void* od, aprintf of) {
    return of(od, "%s%.*s\033[0m", getTokenColor(t), t->lexemeLen, t->lexeme);
}

static int token_sprettyPrint(token_t* t, char* output, int length) {
    char* s = output;
    staticstring_init(&s, length);
    return token_aprettyPrint(t, &s, (void*) staticstring_stream);
}

static int token_fprettyPrint(token_t* t, FILE* out) {
    return token_aprettyPrint(t, out, (void*) fprintf);
}

static void token_init(token_t* t, char* lexeme, int lexemeLen, int type, symbols_t* symbols) {
    if (type == identifier_token) {
        symbol_t* symbol = symbols_lookup(symbols, lexeme, lexemeLen);
        if (symbol != NULL) {
            t->symbolType = symbol->type;
        }
        else t->symbolType = variable_symbol;
    }
    t->lexeme = lexeme;
    t->lexemeLen = lexemeLen;
    t->type = type;
}
