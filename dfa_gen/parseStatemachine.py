import yaml
import os

reservedEdgenames = {"token", "defaultedge"}

def normalizeStateTree(data):
    definitions = data.pop("definitions", dict())
    processedData = {}
    tokens = set()
    alphabet = set()
    for stateName in data:
        state = data[stateName]
        token = state.pop("token", stateName)
        defaultEdge = state.pop("defaultedge", "cantmove")
        for edgeName in state:
            edgeDest = state[edgeName]
            maybeNewState = data.get(edgeDest) or processedData.get(edgeDest)
            if maybeNewState is None:
                # If a state is referred to but never defined, assume it exists with all of the defaults.
                # This may be overwritten later, which is what we want to happen if the destination state simply hasn't been parsed yet.
                processedData[edgeDest] = {"token": edgeDest, "defaultedge": "cantmove", "edges": dict()}
                tokens.add(edgeDest)
        # `token` and `defaultedge` are reserved for typing convenience, but for serious processing, edge characters should be separated from metadata.
        state = {
                "edges": state,
                "token": token,
                "defaultedge": defaultEdge
                }
        valid, message, charset = validateState(state, definitions)
        if (not valid):
            raise Exception(message)
        alphabet = alphabet | charset
        processedData[stateName] = state
        tokens.add(token)
    return definitions, processedData, tokens, alphabet

NONASCII = object()
EOF = object()
# a state may only have one edge for each type of character, including edges derived from charset definitions.
def validateState(state, definitions):
    usedCharacters = set()
    for edgeName in state["edges"]:
        charset = ""
        if (len(edgeName) == 1):
            charset = edgeName
        elif edgeName in definitions:
            charset = definitions[edgeName]
        elif edgeName == "eof":
            charset = [EOF]
        else:
            return False, f"labeled edge \"{edgeName}\" is neither single ascii char nor the name of a defined charset", set()
        if (usedCharacters.intersection(charset)):
            return False, f"rejected charset {charset}; used characters was already {usedCharacters}", set()
        usedCharacters.update(set(charset))
    state["alphabet"] = usedCharacters
    return True, "ok", usedCharacters

# shallow merge; rejects aliases.
def collectStateTree(filenames):
    collected = dict()
    for filename in filenames:
        data = None
        with open(filename, "r") as file:
            data = yaml.safe_load(file.read()) or {}
        collected = collected | data
    return collected


if __name__ == "__main__":
    files = map(lambda x: "stateMachine/" + x, os.listdir("stateMachine"))
    
    data = collectStateTree(files)

    definitions, processedStates, tokens, alphabet = normalizeStateTree(data)

    for statename in processedStates:
        state = processedStates[statename]
        status, message, charset = validateState(state, definitions)
        if (status is False):
            print(message)
