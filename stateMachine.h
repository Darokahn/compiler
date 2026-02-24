#pragma once
#include <ctype.h>

#include "stateMachineDefs.h"

typedef struct {
    enum state state;
} stateMachine_t;

static unsigned char stateMachine_reduceAscii(int c) {
    if (c == EOF) c = 255;
    else if (c > 127) c = 244;
    return asciiEnumeration[c];
}

static int stateMachine_update(stateMachine_t* t, int c, enum tokenType* currentTok) {
    unsigned char index = stateMachine_reduceAscii(c);
    enum state newState = stateMachine_transitions[t->state][index];
    *currentTok = stateMachine_correspondingTokens[t->state];
    t->state = newState;
    return t->state;
}

static void stateMachine_init(stateMachine_t* t) {
    t->state = start_state;
}

static int stateMachine_getToken(void* textCursor, void* f0, enum tokenType* lastTokenType) {
    int (*getChar)(void*) = f0;
    stateMachine_t stateMachine;
    stateMachine_init(&stateMachine);
    int lexemeLen = 0;
    enum state currentState;
    int c;
    do {
        c = getChar(textCursor);
        lexemeLen++;
        currentState = stateMachine_update(&stateMachine, c, lastTokenType);
    } while (currentState != cantmove_state);
    if (*lastTokenType == bad_token) {
        return 0;
    }
    lexemeLen--;
    return lexemeLen;
}
