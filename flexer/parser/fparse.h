#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include "../token.h" 

typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_VARIABLE_DECL,
    NODE_PARAMETER,
    NODE_BLOCK,
    NODE_RETURN,
    NODE_IF,
    NODE_WHILE,
    NODE_EXPRESSION,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_LITERAL,
    NODE_IDENTIFIER,
    NODE_CALL,
    NODE_ADDR_OF
} NodeType;

typedef struct ASTNode {
    NodeType type;
    Token token;
    struct ASTNode** children;  // Double pointer
    size_t childCount;
    size_t childCapacity;
    void* value;
    struct ASTNode* next;
} ASTNode;

typedef struct FunctionType {
    int pointerDepth;
    TokenType returnType;
} FunctionType;

// Public interface
ASTNode* parse(Token* tokens);
void printAST(ASTNode* root);

#endif // PARSER_H