#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "patch.h"

typedef uint8_t byte;

static byte* findByte(byte* stream, byte b) {
    while (*stream != b) stream++;
    return stream;
}

typedef struct {
    byte poprax[1];
    byte zerordi[3];
    byte cmp[3];
    byte je[2];
    byte jeTarget[4];
} ifStruct;

typedef struct {
    byte poprax[1];
    byte zerordi[3];
    byte cmp[3];
    byte je[2];
    byte jeTarget[4];
} whileStruct;

typedef struct {
    byte jmp[1];
    byte jmpTarget[4];
} jmpStruct;

jmpStruct jmpBytes = {
    .jmp={0xe9},
    .jmpTarget={0},
};

enum pushPopVals {
    PUSHRAX = 0x50,
    PUSHRDI = 0x57,
    POPRAX = 0x58,
    POPRDI = 0x5f,
};

ifStruct ifBytes = {
    .poprax   = {POPRAX},
    .zerordi  = {0x48, 0x31, 0xff},
    .cmp      = {0x48, 0x39, 0xc7},
    .je       = {0x0f, 0x84},
    .jeTarget = {0x00, 0x00, 0x00, 0x00},
};

whileStruct whileBytes = {
    .poprax   = {POPRAX},
    .zerordi  = {0x48, 0x31, 0xff},
    .cmp      = {0x48, 0x39, 0xc7},
    .je       = {0x0f, 0x84},
    .jeTarget = {0x00, 0x00, 0x00, 0x00},
};

typedef struct {
    byte mov[3];
    byte offset[1];
} setFromMemStruct;

setFromMemStruct setRaxBytes = {
    {0x48, 0x8b, 0x45},
    {0}
};

setFromMemStruct setRdiBytes = {
    {0x48, 0x8b, 0x7d},
    {0}
};
typedef struct {
    byte mov[3];
    byte offset[1];
} storeToMemStruct;

storeToMemStruct storeRaxBytes = {
    {0x48, 0x89, 0x45},
    {0}
};

storeToMemStruct storeRdiBytes = {
    {0x48, 0x89, 0x7d},
    {0}
};

typedef struct {
    byte movabs[2];
    byte value[8];
} setConstStruct;

setConstStruct setConstRax = {
    {0x48, 0xb8},
    {0}
};
setConstStruct setConstRdi = {
    {0x48, 0xbf},
    {0}
};

typedef struct {
    byte call[2];
} callStruct;

callStruct callRax = {
    {0xff, 0xd0}
};

callStruct callRdi = {
    {0xff, 0xd7}
};

typedef enum {
    REGISTER,
    STACK,
    GLOBALMEM,
    ABI,
    NEXTSLOT_STORAGE,
    NEXTSLOT_ABI,
} storageType;

typedef enum {
    REGISTER_OP1,
    REGISTER_OP2,
    REGISTER_OPDEST,
    REGISTER_N,
} registerId;

typedef struct {
    storageType type;
    union {
        int registerId;
        int stackOffset;
        int globalOffset;
        int abiPosition;
        int constVal;
    };
} storageLocation_t;

int pushCount = 0;
int popCount = 0;
typedef struct funcGen_t {
    byte* code;
    byte* codePtr;
    int codeSize;
    void* objectPage;
} funcGen_t;

    static int funcGen_len(funcGen_t* g) {
        return g->codePtr - g->code;
    }
    static void funcGen_protect(funcGen_t* g) {
        byte* originalPosition = g->code;
        long pageSize = sysconf(_SC_PAGESIZE);
        g->code += (pageSize - 1);
        g->code = (byte*)((intptr_t)g->code & ~(pageSize-1));
        int result = mprotect(g->code, g->codeSize, PROT_READ | PROT_WRITE | PROT_EXEC);
        if (result != 0) {
            printf("error protecting memory %p: %s\n", g->code, strerror(errno));
        }
    }
    static void funcGen_release(funcGen_t* g) {
        mprotect(g->code, g->codeSize, PROT_READ | PROT_WRITE);
    }
    static void funcGen_loadInstruction(funcGen_t* g, char* instructionName) {
        char buffer[512];
        snprintf(buffer, sizeof buffer, "op_%s", instructionName);
        byte* funcBytes = dlsym(g->objectPage, buffer);
        byte* nopIndex = findByte(funcBytes, 0x90);
        int len = nopIndex - funcBytes;
        memcpy(g->codePtr, funcBytes, len);
        g->codePtr += len;
    }
    static void funcGen_prologue(funcGen_t* g) {
        funcGen_loadInstruction(g, "prologue");
    }
    static void funcGen_epilogue(funcGen_t* g) {
        funcGen_loadInstruction(g, "epilogue");
    }
    static funcGen_t* funcGen_init(funcGen_t* g, byte* code, int maxSize, bool protect) {
        g->code = code;
        if (protect) funcGen_protect(g);
        g->codePtr = g->code;
        g->codeSize = maxSize;
        g->objectPage = dlopen(NULL, RTLD_LAZY);
        funcGen_prologue(g);
        return g;
    }
    static int (*funcGen_finish(funcGen_t* g))() {
        funcGen_epilogue(g);
        dlclose(g->objectPage);
        g->objectPage = NULL;
        return (void*) g->code;
    }
    static void funcGen_call(funcGen_t* g, registerId reg) {
        callStruct* s = reg == REGISTER_OP1 ? &callRax : &callRdi;
        memmove(g->codePtr, s, sizeof *s);
        g->codePtr += sizeof *s;
    }
    static void funcGen_push(funcGen_t* g, registerId reg) {
        *g->codePtr = reg == REGISTER_OP1 ? PUSHRAX : PUSHRDI;
        g->codePtr++;
    }
    static void funcGen_pop(funcGen_t* g, registerId reg) {
        *g->codePtr = reg == REGISTER_OP1 ? POPRAX : POPRDI;
        g->codePtr++;
    }
    static void funcGen_setConst(funcGen_t* g, registerId reg, int64_t value) {
        setConstStruct* s = reg == REGISTER_OP1 ? &setConstRax : &setConstRdi;
        setConstStruct* newStruct = (void*) g->codePtr;
        memmove(newStruct, s, sizeof *s);
        memmove(newStruct->value, &value, sizeof newStruct->value);
        g->codePtr += sizeof *s;
    }
    static void funcGen_pushConst(funcGen_t* g, int64_t value) {
        funcGen_setConst(g, REGISTER_OP1, value);
        funcGen_push(g, REGISTER_OP1);
    }
    static void funcGen_setFromStack(funcGen_t* g, registerId reg, int8_t offset) {
        setFromMemStruct* template = reg == REGISTER_OP1 ? &setRaxBytes : &setRdiBytes;
        memcpy(g->codePtr, template, sizeof *template);
        setFromMemStruct* newStruct = (void*) g->codePtr;
        g->codePtr += sizeof *template;
        memcpy(newStruct->offset, &offset, sizeof newStruct->offset);
    }
    static void funcGen_storeInStack(funcGen_t* g, registerId reg, int8_t offset) {
        storeToMemStruct* template = reg == REGISTER_OP1 ? &storeRaxBytes : &storeRdiBytes;
        memcpy(g->codePtr, template, sizeof *template);
        storeToMemStruct* newStruct = (void*) g->codePtr;
        g->codePtr += sizeof *template;
        memcpy(newStruct->offset, &offset, sizeof newStruct->offset);
    }
    static void funcGen_memToStack(funcGen_t* g, int8_t offset) {
        funcGen_setFromStack(g, REGISTER_OP1, offset);
        funcGen_push(g, REGISTER_OP1);
    }
    static void funcGen_stackToMem(funcGen_t* g, int8_t offset) {
        funcGen_pop(g, REGISTER_OP1);
        funcGen_storeInStack(g, REGISTER_OP1, offset);
    }
    static void funcGen_mov(funcGen_t* g, storageLocation_t dest, storageLocation_t src) {
    }
    static void funcGen_fillDest(byte* jmp, int jmpInstrSize, int jmpDestSize, byte* dest) {
        byte* start = jmp + jmpInstrSize + jmpDestSize;
        int total = dest - start;
        memcpy(jmp + jmpInstrSize, &total, jmpDestSize);
    }
    static jmpStruct* funcGen_jmp(funcGen_t* g, byte* dest) {
        jmpStruct* jmp = (jmpStruct*) g->codePtr;
        memcpy(jmp, &jmpBytes, sizeof jmpBytes);
        g->codePtr += sizeof jmpBytes;
        if (dest) {
            funcGen_fillDest((byte*) jmp, sizeof jmp->jmp, sizeof jmp->jmpTarget, dest);
        }
        return jmp;
    }
    static void funcGen_if(funcGen_t* g, ifStatementNode* node) {
        ifStatementNode_expandCondition(node, g);
        ifStruct* newStruct = (void*) g->codePtr;
        memcpy(newStruct, &ifBytes, sizeof ifBytes);
        g->codePtr += sizeof *newStruct;
        ifStatementNode_expandThen(node, g);
        jmpStruct* jmp = funcGen_jmp(g, NULL);
        funcGen_fillDest(newStruct->je, sizeof newStruct->je, sizeof newStruct->jeTarget, g->codePtr);
        ifStatementNode_expandElse(node, g);
        funcGen_fillDest((byte*) jmp, sizeof jmp->jmp, sizeof jmp->jmpTarget, g->codePtr);
    }
    static void funcGen_while(funcGen_t* g, whileStatementNode* node) {
        byte* beginning = g->codePtr;
        whileStatementNode_expandCondition(node, g);
        whileStruct* newStruct = (void*) g->codePtr;
        memcpy(newStruct, &whileBytes, sizeof whileBytes);
        g->codePtr += sizeof *newStruct;
        whileStatementNode_expandBody(node, g);
        funcGen_jmp(g, beginning);
        funcGen_fillDest(newStruct->je, sizeof newStruct->je, sizeof newStruct->jeTarget, g->codePtr);
    }
    static void funcGen_for(funcGen_t* g, forStatementNode* node) {
        forStatementNode_expandInit(node, g);
        byte* beginning = g->codePtr;
        forStatementNode_expandCondition(node, g);
        whileStruct* newStruct = (void*) g->codePtr;
        memcpy(newStruct, &whileBytes, sizeof whileBytes);
        g->codePtr += sizeof *newStruct;
        forStatementNode_expandBody(node, g);
        forStatementNode_expandIter(node, g);
        funcGen_jmp(g, beginning);
        funcGen_fillDest(newStruct->je, sizeof newStruct->je, sizeof newStruct->jeTarget, g->codePtr);
    }
