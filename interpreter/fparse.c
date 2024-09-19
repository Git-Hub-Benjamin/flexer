#include <token.h>
#include <stdbool.h>

extern TokenStream SrcTokenStream;


char** ExpectErrors = {
    "Declaraction (void, int, char)",
    "Function argument ()"
};

TokenType expect_declaration[] = {TOK_VOID, TOK_INT, TOK_CHAR}; // --> expects identifer
TokenType expect_identifer[] = {TOK_IDENTIFER}; // --> expects = OR ; OR (  
TokenType expect_right_parathesise[] = {TOK_RPAREN};
TokenType expect_left_parathesise[] = {TOK_LPAREN}; // expects 
TokenType expect_statement_end[] = {TOK_COMMA};
TokenType expect_function_argument[] = {TOK_IDENTIFER, TOK_IMMEDIATE, TOK_COMMA, TOK_RPAREN};
TokenType expect_assignment[] = {TOK_EQUAL}; // expects IMM or IDENTIFER
TokenType expect_immediate_value[] = {TOK_IMMEDIATE};
TokenType expect_identifer_statement_end[] = {TOK_IDENTIFER}; // expects ; --> speical for when int a = b; --> b expects a ";" not a "="
TokenType expect_keyword[] = {TOK_CONTINUE, TOK_BREAK, TOK_WHILE, TOK_IF};
TokenType expect_left_parathesise_func[] = {TOK_LPAREN};

typedef struct {
    TokenType* expectedTokens;
    int len;
    EXPECT* expectNext;
    int nextLen;
} Expect;

typedef enum {
    EXPECT_DECLATION,
    EXPECT_IDENTIFER,
    EXPECT_ASSIGNMENT,
    EXPECT_STATEMENT_END,
    EXPECT_KEYWORD,
    EXPECT_LPARATHESIS_PARAM, // for function arguments 
    EXPECT_LPARATHESIS_COND, // for conditionals if, while else if

    EXPECT_IDENTIFER_STATEMENT_END,
    EXPECT_IMMEDIATE_VALUE,
    EXPECT_FUNCTION_PARAMETER,
    EXPECT_FUNCTION_ARGUMENT,
    EXPECT_RPARATHESIS,
    EXPECT_LBRACKET
} EXPECT ;

EXPECT declaration_expect_next[] = {EXPECT_IDENTIFER};
EXPECT identifer_expect_next[] = {EXPECT_ASSIGNMENT, EXPECT_STATEMENT_END, EXPECT_LPARATHESIS_PARAM};
EXPECT assignment_expect_next[] = {EXPECT_IMMEDIATE_VALUE, EXPECT_IDENTIFER_STATEMENT_END};
EXPECT statement_end_expect_next[] = {EXPECT_DECLATION, EXPECT_KEYWORD};
EXPECT keyword_expect_next[] = {EXPECT_STATEMENT_END, EXPECT_LPARATHESIS_COND, EXPECT_LBRACKET};
EXPECT function_expect_params[] = {};


// EXPECT indexes into Expects, so they must match up

Expect Expects[] = {
    {expect_declaration, sizeof(expect_declaration), declaration_expect_next, sizeof(declaration_expect_next)}, 
    {expect_identifer, sizeof(expect_identifer), identifer_expect_next, sizeof(identifer_expect_next)},
    {expect_assignment, sizeof(expect_assignment), assignment_expect_next, sizeof(assignment_expect_next)},
    {expect_statement_end, sizeof(expect_statement_end), statement_end_expect_next, sizeof(statement_end_expect_next)},
    {expect_keyword, sizeof(expect_keyword), keyword_expect_next, sizeof(keyword_expect_next)},
    {expect_left_parathesise_func, sizeof(expect_left_parathesise_func), function_arg_param_expect_next, sizeof(function_arg_param_expect_next)},

};

bool verifyTokToExpect(EXPECT expect, TokenType token){
    for(int expect_count = 0; expect < Expects[expect].len; expect_count++)
        if (token == Expects[expect].expectedTokens[expect_count])
            return true;
    return false;
}

int parse() {
    bool global = true;
    EXPECT expect = EXPECT_DECLATION;
    for (int token_index = 0; token_index < SrcTokenStream.basicTokenIndex; token_index++) {
        if (verifyTokToExpect(expect, SrcTokenStream.Tokens[token_index].token))
            expect = Expects[expect].expectNext;
        else
            printf("Unexpected token... Expected --> %s", ExpectErrors[expect]);
    }
}