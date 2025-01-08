#include "fparse.h"
#include "../terminal-colors.h"
#include <stdint.h>

typedef struct {
    Token* tokens;
    Token* current;
    int errors;
} Parser;

static Parser* parser;
static uint32_t tokenIndex = 0;
extern char* TokenStrings[];

// Forward declarations
static ASTNode* parseTranslationUnit(void);
static ASTNode* parseStatement(void);
static ASTNode* parseExpression(void);
static ASTNode* parseDeclaration(void);
static ASTNode* parseVariableDeclaration(Token, Token);
static ASTNode* parseFunctionDefinition(Token, Token);
static ASTNode* parseParameter(void);
static ASTNode* parseBlock(void);
static ASTNode* parseIfStatement(void);
static ASTNode* parseWhileStatement(void);
static ASTNode* parsePrimary(void);
static ASTNode* parseUnary(void);
static ASTNode* parseBinaryRHS(int, ASTNode*);

static Token* peek(void) {
    return &parser->tokens[tokenIndex];
}

static Token* advance(void) {
    Token* current = &parser->tokens[tokenIndex];
    tokenIndex++;
    parser->current = current;
    return current;
}

static bool match(TokenType type) {
    if (peek()->type == type) {
        advance();
        return true;
    }
    return false;
}

static ASTNode* createNode(NodeType type) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = type;
    node->children = NULL;
    node->childCount = 0;
    node->childCapacity = 0;
    node->value = NULL;
    node->next = NULL;
    node->token.data = NULL;  // Initialize token data
    node->token.type = 0;     // Initialize token type
    return node;
}

static void addChild(ASTNode* parent, ASTNode* child) {
    if (parent->childCount >= parent->childCapacity) {
        parent->childCapacity = parent->childCapacity == 0 ? 4 : parent->childCapacity * 2;
        parent->children = (ASTNode**)realloc(parent->children,  // Note double pointer
                                            parent->childCapacity * sizeof(ASTNode*));
    }
    parent->children[parent->childCount++] = child;  // Store pointer instead of copying
}

static int getOperatorPrecedence(TokenType type) {
    switch (type) {
        case TOK_EQUALS:    return 1;
        case TOK_OR:        return 2;
        case TOK_AND:       return 3;
        case TOK_EQ:        return 4;
        case TOK_NEQ:       return 4;
        case TOK_LT:        return 5;
        case TOK_GT:        return 5;
        case TOK_LTE:       return 5;
        case TOK_GTE:       return 5;
        case TOK_PLUS:      return 6;
        case TOK_MINUS:     return 6;
        case TOK_STAR:      return 7;
        case TOK_SLASH:     return 7;
        case TOK_PERCENT:   return 7;
        default:            return 0;
    }
}

ASTNode* parse(Token* tokens) {
    parser = (Parser*)malloc(sizeof(Parser));
    parser->tokens = tokens;
    parser->current = NULL;
    parser->errors = 0;
    tokenIndex = 0;
    
    ASTNode* ast = parseTranslationUnit();
    
    if (parser->errors > 0) {
        printf("Parsing failed with %d errors\n", parser->errors);
    }
    
    free(parser);
    return ast;
}

static ASTNode* parseTranslationUnit(void) {
    ASTNode* root = createNode(NODE_PROGRAM);
    
    while (peek()->type != TOK_EOF) {
        ASTNode* declaration = NULL;
        
        switch (peek()->type) {
            case TOK_INT:
            case TOK_CHAR:
            case TOK_VOID:
                declaration = parseDeclaration();
                if (declaration) {
                    addChild(root, declaration);
                }
                break;
            
            case TOK_SKIP:
            case TOK_SKIP_IMM:
            case TOK_SKIP_LIT:
                advance();
                break;
                
            default:
                parser->errors++;
                advance();
                break;
        }
    }
    
    return root;
}

static ASTNode* parseDeclaration(void) {
    Token typeToken = *advance();
    
    // Handle pointers
    int pointerDepth = 0;
    while (match(TOK_STAR)) {
        pointerDepth++;
    }
    
    Token* id = peek();
    if (id->type != TOK_IDENTIFIER) {
        parser->errors++;
        return NULL;
    }
    advance();
    
    ASTNode* decl;
    if (peek()->type == TOK_LPAREN) {
        decl = parseFunctionDefinition(typeToken, *id);
    } else {
        decl = parseVariableDeclaration(typeToken, *id);
    }
    
    // Store return type in value field

    if (decl->type == NODE_FUNCTION) {
        FunctionType* funcType = (FunctionType*)malloc(sizeof(FunctionType));
        funcType->pointerDepth = pointerDepth;
        funcType->returnType = typeToken.type;
        decl->value = funcType;
    }

    // Store pointer depth in value field

    if (pointerDepth > 0) {
        if (decl->type == NODE_FUNCTION) {
            ((FunctionType*)decl->value)->pointerDepth = pointerDepth;
        } else {
            char* depth = malloc(8);
            snprintf(depth, 8, "%d", pointerDepth);
            decl->value = depth;
        }
    }
    
    return decl;
}

static ASTNode* parseVariableDeclaration(Token typeToken, Token identifier) {
    ASTNode* varDecl = createNode(NODE_VARIABLE_DECL);
    varDecl->token = identifier;
    
    // Handle array declaration
    if (peek()->type == TOK_LBRACKET) {
        advance();
        ASTNode* size = parseExpression();
        if (size) {
            addChild(varDecl, size);
        }
        if (!match(TOK_RBRACKET)) {
            parser->errors++;
        }
    }
    
    // Handle initialization
    if (match(TOK_EQUALS)) {
        // Handle pointer initialization 
        if (match(TOK_AMPERSAND)) {
            ASTNode* addrOf = createNode(NODE_ADDR_OF);
            ASTNode* expr = parseExpression();
            if (expr) addChild(addrOf, expr);
            addChild(varDecl, addrOf);
        } else {
            ASTNode* initializer = parseExpression();
            if (initializer) addChild(varDecl, initializer);
        }
    }
    
    if (!match(TOK_SEMICOLON)) {
        parser->errors++;
    }
    
    return varDecl;
}

static ASTNode* parseFunctionDefinition(Token returnType, Token identifier) {
    ASTNode* function = createNode(NODE_FUNCTION);
    function->token = identifier;
    
    if (!match(TOK_LPAREN)) {
        parser->errors++;
        return function;
    }
    
    while (peek()->type != TOK_RPAREN) {
        ASTNode* param = parseParameter();
        if (param) {
            addChild(function, param);
        }
        
        if (peek()->type == TOK_COMMA) {
            advance();
        }
    }
    
    if (!match(TOK_RPAREN)) {
        parser->errors++;
        return function;
    }
    
    ASTNode* body = parseBlock();
    if (body) {
        addChild(function, body);
    }
    
    return function;
}

static ASTNode* parseParameter(void) {
    ASTNode* param = createNode(NODE_PARAMETER);
    
    Token* type = peek();
    if (type->type != TOK_INT && type->type != TOK_CHAR && type->type != TOK_VOID) {
        parser->errors++;
        return NULL;
    }
    advance();
    param->token = *type;
    
    Token* id = peek();
    if (id->type != TOK_IDENTIFIER) {
        parser->errors++;
        return param;
    }
    advance();
    
    if (match(TOK_LBRACKET)) {
        if (peek()->type != TOK_RBRACKET) {
            ASTNode* size = parseExpression();
            if (size) {
                addChild(param, size);
            }
        }
        
        if (!match(TOK_RBRACKET)) {
            parser->errors++;
        }
    }
    
    return param;
}

static ASTNode* parseBlock(void) {
    if (!match(TOK_LBRACE)) {
        parser->errors++;
        return NULL;
    }
    
    ASTNode* block = createNode(NODE_BLOCK);
    
    while (peek()->type != TOK_RBRACE && peek()->type != TOK_EOF) {
        ASTNode* statement = parseStatement();
        if (statement) {
            addChild(block, statement);
        }
    }
    
    if (!match(TOK_RBRACE)) {
        parser->errors++;
    }
    
    return block;
}

static ASTNode* parseStatement(void) {
    switch (peek()->type) {
        case TOK_LBRACE:
            return parseBlock();
            
        case TOK_IF:
            return parseIfStatement();
            
        case TOK_WHILE:
            return parseWhileStatement();
            
        case TOK_RETURN: {
            advance();
            ASTNode* returnNode = createNode(NODE_RETURN);
            
            if (peek()->type != TOK_SEMICOLON) {
                ASTNode* expr = parseExpression();
                if (expr) {
                    addChild(returnNode, expr);
                }
            }
            
            if (!match(TOK_SEMICOLON)) {
                parser->errors++;
            }
            
            return returnNode;
        }
            
        case TOK_INT:
        case TOK_CHAR:
        case TOK_VOID:
            return parseDeclaration();
            
        default: {
            ASTNode* expr = parseExpression();
            
            if (!match(TOK_SEMICOLON)) {
                parser->errors++;
            }
            
            return expr;
        }
    }
}

static ASTNode* parseIfStatement(void) {
    ASTNode* ifNode = createNode(NODE_IF);
    
    advance();
    
    if (!match(TOK_LPAREN)) {
        parser->errors++;
        return ifNode;
    }
    
    ASTNode* condition = parseExpression();
    if (condition) {
        addChild(ifNode, condition);
    }
    
    if (!match(TOK_RPAREN)) {
        parser->errors++;
    }
    
    ASTNode* thenBranch = parseStatement();
    if (thenBranch) {
        addChild(ifNode, thenBranch);
    }
    
    if (match(TOK_ELSE)) {
        ASTNode* elseBranch = parseStatement();
        if (elseBranch) {
            addChild(ifNode, elseBranch);
        }
    }
    
    return ifNode;
}

static ASTNode* parseWhileStatement(void) {
    ASTNode* whileNode = createNode(NODE_WHILE);
    
    advance();
    
    if (!match(TOK_LPAREN)) {
        parser->errors++;
        return whileNode;
    }
    
    ASTNode* condition = parseExpression();
    if (condition) {
        addChild(whileNode, condition);
    }
    
    if (!match(TOK_RPAREN)) {
        parser->errors++;
    }
    
    ASTNode* body = parseStatement();
    if (body) {
        addChild(whileNode, body);
    }
    
    return whileNode;
}

static ASTNode* parsePrimary(void) {
    Token* token = peek();
    
    switch (token->type) {
        case TOK_INTEGER_LITERAL: {
            advance();
            ASTNode* node = createNode(NODE_LITERAL);
            node->token = *token;
            return node;
        }
        
        case TOK_STRING_LITERAL: {
            advance();
            ASTNode* node = createNode(NODE_LITERAL);
            node->token = *token;
            return node;
        }
        
        case TOK_IDENTIFIER: {
            advance();
            if (peek()->type == TOK_LPAREN) {
                ASTNode* call = createNode(NODE_CALL);
                call->token = *token;
                
                advance();
                
                while (peek()->type != TOK_RPAREN) {
                    ASTNode* arg = parseExpression();
                    if (arg) {
                        addChild(call, arg);
                    }
                    
                    if (peek()->type == TOK_COMMA) {
                        advance();
                    } else {
                        break;
                    }
                }
                
                if (!match(TOK_RPAREN)) {
                    parser->errors++;
                }
                
                return call;
            } else {
                ASTNode* node = createNode(NODE_IDENTIFIER);
                node->token = *token;
                return node;
            }
        }
        
        case TOK_LPAREN: {
            advance();
            ASTNode* expr = parseExpression();
            if (!match(TOK_RPAREN)) {
                parser->errors++;
            }
            return expr;
        }
        
        default:
            parser->errors++;
            advance();
            return NULL;
    }
}

static ASTNode* parseUnary(void) {
    Token* token = peek();
    
    switch (token->type) {
        case TOK_MINUS:
        case TOK_NOT:
        case TOK_PLUS: {
            advance();
            ASTNode* operand = parseUnary();
            if (operand) {
                ASTNode* unary = createNode(NODE_UNARY_OP);
                unary->token = *token;
                addChild(unary, operand);
                return unary;
            }
            return NULL;
        }
        
        default:
            return parsePrimary();
    }
}

static ASTNode* parseBinaryRHS(int minPrecedence, ASTNode* left) {
    while (true) {
        Token* op = peek();
        int precedence = getOperatorPrecedence(op->type);
        
        if (precedence < minPrecedence) {
            break;
        }
        
        advance();
        
        ASTNode* right = parseUnary();
        if (!right) {
            return NULL;
        }
        
        Token* nextOp = peek();
        int nextPrecedence = getOperatorPrecedence(nextOp->type);
        
        if (precedence < nextPrecedence) {
            right = parseBinaryRHS(precedence + 1, right);
            if (!right) {
                return NULL;
            }
        }
        
        ASTNode* binary = createNode(NODE_BINARY_OP);
        binary->token = *op;
        addChild(binary, left);
        addChild(binary, right);
        left = binary;
    }
    
    return left;
}

static ASTNode* parseExpression(void) {
    ASTNode* left = parseUnary();
    if (!left) {
        return NULL;
    }
    
    return parseBinaryRHS(1, left);
}

void printASTNode(ASTNode* node, int depth) {
    for (int i = 0; i < depth; i++) {
        printf("│   ");
    }

    printf("├── " COLOR_YELLOW "Node: " COLOR_RESET);
    switch (node->type) {
        case NODE_PROGRAM:        printf(COLOR_BRIGHT_GREEN "【Program】" COLOR_RESET); break;
        case NODE_FUNCTION:       printf(COLOR_BRIGHT_BLUE "【Function】" COLOR_RESET); break;
        case NODE_VARIABLE_DECL:  printf(COLOR_BRIGHT_CYAN "【Variable】" COLOR_RESET); break;
        case NODE_PARAMETER:      printf(COLOR_BRIGHT_MAGENTA "【Parameter】" COLOR_RESET); break;
        case NODE_BLOCK:          printf(COLOR_BRIGHT_YELLOW "【Block】" COLOR_RESET); break;
        case NODE_RETURN:         printf(COLOR_BRIGHT_RED "【Return】" COLOR_RESET); break;
        case NODE_IF:             printf(COLOR_BRIGHT_BLUE "【If】" COLOR_RESET); break;
        case NODE_WHILE:          printf(COLOR_BRIGHT_MAGENTA "【While】" COLOR_RESET); break;
        case NODE_EXPRESSION:     printf(COLOR_BRIGHT_GREEN "【Expression】" COLOR_RESET); break;
        case NODE_BINARY_OP:      printf(COLOR_BRIGHT_YELLOW "【Binary Op】" COLOR_RESET); break;
        case NODE_UNARY_OP:       printf(COLOR_BRIGHT_CYAN "【Unary Op】" COLOR_RESET); break;
        case NODE_LITERAL:        printf(COLOR_BRIGHT_WHITE "【Literal】" COLOR_RESET); break;
        case NODE_IDENTIFIER:     printf(COLOR_BRIGHT_GREEN "【Identifier】" COLOR_RESET); break;
        case NODE_CALL:           printf(COLOR_BRIGHT_BLUE "【Call】" COLOR_RESET); break;
        default:                  printf(COLOR_BRIGHT_RED "【Unknown】" COLOR_RESET); break;
    }

    if (node->token.data)
        printf(COLOR_CYAN " ➜ %s" COLOR_RESET, node->token.data);

    if (node->value) {
        if (node->type == NODE_VARIABLE_DECL)
            printf(COLOR_MAGENTA " 📍 ptr[%s]" COLOR_RESET, (char*)node->value);
        else if (node->type == NODE_FUNCTION)
            printf(COLOR_MAGENTA " 📍 ptr[%d] ret[%s]" COLOR_RESET, 
                ((FunctionType*)node->value)->pointerDepth,
                TokenStrings[((FunctionType*)node->value)->returnType]);
        else
            printf(COLOR_MAGENTA " 📍 %s" COLOR_RESET, (char*)node->value);
    }

    printf("\n");

    for (size_t i = 0; i < node->childCount; i++) {
        printASTNode(node->children[i], depth + 1);
    }

    if (node->next) {
        printASTNode(node->next, depth);
    }
}


void printAST(ASTNode* root) {
    if (!root) {
        printf("Empty AST\n");
        return;
    }
   
    printf("Abstract Syntax Tree:\n");
    printASTNode(root, 0);
}