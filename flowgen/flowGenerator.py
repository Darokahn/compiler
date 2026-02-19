import os
import yaml
from graphviz import Digraph
import parseStatemachine

files = map(lambda x: "stateMachine/" + x, os.listdir("stateMachine"))
    
data = parseStatemachine.collectStateTree(files)

definitions, processedStates, tokens, alphabet = parseStatemachine.normalizeStateTree(data)

dot = Digraph(comment="State Machine")
dot.attr(rankdir="LR")

# Add nodes
for stateName, state in processedStates.items():
    if stateName == "cantmove":
        continue
    label = stateName
    if state["token"] != stateName:
        label += f"\n[{state['token']}]"
    shape = "circle"
    style = "filled,bold"
    fillcolor = "green"
    if state["token"] == "bad":
        fillcolor = "red"
    dot.node(stateName, label=label, shape=shape, style=style,fillcolor=fillcolor)

# Add edges
for stateName, state in processedStates.items():
    combinedEdges = {}
    for edgeName, dest in state["edges"].items():
        combinedEdges[dest] = combinedEdges.get(dest) or []
        combinedEdges[dest].append(edgeName)
    combinedEdges[state["defaultedge"]] = ["all others"]
    for dest, edgeNames in combinedEdges.items():
        label = ", ".join(edgeNames)
        if dest == "cantmove":
            if state["defaultedge"] == "cantmove":
                continue
            newNode = f"{stateName}.cantmove"
            dot.node(newNode, label="", shape="circle", style="filled", fillcolor="yellow", width="0.2", height="0.2")
            dest = newNode
        dot.edge(stateName, dest, label=label)

dot.render("graph", format="png", cleanup=True)
print("Graph saved to statemachine_graph.png")
