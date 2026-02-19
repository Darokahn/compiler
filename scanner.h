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

extern char* emptyString = "";

typedef struct {
    char* fileBase;
    char* fileReader;
    int length;
    int lineCount;
} scanner_t;

static void scanner_init(scanner_t* s, char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open %s failed: ", filename);
        perror("");
        exit(errno);
    }
    struct stat stat;
    fstat(fd, &stat);
    if (stat.st_size == 0) {
        s->fileBase = emptyString;
    }
    else {
        s->fileBase = mmap(NULL, stat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    }
    if ((intptr_t) s->fileBase == -1) {
        perror("mmap failed");
        exit(errno);
    }
    s->fileReader = s->fileBase;
    s->length = stat.st_size;
    s->lineCount = 1;
}

static int scanner_getc(scanner_t* s) {
    if (s->fileReader - s->fileBase >= s->length) {
        s->fileReader++;
        return EOF;
    }
    char c = *s->fileReader;
    s->fileReader++;
    return c;
}

static void scanner_ungetc(scanner_t* s) {
    s->fileReader--;
}

static token_t scanner_getNextToken(scanner_t* s) {
    stateMachine_t stateMachine;
    stateMachine_init(&stateMachine);
    char* lexemeBase = s->fileReader;
    int lexemeLen = 0;
    enum state currentState;
    enum tokenType lastTokenType;
    int c;
    do {
        c = scanner_getc(s);
        lexemeLen++;
        currentState = stateMachine_update(&stateMachine, c, &lastTokenType);
        if (currentState == start_state || currentState == eof_state) {
            lexemeBase = s->fileReader;
            lexemeLen = 0;
        }
        if (c == '\n' && currentState != cantmove_state) s->lineCount++;
    } while (currentState != cantmove_state);
    if (lastTokenType == bad_token) {
        fprintf(stderr, "bad token from lexeme \"%.*s\"\n", lexemeLen, lexemeBase);
        exit(1);
    }
    scanner_ungetc(s);
    lexemeLen--;
    token_t tok;
    token_init(&tok, lexemeBase, lexemeLen, lastTokenType);
    return tok;
}

static void scanner_destroy(scanner_t* s) {
    if (s->fileBase != NULL && s->fileBase != emptyString) {
        munmap(s->fileBase, s->length);
    }
    *s = (scanner_t){0};
}
