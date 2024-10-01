#include <token.h>
#include <stdbool.h>

extern TokenStream SrcTokenStream;


char** ExpectErrors = {
    "Declaraction (void, int, char)",
    "Function argument ()"
};

typedef struct Node {} Node;

typedef struct Parser {
    TokenStream* stream;
    Token* current_token;
} Parser;

static size_t curr_token_index = 0;
Parser SrcPraser;

int curr_line;
int curr_col;
TokenType curr_token;
void* curr_data;

static bool in_function = false;
static bool global = true;

void fparse() {
    // init parser
    SrcPraser.stream = &SrcTokenStream;
    SrcPraser.current_token = &SrcPraser.stream->Tokens[0];

    // parse
    while (!next_token()) {
        if (global) {
            if (curr_token == TOK_INT || curr_token == TOK_CHAR || curr_token == TOK_VOID) {
                
            }

        } else {

        }
    }
}

static bool next_token() {
    if (SrcPraser.current_token->type == TOK_TOTAL)
        return false;
    SrcPraser.current_token = SrcPraser.current_token + 1;
    curr_line = SrcPraser.current_token->line;
    curr_col = SrcPraser.current_token->col;
    curr_token = SrcPraser.current_token->type;
    curr_data = SrcPraser.current_token->data;
}

Node* parse_declaration() {
    
}