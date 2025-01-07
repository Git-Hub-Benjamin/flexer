#include "fparse.h"

static ASTNode* parseTranslationUnit(Parser*);
static ASTNode* parseFunction(Parser*);
static ASTNode* parseStatement(Parser*);
static ASTNode* parseExpression(Parser*);
static ASTNode* parseDeclaration(Parser*);
static ASTNode* createNode(NodeType);
static void addChild(ASTNode*, ASTNode*);
static Token* peek(Parser*);
static Token* advance(Parser*);
static bool match(Parser*, TokenType);
static ASTNode* parseDeclaration(Parser*);
static ASTNode* parseVariableDeclaration(Parser*, Token, Token);
static ASTNode* parseFunctionDefinition(Parser*, Token, Token);
static ASTNode* parseParameter(Parser*);
static ASTNode* parseBlock(Parser*);
static int getOperatorPrecedence(TokenType);
static ASTNode* parsePrimary(Parser*);
static ASTNode* parseUnary(Parser*);
static ASTNode* parseBinaryRHS(Parser*, int, ASTNode*);
static ASTNode* parseExpression(Parser*);
static ASTNode* parseStatement(Parser*);
static ASTNode* parseIfStatement(Parser*);
static ASTNode* parseWhileStatement(Parser*);

static uint32_t tokenIndex = 0;

// Helper functions
static ASTNode* createNode(NodeType type) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = type;
    node->children = NULL;
    node->childCount = 0;
    node->childCapacity = 0;
    node->value = NULL;
    node->next = NULL;
    return node;
}

static void addChild(ASTNode* parent, ASTNode* child) {
    if (parent->childCount >= parent->childCapacity) {
        parent->childCapacity = parent->childCapacity == 0 ? 4 : parent->childCapacity * 2;
        parent->children = (ASTNode*)realloc(parent->children, 
                                           parent->childCapacity * sizeof(ASTNode));
    }
    parent->children[parent->childCount++] = *child;
    free(child);  // Since we copied it into the array
}

// Token management functions

// Return the current token
static Token* peek(Parser* parser) {
    return parser->current;
}

// Return the current token AND set current to next token
static Token* advance(Parser* parser) {
    parser->current = &parser->tokens[tokenIndex++];
    return &parser->tokens[tokenIndex - 1];
}

static bool match(Parser* parser, TokenType type) {
    if (peek(parser)->type == type) {
        advance(parser);
        return true;
    }
    return false;
}

// Main parsing functions
static ASTNode* parseTranslationUnit(Parser* parser) {
    ASTNode* root = createNode(NODE_PROGRAM);
    
    while (peek(parser)->type != TOK_EOF) {
        // Handle global declarations and function definitions
        ASTNode* declaration = NULL;
        
        switch (peek(parser)->type) {
            case TOK_SKIP: // Maybe just remove these instead, hard though becuase this is not a linkedList
            case TOK_SKIP_IMM:
            case TOK_SKIP_LIT:
                advance(parser);
                break;
            case TOK_INT:
            case TOK_CHAR:
            case TOK_VOID:
                declaration = parseDeclaration(parser);
                if (declaration) {
                    addChild(root, declaration);
                }
                break;
                
            default:
                // Error handling
                parser->errors++;
                advance(parser);
                break;
        }
    }
    
    return root;
}

ASTNode* parseDeclaration(Parser* parser) {
    // Save the type token
    Token typeToken = *advance(parser);
    
    // Look ahead for identifier
    Token* id = peek(parser);
    if (id->type != TOK_IDENTIFIER) {
        parser->errors++;
        return NULL;
    }
    advance(parser);
    
    // Function declaration

    if (peek(parser)->type == TOK_LPAREN) {
        return parseFunctionDefinition(parser, typeToken, *id);
    }
    
    // Variable declaration
    return parseVariableDeclaration(parser, typeToken, *id);
}

ASTNode* parseVariableDeclaration(Parser* parser, Token typeToken, Token identifier) {
    ASTNode* varDecl = createNode(NODE_VARIABLE_DECL);
    varDecl->token = identifier;  // Save the variable name
    
    // Handle array declarations
    if (peek(parser)->type == TOK_LBRACKET) {
        advance(parser);  // Consume '['
        
        // Parse array size expression
        ASTNode* size = parseExpression(parser);
        if (size) {
            addChild(varDecl, size);
        }
        
        if (!match(parser, TOK_RBRACKET)) {
            parser->errors++;
        }
    }
    
    // Handle initialization
    if (match(parser, TOK_EQUALS)) {
        ASTNode* initializer = parseExpression(parser);
        if (initializer) {
            addChild(varDecl, initializer);
        }
    }
    
    // Expect semicolon
    if (!match(parser, TOK_SEMICOLON)) {
        parser->errors++;
    }
    
    return varDecl;
}

ASTNode* parseFunctionDefinition(Parser* parser, Token returnType, Token identifier) {
    ASTNode* function = createNode(NODE_FUNCTION);
    function->token = identifier;
    
    // Parse parameters
    if (!match(parser, TOK_LPAREN)) {
        parser->errors++;
        return function;
    }
    
    while (peek(parser)->type != TOK_RPAREN) {
        ASTNode* param = parseParameter(parser);
        if (param) {
            addChild(function, param);
        }
        
        if (peek(parser)->type == TOK_COMMA) {
            advance(parser);
        }
    }
    
    if (!match(parser, TOK_RPAREN)) {
        parser->errors++;
        return function;
    }
    
    // Parse function body
    ASTNode* body = parseBlock(parser);
    if (body) {
        addChild(function, body);
    }
    
    return function;
}

ASTNode* parseParameter(Parser* parser) {
    ASTNode* param = createNode(NODE_PARAMETER);
    
    // Parse parameter type
    Token* type = peek(parser);
    if (type->type != TOK_INT && type->type != TOK_CHAR && type->type != TOK_VOID) {
        parser->errors++;
        return NULL;
    }
    advance(parser);  // Consume type token
    param->token = *type;  // Save type information
    
    // Parse parameter name (identifier)
    Token* id = peek(parser);
    if (id->type != TOK_IDENTIFIER) {
        parser->errors++;
        return param;
    }
    advance(parser);  // Consume identifier
    
    // Handle array parameter
    if (match(parser, TOK_LBRACKET)) {
        // Array parameters like int arr[] or int arr[10]
        if (peek(parser)->type != TOK_RBRACKET) {
            // If there's a size specified, parse it
            ASTNode* size = parseExpression(parser);
            if (size) {
                addChild(param, size);
            }
        }
        
        if (!match(parser, TOK_RBRACKET)) {
            parser->errors++;
        }
    }
    
    return param;
}

// Define operator precedence
static int getOperatorPrecedence(TokenType type) {
    switch (type) {
        case TOK_EQUALS:            return 1;  // Assignment
        case TOK_OR:                return 2;  // Logical OR
        case TOK_AND:               return 3;  // Logical AND
        case TOK_EQ:                return 4;  // Equality
        case TOK_NEQ:               return 4;
        case TOK_LT:                return 5;  // Comparison
        case TOK_GT:                return 5;
        case TOK_LTE:               return 5;
        case TOK_GTE:               return 5;
        case TOK_PLUS:              return 6;  // Addition
        case TOK_MINUS:             return 6;
        case TOK_STAR:              return 7;  // Multiplication
        case TOK_SLASH:             return 7;
        case TOK_PERCENT:           return 7;
        default:                    return 0;
    }
}

// Parse primary expressions (literals, identifiers, parenthesized expressions)
static ASTNode* parsePrimary(Parser* parser) {
    Token* token = peek(parser);
    
    switch (token->type) {
        case TOK_INTEGER_LITERAL: {
            advance(parser);
            ASTNode* node = createNode(NODE_LITERAL);
            node->token = *token;
            return node;
        }
        
        case TOK_STRING_LITERAL: {
            advance(parser);
            ASTNode* node = createNode(NODE_LITERAL);
            node->token = *token;
            return node;
        }
        
        case TOK_IDENTIFIER: {
            advance(parser);
            // Check for function call
            if (peek(parser)->type == TOK_LPAREN) {
                ASTNode* call = createNode(NODE_CALL);
                call->token = *token;
                
                advance(parser);  // Consume '('
                
                // Parse arguments
                while (peek(parser)->type != TOK_RPAREN) {
                    ASTNode* arg = parseExpression(parser);
                    if (arg) {
                        addChild(call, arg);
                    }
                    
                    if (peek(parser)->type == TOK_COMMA) {
                        advance(parser);
                    } else {
                        break;
                    }
                }
                
                if (!match(parser, TOK_RPAREN)) {
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
            advance(parser);
            ASTNode* expr = parseExpression(parser);
            if (!match(parser, TOK_RPAREN)) {
                parser->errors++;
            }
            return expr;
        }
        
        default:
            parser->errors++;
            advance(parser);
            return NULL;
    }
}

// Parse unary expressions
static ASTNode* parseUnary(Parser* parser) {
    Token* token = peek(parser);
    
    switch (token->type) {
        case TOK_MINUS:
        case TOK_NOT:
        case TOK_PLUS: {
            advance(parser);
            ASTNode* operand = parseUnary(parser);
            if (operand) {
                ASTNode* unary = createNode(NODE_UNARY_OP);
                unary->token = *token;
                addChild(unary, operand);
                return unary;
            }
            return NULL;
        }
        
        default:
            return parsePrimary(parser);
    }
}

// Parse binary expressions with precedence climbing
static ASTNode* parseBinaryRHS(Parser* parser, int minPrecedence, ASTNode* left) {
    while (true) {
        Token* op = peek(parser);
        int precedence = getOperatorPrecedence(op->type);
        
        if (precedence < minPrecedence) {
            break;
        }
        
        advance(parser);  // Consume operator
        
        ASTNode* right = parseUnary(parser);
        if (!right) {
            return NULL;
        }
        
        Token* nextOp = peek(parser);
        int nextPrecedence = getOperatorPrecedence(nextOp->type);
        
        if (precedence < nextPrecedence) {
            right = parseBinaryRHS(parser, precedence + 1, right);
            if (!right) {
                return NULL;
            }
        }
        
        // Create binary operator node
        ASTNode* binary = createNode(NODE_BINARY_OP);
        binary->token = *op;
        addChild(binary, left);
        addChild(binary, right);
        left = binary;
    }
    
    return left;
}

static ASTNode* parseStatement(Parser* parser) {
    switch (peek(parser)->type) {
        case TOK_LBRACE:
            return parseBlock(parser);
            
        case TOK_IF:
            return parseIfStatement(parser);
            
        case TOK_WHILE:
            return parseWhileStatement(parser);
            
        case TOK_RETURN: {
            advance(parser);  // Consume 'return'
            ASTNode* returnNode = createNode(NODE_RETURN);
            
            // Handle return with expression
            if (peek(parser)->type != TOK_SEMICOLON) {
                ASTNode* expr = parseExpression(parser);
                if (expr) {
                    addChild(returnNode, expr);
                }
            }
            
            if (!match(parser, TOK_SEMICOLON)) {
                parser->errors++;
            }
            
            return returnNode;
        }
            
        case TOK_INT:
        case TOK_CHAR:
        case TOK_VOID:
            return parseDeclaration(parser);
            
        default: {
            // Expression statement or assignment
            ASTNode* expr = parseExpression(parser);
            
            if (!match(parser, TOK_SEMICOLON)) {
                parser->errors++;
            }
            
            return expr;
        }
    }
}

static ASTNode* parseIfStatement(Parser* parser) {
    ASTNode* ifNode = createNode(NODE_IF);
    
    advance(parser);  // Consume 'if'
    
    if (!match(parser, TOK_LPAREN)) {
        parser->errors++;
        return ifNode;
    }
    
    // Parse condition
    ASTNode* condition = parseExpression(parser);
    if (condition) {
        addChild(ifNode, condition);
    }
    
    if (!match(parser, TOK_RPAREN)) {
        parser->errors++;
    }
    
    // Parse 'then' branch
    ASTNode* thenBranch = parseStatement(parser);
    if (thenBranch) {
        addChild(ifNode, thenBranch);
    }
    
    // Parse 'else' branch if present
    if (match(parser, TOK_ELSE)) {
        ASTNode* elseBranch = parseStatement(parser);
        if (elseBranch) {
            addChild(ifNode, elseBranch);
        }
    }
    
    return ifNode;
}

static ASTNode* parseWhileStatement(Parser* parser) {
    ASTNode* whileNode = createNode(NODE_WHILE);
    
    advance(parser);  // Consume 'while'
    
    if (!match(parser, TOK_LPAREN)) {
        parser->errors++;
        return whileNode;
    }
    
    // Parse condition
    ASTNode* condition = parseExpression(parser);
    if (condition) {
        addChild(whileNode, condition);
    }
    
    if (!match(parser, TOK_RPAREN)) {
        parser->errors++;
    }
    
    // Parse body
    ASTNode* body = parseStatement(parser);
    if (body) {
        addChild(whileNode, body);
    }
    
    return whileNode;
}

// Main expression parser
ASTNode* parseExpression(Parser* parser) {
    ASTNode* left = parseUnary(parser);
    if (!left) {
        return NULL;
    }
    
    return parseBinaryRHS(parser, 1, left);
}

ASTNode* parseBlock(Parser* parser) {
    if (!match(parser, TOK_LBRACE)) {
        parser->errors++;
        return NULL;
    }
    
    ASTNode* block = createNode(NODE_BLOCK);
    
    while (peek(parser)->type != TOK_RBRACE && peek(parser)->type != TOK_EOF) {
        ASTNode* statement = parseStatement(parser);
        if (statement) {
            addChild(block, statement);
        }
    }
    
    if (!match(parser, TOK_RBRACE)) {
        parser->errors++;
    }
    
    return block;
}

// Main parse function
ASTNode* parse(Token* SrcTokens) {
    Parser parser = {
        .tokens = SrcTokens,
        .current = NULL,
        .errors = 0
    };

    ASTNode* ast = NULL;
    if(SrcTokens->type != TOK_EOF) {
        parser.current = SrcTokens;
        ast = parseTranslationUnit(&parser);
    }
    
    if (parser.errors > 0) {
        // Handle errors, maybe free the AST and return NULL
        printf("Parsing failed with %d errors\n", parser.errors);
    } 

    return ast;
}