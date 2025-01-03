#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include "token.h"
#include "../hash/hash.h"

const char* TokenStrings[] = {
    // Keywords
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "inline", "int", "long", "register", "return", "short", "signed", "sizeof",
    "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
    "define", "include", "ifdef", "elif", "endif",

    "_TOK_DIVIDER_", // 39

    // Operators
    "+", "-", "*", "/", "%", "++", "--", "==", "!=", ">", "<", ">=", "<=",
    "&&", "||", "!", "&", "|", "^", "~", "<<", ">>", "=", "+=", "-=", "*=",
    "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",

    "_TOK_DIVIDER_", // 73

    // Punctuation
    "{", "}", "[", "]", "(", ")", ";", ",", ".", "->", ":", "#",

    // Additional common tokens
    "main", "identifer", "literal", "immediate", "TOK_SKIP", "TOK_SKIP_IMM", "TOK_SKIP_LIT", "TOK_TOTAL"
}; 
void EXIT_FAIL_MSG(const char* msg) {
    printf("%s\n", msg);
    exit(EXIT_FAILURE);
}

ht* keywordTokenConverter;
TokenStream SrcTokenStream; // Source file token stream
static Token* tokens; // tmp pointer to the same address as (SrcTokenStream.tokens)

// see if we need to expand the stream
enum ts_type {BASIC, ATTR};
static void ts_new_index(enum ts_type type) {
    if(SrcTokenStream.tokenIndex >= SrcTokenStream.tokenCapacity) {
        size_t new_capacity = SrcTokenStream.tokenCapacity * 2;

        if (new_capacity < SrcTokenStream.tokenCapacity) 
            EXIT_FAIL_MSG("OVERFLOW...");
        // 0 out new memory
        void* new_stream = calloc(new_capacity, sizeof(Token));
        if (new_stream == NULL)
            EXIT_FAIL_MSG("NO MEMORY...");

        // copy old data to new pointer
        memcpy(new_stream, tokens, sizeof(Token) * (SrcTokenStream.tokenIndex + 1));

        // free old stream & update SrcTokenStream pointer & update capacity
        SrcTokenStream.tokenCapacity = new_capacity;
        SrcTokenStream.Tokens = new_stream;
        free(tokens);

        // update tmp pointer for lexer to new pointer
        tokens = SrcTokenStream.Tokens;
    }
    SrcTokenStream.tokenIndex++;
}


typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

static bool isAtEnd() {
    return *scanner.current == '\0';
}

static char peek() {
    return *scanner.current;
}

static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1]; // 1 ahead of current
}

static char peekSuperNext() {
    if (scanner.current[1] == '\0') return '\0';
    return scanner.current[2]; // 2 ahead of current
}

static char advance() {
    scanner.current++;
    return scanner.current[-1]; // 1 behind increment
}

static char retreat() {
    scanner.current--;
    return scanner.current[1];
}

static void updateStart() {
    scanner.start = scanner.current;
}

static void skipWhitespace(int* col) {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                (*col)++; // maybe different for \t?
                advance();
                break;
            default:
                updateStart();
                return;
        }
    }
}

static void keywordIdToken() {
    // get pointer to end of string
    do {
        advance(); 
    } while ((peek() >= 'A' && peek() <= 'Z') || (peek() >= 'a' && peek() <= 'z') || peek() == '_');

    // save the char here
    char saved = peek();
    // null terminate temporarily
    *(char*)scanner.current = '\0'; // lmao - override the const for a second

    // search for keyword
    void* tokdata = ht_get(keywordTokenConverter, scanner.start);
    if (tokdata != NULL) { // found keyword 
        tokens[SrcTokenStream.tokenIndex].type = *(TokenType*)tokdata;
    } else { // copy the identifer into attr token
        tokens[SrcTokenStream.tokenIndex].type = TOK_IDENTIFER; // set basic token
        tokens[SrcTokenStream.tokenIndex].data = (char*)malloc(scanner.current - scanner.start + 1); // malloc size of string
        if (tokens[SrcTokenStream.tokenIndex].data == NULL)
            EXIT_FAIL_MSG("NO MEMORY...");
        strcpy(tokens[SrcTokenStream.tokenIndex].data, scanner.start); // copy 
    }

    // restore saved char
    *(char*)scanner.current = saved;
}


// does not support (FLOATS, DOUBLE)
void immToken() {
    enum immType {HEX, BIN, OCT, DEC} type;

    if (peek() == '0')
        if (peekNext() == 'x' || peekNext() == 'X')
            type = HEX;
        else if (peekNext() == 'b' || peekNext() == 'B')
            type = BIN;
        else
            type = OCT;
    else
        type = DEC;
    switch(type){
        case HEX:
        case BIN:
            advance();
        case OCT:
            advance();
        default:
            break;
    }

    updateStart(); // update start to new current

    // get pointer to end of immediate value
    if (type == BIN || type == DEC) // BINARY & DECIAML
        while(peek() >= '0' && peek() <= '9') advance();
    else if (type == HEX) // HEX
        while((peek() >= '0' && peek() <= '9') || (peek() >= 'A' && peek() <= 'F') || (peek() >= 'a' && peek() <= 'f')) advance();
    else if(type == BIN)
        while(peek() >= '0' && peek() <= '1') advance();
    else // OCTAL
        while(peek() >= '0' && peek() <= '7') advance();

    retreat(); // move to the last digit

    uint32_t num = 0;
    int digit_count = 0;

    // line_ptr pointing char in num, we are going from back to front
    while(scanner.current >= scanner.start) {
        if (type == DEC)
            num += (peek() - '0') * (uint32_t)pow(10, digit_count);
        else if(type == HEX)
            if (peek() >= 'A' && peek() <= 'F')
                num += (peek() - 55) * (uint32_t)pow(16, digit_count);
            else if (peek() >= 'a' && peek() <= 'f')
                num += (peek() - 87) * (uint32_t)pow(16, digit_count);
            else
                num += (peek() - 48) * (uint32_t)pow(16, digit_count);
        else if(type == BIN) {
            if (peek() == '1')
                num += (uint32_t)pow(2, digit_count);
        } else 
            num += (peek() - 48) * (uint32_t)pow(8, digit_count);
        digit_count++;
        retreat();
    }

    // get position to after last digit
    scanner.current += digit_count + 1; 

    // store the result
    tokens[SrcTokenStream.tokenIndex].type = TOK_IMMEDIATE;
    tokens[SrcTokenStream.tokenIndex].data = malloc(sizeof(int));
    *(int*)(tokens[SrcTokenStream.tokenIndex].data) = (int)num;
}


// "abc"
void literalToken() {
    if (peekNext() == '"')
        EXIT_FAIL_MSG("NO EMPTY STRING LITERALS...");
    advance(); // go to first character
    updateStart(); 
    while (peek() != '"') {
        if (isAtEnd()) // check for null in case user never closes quotes
            EXIT_FAIL_MSG("NO NEVER ENDING STRING LITERALS...");
        else if (peek() == '\n')
            scanner.line++;
        advance();
    } 

    tokens[SrcTokenStream.tokenIndex].data = malloc(scanner.current - scanner.start); // we dont have to plus 1 because scanner.current is already 1 ahead of last character at "
    memcpy(tokens[SrcTokenStream.tokenIndex].data, scanner.start, scanner.current - scanner.start ); 
    tokens[SrcTokenStream.tokenIndex].data[scanner.current - scanner.start] = '\0';

    advance(); // go past ending quote

    // set token
    tokens[SrcTokenStream.tokenIndex].type = TOK_LITERAL;
}

int tokenize(const char* source) {
    // initalize scanner
    scanner.line = 1;
    scanner.start = source;
    scanner.current = source;
    int column = 1;
    const char* begginingLineTracker = source;

    while (!isAtEnd()) {

        skipWhitespace(&column); // skip whitespace

        if (peek() == '\n') { // check for new line
            column = 1;
            scanner.line++;
            advance();
            updateStart();
            begginingLineTracker = scanner.current;
            continue;
        }

        bool debug = false;
        if (peek() == '"')
            debug = true;

        switch (peek()) {
            case '#':
                tokens[SrcTokenStream.tokenIndex].type = TOK_HASH; break;
            case '(':
                tokens[SrcTokenStream.tokenIndex].type = TOK_LPAREN; break;
            case ')':
                tokens[SrcTokenStream.tokenIndex].type = TOK_RPAREN; break;
            case '[':
                tokens[SrcTokenStream.tokenIndex].type = TOK_LBRACKET; break;
            case ']':
                tokens[SrcTokenStream.tokenIndex].type = TOK_RBRACKET; break;
            case '{':
                tokens[SrcTokenStream.tokenIndex].type = TOK_LBRACE; break;
            case '}':
                tokens[SrcTokenStream.tokenIndex].type = TOK_RBRACE; break;
            case ',':
                tokens[SrcTokenStream.tokenIndex].type = TOK_COMMA; break;
            case ':':
                tokens[SrcTokenStream.tokenIndex].type = TOK_COLON; break;
            case ';':
                tokens[SrcTokenStream.tokenIndex].type = TOK_SEMICOLON; break;
            case '~':
                tokens[SrcTokenStream.tokenIndex].type = TOK_TILDE; break;
            case '*':
                if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_STAR_EQUAL;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_STAR;
                break;
            case '/':
                if (peekNext() == '/')
                    break; // Comment, end of line
                else if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_SLASH_EQUAL;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_SLASH;
                break;
            case '+':
                if (peekNext() == '+')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_PLUSPLUS;
                else if (peekNext() == '=') 
                    tokens[SrcTokenStream.tokenIndex].type = TOK_PLUS_EQUAL;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_PLUS;
                break;
            case '-':
                if (peekNext() == '-')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_MINUSMINUS;
                else if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_MINUS_EQUAL;
                else
                    if (peekNext() >= '0' && peekNext() <= '9') {
                        advance(); // advance pass negative sign
                        immToken(); // get number
                        *((int*)tokens[SrcTokenStream.tokenIndex].data) = *((int*)tokens[SrcTokenStream.tokenIndex].data) * -1; // make negative
                    } else
                        tokens[SrcTokenStream.tokenIndex].type = TOK_MINUS;
                break;
            case '=':
                if (peekNext() == '=') 
                    tokens[SrcTokenStream.tokenIndex].type = TOK_EQUAL_EQUAL;
                else 
                    tokens[SrcTokenStream.tokenIndex].type = TOK_EQUAL;
                break;
            case '&':
                if (peekNext() == '&')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_AND_AND;
                else if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_AMPERSAND_EQUAL;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_AMPERSAND;
                break;
            case '|':
                if (peekNext() == '|')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_OR_OR;
                else if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_PIPE_EQUAL;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_PIPE;
                break;
            case '^':
                if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_CARET_EQUAL;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_CARET;
                break;
            case '%':
                if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_PERCENT_EQUAL;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_PERCENT;
                break;
            case '>':
                if (peekNext() == '>')
                    if (peekSuperNext() == '=')
                        tokens[SrcTokenStream.tokenIndex].type = TOK_RSHIFT_EQUAL;
                    else
                        tokens[SrcTokenStream.tokenIndex].type = TOK_RSHIFT;
                else if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_GREATER_EQUAL;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_GREATER;
                break;
            case '<':
                if (peekNext() == '<')
                    if (peekSuperNext() == '=')
                        tokens[SrcTokenStream.tokenIndex].type = TOK_LSHIFT_EQUAL;
                    else
                        tokens[SrcTokenStream.tokenIndex].type = TOK_LSHIFT;
                else if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_LESS_EQUAL;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_LESS;
                break;
            case '!':
                if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_BANG_EQUAL;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_BANG;
                break;
            default:
                if (peek() >= '0' && peek() <= '9')
                    immToken(); // this does NOT support negative
                else if (peek() == '"') {
                    literalToken(); 
                } else if ((peek() >= 'A' && peek() <= 'Z') || (peek() >= 'a' && peek() <= 'z') || peek() == '_')  
                    keywordIdToken();
                else 
                    EXIT_FAIL_MSG("TOKENIZE: UNKNOWN CHARACTER...");
                break;
        }

        // 1. Token has been set
        
        // 2. Set line and col of token
        tokens[SrcTokenStream.tokenIndex].col = column;
        tokens[SrcTokenStream.tokenIndex].line = scanner.line;

        // 3. Update column for special cases           (if keyword, ID, LITERAL or IMM)
        if (tokens[SrcTokenStream.tokenIndex].type < __TOK__DIVIDER__OPERATORS__ || tokens[SrcTokenStream.tokenIndex].type >= TOK_IDENTIFER && tokens[SrcTokenStream.tokenIndex].type <= TOK_IMMEDIATE) {
            column = scanner.current - begginingLineTracker;
            retreat(); // because its going to be incremented automatically too
        }

        // 4. Increment line_ptr & column for Operators that need it 
        switch(tokens[SrcTokenStream.tokenIndex].type) {
            case TOK_RSHIFT_EQUAL: // 3 character operators
            case TOK_LSHIFT_EQUAL:
            case TOK_CARET_EQUAL:
                advance();
                column++;
            case TOK_PERCENT_EQUAL: // 2 character operators 
            case TOK_AMPERSAND_EQUAL:
            case TOK_PIPE_EQUAL:
            case TOK_STAR_EQUAL:
            case TOK_SLASH_EQUAL:
            case TOK_MINUS_EQUAL:
            case TOK_PLUS_EQUAL:
            case TOK_MINUSMINUS:
            case TOK_PLUSPLUS:
            case TOK_BANG_EQUAL:
            case TOK_EQUAL_EQUAL:
            case TOK_GREATER_EQUAL:
            case TOK_LESS_EQUAL:
            case TOK_OR_OR:
            case TOK_AND_AND:
            case TOK_RSHIFT:
            case TOK_LSHIFT:
                advance();
                column++;
            default:
                break;
        }

        // 5. Increment basic token index
        ts_new_index(BASIC);
next_character:
        advance();
        column++;
        updateStart();
    }

    // set last ending token to TOT_TOTAL
    tokens[SrcTokenStream.tokenIndex].type = TOK_TOTAL;
    return 1; // good
}

extern bool hashPreprocessorDirectives(ht*);
extern bool preProcessorMacroExpansion(ht*);
static ht* preProcessHt;

static char* getFileSourcePointer(const char* path) {
    FILE* file = fopen(path, "rb");
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(fileSize + 1);
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}

static void printTokens(){
    for (int i = 0; i < SrcTokenStream.tokenIndex; i++) {
        printf("<line: %d, col: %d, token: %s", tokens[i].line, tokens[i].col, TokenStrings[tokens[i].type]);

        if (tokens[i].type >= TOK_IDENTIFER && tokens[i].type <= TOK_IMMEDIATE) {
            if (tokens[i].type == TOK_IMMEDIATE)
                printf(", data: %d", (*(int*)(tokens[i].data)));
            else
                printf(", data: %s", ((char*)tokens[i].data));
        }
        printf(">\n");
    }
}

void ht_print(ht* table) {
    printf("Table length: %lu\n", table->length);
    hti tmp = ht_iterator(table);
    for(int i = 0; i < table->length; i++) {
        if(ht_next(&tmp)) {
            printf("KEY: %s, VAL: %d\n", tmp.key, *(int*)((Token*)tmp.value)->data);
        }
        
    }
}

extern ht* preProcessHt;

// [0] path to file
int main(int argc, char** agrv) {

    if (argc < 1) 
        EXIT_FAIL_MSG("PROVIDE FILE PATH...");

    // get pointer to file data 
    const char* source = getFileSourcePointer(agrv[1]);

    // Setup keyword hash table
    keywordTokenConverter = ht_create((int)__TOK__DIVIDER__OPERATORS__);
    for(int i = 0; i < __TOK__DIVIDER__OPERATORS__; i++) {
        void* tokdata = malloc(sizeof(TokenType));
        if (tokdata == NULL) {
            printf("No memory...\n");
            exit(EXIT_FAILURE);
        }
        *(TokenType*)tokdata = (TokenType)i;
        ht_set(keywordTokenConverter, TokenStrings[i], tokdata);
    }

    // setup token stream
    SrcTokenStream.tokenCapacity = TOKEN_STREAM_INITAL_CAPACITY;
    SrcTokenStream.Tokens = (Token*)calloc(TOKEN_STREAM_INITAL_CAPACITY, sizeof(Token));
    if (SrcTokenStream.Tokens == NULL)
        EXIT_FAIL_MSG("NO MEMORY");

    // setup tmp pointers
    tokens = SrcTokenStream.Tokens;

    if (!tokenize(source))
        EXIT_FAIL_MSG("TOKEN ERROR...");

    // create preProcessHt
    preProcessHt = ht_create(-1);
    if (preProcessHt == NULL)
        EXIT_FAIL_MSG("NO MEMORY...");

    if (!hashPreprocessorDirectives(preProcessHt))
        EXIT_FAIL_MSG("PREPROCESS ERROR...");

    if (!preProcessorMacroExpansion(preProcessHt))
        EXIT_FAIL_MSG("PREPROCESS EXPANSION ERROR...");

    printTokens();

    printf("File path: %s\n", agrv[1]);
    return 0;
}