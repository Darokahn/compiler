#pragma once

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "keywords.h"
#include "table/table.h"
#include "stringStreaming/stringStreaming.h"

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
    char* name;
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

static void symbols_add(symbols_t* t, char* name, int nameLen, symbol_t symbol) {
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

static symbol_t* symbols_lookup(symbols_t* t, char* name, int nameLen) {
    int* indexPtr = table_lookup(&t->table, name, nameLen);
    if (indexPtr == TABLE_NULL) {
        return NULL;
    }
    int index = *indexPtr;
    symbol_t sym = t->storage[index];
    return &(t->storage[index]);
}

static bool symbols_exists(symbols_t* t, char* name, int nameLen) {
    return symbols_lookup(t, name, nameLen) != NULL;
}

static void symbols_delete(symbols_t* t, char* name, int nameLen) {
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

static int symbols_adebug(symbols_t* t, void* od, int (*of)(void*, char*, ...), char* tabString, char* newlineString) {
    int charsPrinted = 0;
    char* fmt = "symbol table with length %d: {%s";
    charsPrinted += of(od, fmt, t->count, newlineString);
    int maxName = 0;
    int namePadding = 3;
    for (int i = 0; i < t->count; i++) {
        symbol_t sym = t->storage[i];
        if (sym.nameLen > maxName) {
            maxName = sym.nameLen;
        }
    }
    fmt = "%sname: \x1b[s%.*s,\x1b[u\x1b[%dCtype: %s%s";
    for (int i = 0; i < t->count; i++) {
        symbol_t sym = t->storage[i];
        charsPrinted += of(od, fmt, tabString, sym.nameLen, sym.name, maxName + namePadding, symbolStrings[sym.type], newlineString);
    }
    fmt = "}%s";
    charsPrinted += of(od, fmt, newlineString);
    return charsPrinted;
}

static int symbols_fdebug(symbols_t* t, FILE* out, char* tabString, char* newlineString) {
    return symbols_adebug(t, out, (void*) fprintf, tabString, newlineString);
}

static int symbols_sdebug(symbols_t* t, char* out, int len, char* tabString, char* newlineString) {
    return symbols_adebug(t, &out, (void*) staticstring_stream, tabString, newlineString);
}

static int symbols_debug(symbols_t* t) {
    return symbols_fdebug(t, stdout, "    ", "\n");
}
