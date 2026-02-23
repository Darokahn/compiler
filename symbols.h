#pragma once

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "keywords.h"
#include "table/table.h"

enum symbolType {
    VARIABLE,
    KEYWORD,
    TYPE
};

typedef struct {} variable_t;

typedef struct {} keyword_t;

typedef struct {} type_t;

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

static void symbols_delete(symbols_t* t, const char* name, int nameLen) {
    int deletedIndex = *table_lookup(&t->table, name, nameLen);
    t->storage[deletedIndex] = t->storage[t->count - 1];
    t->count--;
    symbol_t reIndex = t->storage[deletedIndex];
    *table_lookup(&t->table, reIndex.name, reIndex.nameLen) = deletedIndex;
}

static void symbols_destroy(symbols_t* t) {
    if (t->storage != NULL) {
        free(t->storage);
    }
    table_destroy(&t->table);
}
