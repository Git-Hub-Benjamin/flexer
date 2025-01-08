#include <stdlib.h>
#include <string.h>
#include "./flexer/token.h"

TokenStream tokenStream;

static void newTokenIndex() {
    tokenStream.tokenIndex++;
    if (tokenStream.tokenIndex == tokenStream.tokenCapacity) {
        size_t newCapacity = tokenStream.tokenCapacity * 2;
        void* newStream = malloc(sizeof(Token) * newCapacity);
        memset(newStream, 0, sizeof(Token) * newCapacity);
        memcpy(newStream, tokenStream.Tokens, sizeof(Token) * tokenStream.tokenIndex);
        free(tokenStream.Tokens);
        tokenStream.Tokens = newStream;
        tokenStream.tokenCapacity = newCapacity;
    }
}

int main() {
    
    // initalize
    tokenStream.Tokens = (Token*)malloc(sizeof(Token) * TOKEN_STREAM_INITAL_CAPACITY);
    tokenStream.tokenCapacity = TOKEN_STREAM_INITAL_CAPACITY;
    tokenStream.tokenIndex = 0;

    // add tokens
    for(int i = 0; i < 10; i++) {
        tokenStream.Tokens[tokenStream.tokenIndex].type = TOK_INTEGER_LITERAL;
        tokenStream.Tokens[tokenStream.tokenIndex].line = 1;
        tokenStream.Tokens[tokenStream.tokenIndex].col = i + 1;
        newTokenIndex();
    }

    return 0;
}