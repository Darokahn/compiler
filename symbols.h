#pragma once

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "keywords.h"
#include "table/table.h"

enum symbolType {
    variable_symbol,
    keyword_symbol,
    type_symbol
};

static const char* symbolStrings[] = {
    "variable",
    "keyword",
    "type"
};

// As I build on the compiler, these three items will gain the information necessary
typedef struct {
    int value;
} variable_t;

typedef struct {
} keyword_t;

typedef struct {
} type_t;

typedef struct {
    const char* name;
    int nameLen;
    int type;
    union {
        variable_t v;
        keyword_t k;
        type_t t;
    };
} symbol_t;

typedef struct {
    table_t table;
    symbol_t* storage;
    int capacity;
    int count;
} symbols_t;

static void symbols_init(symbols_t* t, int initialSize) {
    t->count = 0;
    t->capacity = initialSize;
    table_init(&t->table, initialSize);
    t->storage = malloc(sizeof *t->storage * initialSize);
}

static void symbols_add(symbols_t* t, const char* name, int nameLen, symbol_t symbol) {
    if (t->capacity == t->count) {
        t->capacity *= 2;
        t->storage = realloc(t->storage, sizeof *t->storage * t->capacity);
        table_resize(&t->table, t->capacity);
    }
    symbol.name = name;
    symbol.nameLen = nameLen;
    t->storage[t->count] = symbol;
    *table_insert(&t->table, name, nameLen) = t->count;
    t->count++;
}

static symbol_t* symbols_lookup(symbols_t* t, const char* name, int nameLen) {
    int* indexPtr = table_lookup(&t->table, name, nameLen);
    if (indexPtr == TABLE_NULL) {
        return NULL;
    }
    int index = *indexPtr;
    symbol_t sym = t->storage[index];
    return &(t->storage[index]);
}

bool symbols_exists(symbols_t* t, const char* name, int nameLen) {
    return symbols_lookup(t, name, nameLen) != NULL;
}

static void symbols_delete(symbols_t* t, const char* name, int nameLen) {
    int deletedIndex = *table_lookup(&t->table, name, nameLen);
    t->storage[deletedIndex] = t->storage[t->count - 1];
    t->count--;
    symbol_t reIndex = t->storage[deletedIndex];
    *table_lookup(&t->table, reIndex.name, reIndex.nameLen) = deletedIndex;
    table_delete(&t->table, name, nameLen);
}

static void symbols_destroy(symbols_t* t) {
    if (t->storage != NULL) {
        free(t->storage);
    }
    table_destroy(&t->table);
}

int symbols_adebug(symbols_t* t, void* out, int outType, const char* tabString, char* newlineString) {
    int charsPrinted = 0;
    FILE* f = (FILE*) out;
    char* s = (char*) out;
    char* fmt = "symbol table with length %d: {%s";
    if (outType == 0) {
        charsPrinted += sprintf(s, fmt, t->count, newlineString);
        s = (char*) out + charsPrinted;
    }
    else charsPrinted += fprintf(f, fmt, t->count, newlineString);
    int maxName = 0;
    for (int i = 0; i < t->count; i++) {
        symbol_t sym = t->storage[i];
        if (sym.nameLen > maxName) {
            maxName = sym.nameLen;
        }
    }
    fmt = "%sname: %.*s,\x1b[%dGtype: %s%s";
    int initialLen = strlen(tabString) + sizeof ("name: ") + 3;
    for (int i = 0; i < t->count; i++) {
        symbol_t sym = t->storage[i];
        if (outType == 0) {
            charsPrinted += sprintf(s, fmt, tabString, sym.nameLen, sym.name, initialLen + maxName, symbolStrings[sym.type], newlineString);
            s = (char*) out + charsPrinted;
        }
        else charsPrinted += fprintf(f, fmt, tabString, sym.nameLen, sym.name, initialLen + maxName, symbolStrings[sym.type], newlineString);
    }
    fmt = "}%s";
    if (outType == 0) {
        charsPrinted += sprintf(s, fmt, newlineString);
        s = (char*) out + charsPrinted;
    }
    else charsPrinted += fprintf(f, fmt, newlineString);
    return charsPrinted;
}

int symbols_fdebug(symbols_t* t, FILE* out, char* tabString, char* newlineString) {
    return symbols_adebug(t, out, 1, tabString, newlineString);
}

int symbols_sdebug(symbols_t* t, char* out, int len, char* tabString, char* newlineString) {
    return symbols_adebug(t, out, 0, tabString, newlineString);
}

int symbols_debug(symbols_t* t) {
    return symbols_fdebug(t, stdout, "    ", "\n");
}
