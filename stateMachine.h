#pragma once
#include <ctype.h>

#include "stateMachineDefs.h"

typedef struct {
    enum state state;
} stateMachine_t;

static unsigned char stateMachine_reduceAscii(unsigned char c) {
    return asciiEnumeration[c];
}

static int stateMachine_update(stateMachine_t* t, int c, enum tokenType* currentTok) {
    if (c == EOF) c = 255;
    unsigned char index = stateMachine_reduceAscii(c);
    enum state newState = stateMachine_transitions[t->state][index];
    *currentTok = stateMachine_correspondingTokens[t->state];
    t->state = newState;
    return t->state;
}

static void stateMachine_init(stateMachine_t* t) {
    t->state = start_state;
}
