import os
import sys
import parseStatemachine
import color

arg = "./stateMachine"

if (len(sys.argv) >= 2):
    arg = sys.argv[1]

files = map(lambda x: os.path.join(arg, x), os.listdir(arg))
    
data = parseStatemachine.collectStateTree(files)

definitions, processedStates, tokens, alphabet = parseStatemachine.normalizeStateTree(data)

alphabet.add(parseStatemachine.NONASCII)

alphabetEnumeration = dict(zip(alphabet, range(1, len(alphabet) + 1)))

correspondingTokens = list(map(lambda x: f"[{x}_state] = {processedStates[x]['token']}_token", list(processedStates.keys())))

transitions = []

for statename in processedStates:
    state = processedStates[statename]
    remainingChars = alphabet.copy() # alphabet is shallow, this is ok
    for edgeName in state["edges"]:
        edgeChars = set(edgeName)
        if (edgeName == "eof"):
            edgeChars = set([parseStatemachine.EOF])
        if (len(edgeChars) > 1):
            edgeChars = set(definitions[edgeName])
        edgeDest = state["edges"][edgeName]
        for char in edgeChars:
            if edgeDest == 'cantmove':
                continue
            transitions.append(f"[{statename}_state][{alphabetEnumeration[char]}] = {edgeDest}_state")
        remainingChars -= edgeChars

    if state['defaultedge'] != 'cantmove':
        for char in remainingChars:
            transitions.append(f"[{statename}_state][{alphabetEnumeration[char]}] = {state['defaultedge']}_state")

file = """
#pragma once
enum tokenType {
    %s
};

extern const char* tokenTypeStrings[TOKENCOUNT] = {
    %s
};

extern const char* tokenTypeColors[TOKENCOUNT] = {
    %s
};

#define MAXCHAR %d

enum state {
    %s
};

extern enum state stateMachine_transitions[STATECOUNT][MAXCHAR + 1] = {
    %s
};

extern enum tokenType stateMachine_correspondingTokens[STATECOUNT] = {
    %s
};

extern unsigned char asciiEnumeration[] = {
    %s
};
"""

processedStates.pop("cantmove", None)

def salvageCharRepr(char):
    if char is parseStatemachine.EOF:
        return '255'
    if char is parseStatemachine.NONASCII:
        return '244'
    if char == "'":
        return "\'\\'\'"
    return repr(char)

def getColor(tokenType):
    return "\"\\033[38;2;%d;%d;%dm\"" % (color.to_rgb(color.colors.get(tokenType) or (0xD4 / 255, 0xD4 / 255, 0xD4 / 255)))

print(file % (
    ",\n    ".join(map(lambda x: x + "_token", tokens)) + ",\n    TOKENCOUNT",
    ",\n    ".join(map(lambda x: f"\"{x}\"", tokens)),
    ",\n    ".join(map(getColor, tokens)),
    len(alphabet),
    "cantmove_state,\n    " + ",\n    ".join(map(lambda x: x + "_state", processedStates.keys())) + ",\n    STATECOUNT",
    ",\n    ".join(transitions),
    ",\n    ".join(correspondingTokens),
    ",\n    ".join(map(lambda x: f"[{salvageCharRepr(x)}]={alphabetEnumeration[x]}", alphabetEnumeration))
    ))
