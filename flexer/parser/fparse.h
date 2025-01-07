#ifndef FPARSE_H
#define FPARSE_H

#include <stdint.h>
#include "../token.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

// Parser context
typedef struct {
    Token* tokens;
    Token* current;
    int errors;
} Parser;

// AST Node Types
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
    NODE_CALL
} NodeType;

// Generic AST Node structure
typedef struct ASTNode {
    NodeType type;
    struct ASTNode* children;
    size_t childCount;
    size_t childCapacity;
    Token token;          // Original token that created this node
    char* value;         // For literals and identifiers
    struct ASTNode* next; // For parameter lists, statement lists, etc.
} ASTNode;

ASTNode* parse(Token*);

#endif