#pragma once

#include <stdint.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "token.h"
#include "stateMachineDefs.h"
#include "stateMachine.h"

static char* emptyString = "";

typedef struct {
    unsigned char* fileBase;
    unsigned char* fileReader;
    int lineCount;
} scannerState_t;

typedef struct {
    scannerState_t state;
    symbols_t* symbols;
    int length;
    bool stripWhitespace;
} scanner_t;

static int scanner_init(scanner_t* s, char* filename, symbols_t* symbols) {
    errno = 0;
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return errno;
    }
    struct stat stat;
    fstat(fd, &stat);
    if (stat.st_size == 0) {
        s->state.fileBase = (unsigned char*) emptyString;
    }
    else {
        s->state.fileBase = mmap(NULL, stat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    }
    if ((intptr_t) s->state.fileBase == -1) {
        perror("mmap failed");
        exit(errno);
    }
    s->state.fileReader = s->state.fileBase;
    s->length = stat.st_size;
    s->state.lineCount = 1;
    s->symbols = symbols;
    return 0;
}

// interface for reverting a scanner to a prior state, which can fail for a
// different implementation of a scanner where its underlying source is
// neither rewindable nor recorded for rewinding.
void scanner_revert(scanner_t* scanner, scannerState_t state) {
    scanner->state = state;
}

static int scanner_getc(scanner_t* s) {
    if (s->state.fileReader - s->state.fileBase >= s->length) {
        s->state.fileReader++;
        return EOF;
    }
    int c = *s->state.fileReader;
    s->state.fileReader++;
    return c;
}

static void scanner_ungetc(scanner_t* s) {
    s->state.fileReader--;
}

static token_t scanner_getNextToken(scanner_t* s) {
    stateMachine_t stateMachine;
    stateMachine_init(&stateMachine);
    unsigned char* lexemeBase = s->state.fileReader;
    int lexemeLen = 0;
    enum state currentState;
    enum tokenType lastTokenType;
    int c;
    do {
        c = scanner_getc(s);
        lexemeLen++;
        currentState = stateMachine_update(&stateMachine, c, &lastTokenType);
        if (currentState == start_state || currentState == eof_state) {
            lexemeBase = s->state.fileReader;
            lexemeLen = 0;
        }
        if (c == '\n' && currentState != cantmove_state) s->state.lineCount++;
    } while (currentState != cantmove_state);
    if (lastTokenType == bad_token) {
        fprintf(stderr, "bad token from lexeme \"%.*s\"\n", lexemeLen, lexemeBase);
        exit(1);
    }
    scanner_ungetc(s);
    lexemeLen--;
    token_t tok;
    token_init(&tok, (char*) lexemeBase, lexemeLen, lastTokenType, s->symbols);
    return tok;
}

static token_t scanner_peekNextToken(scanner_t* s) {
    scanner_t snapshot = *s;
    token_t tok = scanner_getNextToken(s);
    *s = snapshot;
    return tok;
}

static void scanner_destroy(scanner_t* s) {
    if (s->state.fileBase != NULL && s->state.fileBase != (void*)emptyString) {
        munmap(s->state.fileBase, s->length);
    }
    *s = (scanner_t){0};
}
