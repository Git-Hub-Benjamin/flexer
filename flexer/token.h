#ifndef TOKEN_H
#define TOKEN_H

#include <stdlib.h>

typedef enum {
    // Keywords
    TOK_AUTO,
    TOK_BREAK,
    TOK_CASE,
    TOK_CHAR,
    TOK_CONST,
    TOK_CONTINUE,
    TOK_DEFAULT,
    TOK_DO,
    TOK_DOUBLE,
    TOK_ELSE,
    TOK_ENUM,
    TOK_EXTERN,
    TOK_FLOAT,
    TOK_FOR,
    TOK_GOTO,
    TOK_IF,
    TOK_INLINE,
    TOK_INT,
    TOK_LONG,
    TOK_REGISTER,
    TOK_RETURN,
    TOK_SHORT,
    TOK_SIGNED,
    TOK_SIZEOF,
    TOK_STATIC,
    TOK_STRUCT,
    TOK_SWITCH,
    TOK_TYPEDEF,
    TOK_UNION,
    TOK_UNSIGNED,
    TOK_VOID,
    TOK_VOLATILE,
    TOK_WHILE,
    TOK_DEFINE,
    TOK_INCLUDE,
    TOK_IFDEF,
    TOK_ELIF,
    TOK_ENDIF,

    __TOK__DIVIDER__OPERATORS__,

    // Operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_PLUSPLUS,
    TOK_MINUSMINUS,
    TOK_EQ,
    TOK_NEQ,
    TOK_GT,
    TOK_LT,
    TOK_GTE,
    TOK_LTE,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_AMPERSAND,
    TOK_PIPE,
    TOK_CARET,
    TOK_TILDE,
    TOK_LSHIFT,
    TOK_RSHIFT,
    TOK_EQUALS,
    TOK_PLUS_EQUAL,
    TOK_MINUS_EQUAL,
    TOK_STAR_EQUAL,
    TOK_SLASH_EQUAL,
    TOK_PERCENT_EQUAL,
    TOK_AMPERSAND_EQUAL,
    TOK_PIPE_EQUAL,
    TOK_CARET_EQUAL, //
    TOK_LSHIFT_EQUAL, //
    TOK_RSHIFT_EQUAL, // 

    __TOK__DIVIDER__PUNCTUATION__,

    // Punctuation
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_DOT,
    TOK_ARROW,
    TOK_COLON,
    TOK_HASH,

    // Additional common tokens
    TOK_MAIN,
    TOK_IDENTIFIER,
    TOK_STRING_LITERAL,
    TOK_INTEGER_LITERAL,

    // Total number of tokens
    TOK_SKIP,
    TOK_SKIP_IMM,
    TOK_SKIP_LIT,
    TOK_EOF,
} TokenType;

#define TOKEN_STREAM_INITAL_CAPACITY 2

typedef struct {
    char* data; // for imm, id, literal
    TokenType type;
    int line;
    int col;
} Token;

typedef struct {
    Token* Tokens;
    size_t tokenCapacity;
    size_t tokenIndex;
} TokenStream;


#endif