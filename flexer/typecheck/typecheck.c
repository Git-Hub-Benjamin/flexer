#include "../parser/fparse.h"
#include "../hash/hash.h"
#include "../token.h"

typedef struct {
    ASTNode* program;
    int errors;
} TypeChecker;

TypeChecker checker;
ht* functionTable;

bool typeCheck(ASTNode* program) {
    checker.program = program;
    checker.errors = 0;

    functionTable = ht_create(-1);

    traverseProgram(program->children[0]);

    return checker.errors == 0;
}

bool matchType(TokenType type1, TokenType type2) {
    return type1 == type2;
}

void traverseReturnStatement(ASTNode* node) {

    TokenType returnType = ((DeclarationType*)currentFunction->value)->returnType;
    TokenType currentType = TOK_VOID; // default as void

    for(size_t i = 0; i < node->childCount; i++) {
        ASTNode* child = node->children[i];

        if (!matchType(currentType)) {
            currentType = child->token.type;
        } else if (child->type == NODE_LITERAL_INT) {
            currentType = TOK_INT;
        }
    }

    
}

static ASTNode* currentFunction;
static ht* functionVariables;

void traverseFunctionBody(ASTNode* node) {
    
    for (size_t i = 0; i < node->childCount; i++) {

        ASTNode* child = node->children[i];

        switch(child->type) {
            case NODE_RETURN:

        }

        if (child->type == NODE_RETURN) {
            // check return type
            ASTNode* function = ht_get(functionTable, (char*)node->parent->token.data);
            if (function->value != NULL) {
                DeclarationType* decl = (DeclarationType*)function->value;
                if (decl->returnType != ((ASTNode*)child->children[0])->token.type) {
                    printf("Return type does not match function return type\n");
                    checker.errors++;
                }
            }
        }
    }
}


void traverseFunction(ASTNode* node) {

    // register function in table
    ht_set(functionTable, (char*)node->token.data, node);


    // check if body
    if (node->childCount > 1 && node->children[node->childCount - 1]->type == NODE_BLOCK) {
        currentFunction = node;
        functionVariables = ht_create(-1);
        for(int params = 0; params < node->childCount - 1; params++) {
            ASTNode* param = node->children[params];
            ht_set(functionVariables, (char*)param->children[1]->token.data, param);
        }
        traverseFunctionBody(node->children[1]);
    }
    
}

//* Check if function return type matches the return statement
//* Check if function arguments match the function call
//* Check if variable is declared before use
//* Check if variable is assigned before use
//* Check if variable is assigned the correct type

void traverseProgram(ASTNode* node) {

    if (node->type == NODE_FUNCTION) {
        traverseFunction(node);
    } 
   
    // Then traverse next
    if (node->next) {
        traverseProgram(node->next);
    }
}

