#include "nodes.h"

int main() {
    nodeBase b;
    nodeBase_init(&b, 128);
    {
        plusOperatorNode n;
        plusOperatorNode_init(&n);
        nodeBase_add(&b, &n);
    }
    {
        timesOperatorNode n;
        timesOperatorNode_init(&n);
        nodeBase_add(&b, &n);
    }
    {
        integerNode n;
        integerNode_init(&n, 10);
        nodeBase_add(&b, &n);
    }
    {
        integerNode n;
        integerNode_init(&n, 10);
        nodeBase_add(&b, &n);
    }
    {
        integerNode n;
        integerNode_init(&n, 10);
        nodeBase_add(&b, &n);
    }
    int result = ((expressionNode*)(b.base))->eval(b.base);
    printf("%d\n", result);
}
