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

    __TOK__DIVIDER__OPERATORS__,

    // Operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_PLUSPLUS,
    TOK_MINUSMINUS,
    TOK_EQUAL_EQUAL,
    TOK_BANG_EQUAL,
    TOK_GREATER,
    TOK_LESS,
    TOK_GREATER_EQUAL,
    TOK_LESS_EQUAL,
    TOK_AND_AND,
    TOK_OR_OR,
    TOK_BANG,
    TOK_AMPERSAND,
    TOK_PIPE,
    TOK_CARET,
    TOK_TILDE,
    TOK_LSHIFT,
    TOK_RSHIFT,
    TOK_EQUAL,
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
    TOK_INCLUDE,
    TOK_DEFINE,

    // Additional common tokens
    TOK_MAIN,
    TOK_IDENTIFER,
    TOK_LITERAL,
    TOK_IMMEDIATE,

    // Total number of tokens
    TOK_TOTAL,
    TOK_SKIP,
} TokenType;

/*

enum tokenType {
    // Unary Operators
    INC,    // increment 1
    DEC,    // decrement 1
    NEG,    // negation

    // Multiplication, Division, Modulus
    MUL,    // multiplication *
    DIV,    // division /
    MOD,    // modulus %

    // Addition, Subtraction
    ADD,    // addition +
    SUB,    // subtraction -

    // Bitwise Shift
    LSH,    // left shift <<
    RSH,    // right shift >>

    // Relational Operators
    LTHEN,  // less than <
    LTHENEQ,// less than or equal to <=
    GTHEN,  // greater than >
    GTHENEQ,// greater than or equal to >=

    // Equality Operators
    EQUAL,  // equal to ==
    NOTEQUAL,// not equal to !=

    // Bitwise AND
    BAND,   // bitwise AND &

    // Bitwise XOR
    XOR,    // bitwise XOR ^

    // Bitwise OR
    BOR,    // bitwise OR |

    // Logical AND
    LAND,   // logical AND &&

    // Logical OR
    LOR,    // logical OR ||

    // Assignment Operators
    ASSIGN, // assignment =
    ADDEQ,  // addition assignment +=
    SUBEQ,  // subtraction assignment -=
    MULEQ,  // multiplication assignment *=
    DIVEQ,  // division assignment /=
    MODEQ,  // modulus assignment %=
    LSHEQ,  // left shift assignment <<= 
    RSHEQ,  // right shift assignment >>=
    XOREQ,  // bitwise XOR assignment ^=
    BOREQ,  // bitwise OR assignment |=
    BANDEQ, // bitwise AND assignment &=
    
    // Other Tokens
    COMMA,  // comma ,
    COLON,  // colon :
    SEMI,   // semicolon ;
    BCOMP,  // bitwise complement ~
    LCURLY, // left curly brace {
    RCURLY, // right curly brace }
    LBRAC,  // left square bracket [
    RBRAC,  // right square bracket ]
    LPARA,  // left parenthesis (
    RPARA,  // right parenthesis )
    IMM,    // immediate value (not an operator but a token)
    IDENTIFER, // identifier (not an operator but a token)
    LITERAL  // literal value (not an operator but a token)
};

*/

typedef struct {
    int line;
    int col;
    TokenType token;
    union {
        int self_basic_token_num; // just for readability (if this token has no attribute then the token number will be itself)
        int other_attr_token_num; // if it has an attribute, the token num here will be the attribute token num and will be stored here
    } token_num;
} Token;

typedef struct {
    void* data;
    int other_basic_token_num;
    TokenType token;
} AttrToken ;

typedef struct  {
    char* data;
    int len;
} StringToken;

#define TOKEN_STREAM_INITAL_CAPACITY 512

typedef struct {
    Token* Tokens;
    size_t basicTokenCapacity;
    size_t basicTokenIndex;
    AttrToken* AttrTokens;
    size_t attrTokenCapacity;
    size_t attrTokenIndex;
} TokenStream;


#endif