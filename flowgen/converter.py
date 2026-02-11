import csv
import sys

def getDict():
    columns = {}
    columnHeaders = []
    rowHeaders = []
    correspondingTokens = {}
    with open(sys.argv[1], "r") as csvfile:
        firstRow = True
        columnIndex = {}
        reader = csv.reader(csvfile)
        columns = dict.fromkeys(next(reader))
        for i, name in enumerate(columns):
            columnHeader = ""
            columnIndex[i] = name
            if i != 0:
                columnHeaders.append(name)
                columnHeader = name
            columns[name] = {}
        for i, row in enumerate(reader):
            rowHeader = row[0]
            if rowHeader == '':
                continue
            if rowHeader == "CORRESPONDINGTOKEN":
                correspondingTokens = dict(zip([None] + columnHeaders, row))
                continue
            rowHeaders.append(rowHeader)
            for j, cell in enumerate(row):
                if j == 0:
                    continue
                columns[columnIndex[j]][rowHeader] = cell
    return columns, rowHeaders, columnHeaders, correspondingTokens



dicti, rowHeaders, columnHeaders, correspondingTokens = getDict()
correspondingTokens.pop(None)

rowHeaders.append("LASTCHAR")
columnHeaders.append("LASTSTATE")

stateMachineLines = []

for row in dicti:
    for col in dicti[row]:
        if dicti[row][col] == '': 
            continue
        stateMachineLines.append(f"t->transitions[{row}][{col}] = {dicti[row][col]};")

correspondingTokenLines = []

for key in correspondingTokens:
    if (correspondingTokens[key] == ''):
        continue
    correspondingTokenLines.append(f"t->correspondingTokens[{key}] = {correspondingTokens[key]};")

hfile = """
#pragma once
#include <ctype.h>
enum state {
    %s
};

enum charType {
    %s
};

typedef struct {
    enum state state;
    enum state transitions[LASTSTATE][LASTCHAR];
    enum tokenType correspondingTokens[LASTSTATE];
} stateMachine_t;

static enum charType getCharType(int c) {
    if (isalpha(c) || c == '_') return LETTERCHAR;
    if (isdigit(c)) return DIGITCHAR;
    if (c == '\\n') return NEWLINECHAR;
    if (isspace(c)) return WHITESPACECHAR;
    if (c == '+') return PLUSCHAR;
    if (c == '-') return MINUSCHAR;
    if (c == '%%') return MODCHAR;
    if (c == '^') return CARETCHAR;
    if (c == '~') return TILDECHAR;
    if (c == '#') return HASHCHAR;
    if (c == '*') return STARCHAR;
    if (c == '&') return AMPCHAR;
    if (c == '/') return SLASHCHAR;
    if (c == '*') return STARCHAR;
    if (c == '!') return EXCCHAR;
    if (c == '?') return QUESTIONCHAR;
    if (c == '[') return LBRACKETCHAR;
    if (c == ']') return RBRACKETCHAR;
    if (c == '(') return LPARENCHAR;
    if (c == ')') return RPARENCHAR;
    if (c == '{') return LCURLYCHAR;
    if (c == '}') return RCURLYCHAR;
    if (c == '.') return DOTCHAR;
    if (c == ',') return COMMACHAR;
    if (c == '|') return PIPECHAR;
    if (c == ';') return SEMICOLONCHAR;
    if (c == ':') return COLONCHAR;
    if (c == '=') return EQCHAR;
    if (c == '"') return QUOTECHAR;
    if (c == '\\\\') return BSLASHCHAR;
    if (c == '<') return LANGLECHAR;
    if (c == '>') return RANGLECHAR;
    if (c == EOF) return EOFCHAR;
    return BADCHAR;
}

static int stateMachine_update(stateMachine_t* t, int c, enum tokenType* currentTok) {
    enum charType charType = getCharType(c);
    enum state newState = t->transitions[t->state][charType];
    *currentTok = t->correspondingTokens[t->state];
    t->state = newState;
    return t->state;
}

static void stateMachine_init(stateMachine_t* t) {
    t->state = STARTSTATE;
    for (int i = 0; i < LASTSTATE; i++) {
        for (int j = 0; j < LASTCHAR; j++) {
            t->transitions[i][j] = CANTMOVESTATE;
        }
    }
    %s
    for (int i = 0; i < LASTSTATE; i++) {
        t->correspondingTokens[i] = BAD;
    }
    %s
}
"""
hfile %= (",\n    ".join(columnHeaders), ",\n    ".join(rowHeaders), "\n    ".join(stateMachineLines), "\n    ".join(correspondingTokenLines))
print(hfile)
