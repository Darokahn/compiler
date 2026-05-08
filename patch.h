#pragma once

// this is an ugly solution to an annoying include problem

typedef struct node node;
typedef struct funcGen_t funcGen_t;

typedef node ifStatementNode;
void ifStatementNode_expandCondition(node* in, struct funcGen_t* out);
void ifStatementNode_expandThen(node* in, struct funcGen_t* out);
void ifStatementNode_expandElse(node* in, struct funcGen_t* out);
typedef node whileStatementNode;
int whileStatementNode_eval(void* in);
void whileStatementNode_expand(node* in, funcGen_t* out);
void whileStatementNode_expandCondition(node* node, funcGen_t* g);
void whileStatementNode_expandBody(node* node, funcGen_t* g);
whileStatementNode* whileStatementNode_init(whileStatementNode* n);
typedef node forStatementNode;
int forStatementNode_eval(void* in);
void forStatementNode_expandInit(node* in, funcGen_t* out);
void forStatementNode_expandCondition(node* in, funcGen_t* out);
void forStatementNode_expandBody(node* in, funcGen_t* out);
void forStatementNode_expandIter(node* in, funcGen_t* out);
forStatementNode* forStatementNode_init(forStatementNode* n);
