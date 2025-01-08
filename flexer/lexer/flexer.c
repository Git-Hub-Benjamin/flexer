#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "../token.h"
#include "../hash/hash.h"
#include "../platform.h"
#include "../flexer.h"
#include "../parser/fparse.h"
#include "../terminal-colors.h"

extern bool hashPreprocessorDirectives(ht*);
extern bool preProcessorMacroExpansion(ht*);
static ht* preProcessHt;
Config flexerCfg;

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
    "main", "identifer", "literal", "immediate", "TOK_SKIP", "TOK_SKIP_IMM", "TOK_SKIP_LIT", "TOK_EOF"
}; 
void EXIT_FAIL_MSG(const char* msg) {
    printf("%s\n", msg);
    exit(EXIT_FAILURE);
}

ht* keywordTokenConverter;
TokenStream SrcTokenStream; // Source file token stream
static Token* tokens; // tmp pointer to the same address as (SrcTokenStream.tokens)

// see if we need to expand the stream
static void ts_new_index() {
    SrcTokenStream.tokenIndex++;
    if(SrcTokenStream.tokenIndex == SrcTokenStream.tokenCapacity) {
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
        tokens[SrcTokenStream.tokenIndex].type = TOK_IDENTIFIER; // set basic token
        size_t len = scanner.current - scanner.start + 1; // +1 for null terminator
        tokens[SrcTokenStream.tokenIndex].data = (char*)malloc(len); // malloc size of string
        if (tokens[SrcTokenStream.tokenIndex].data == NULL)
            EXIT_FAIL_MSG("NO MEMORY...");
#if defined(_WIN32) || defined(_WIN64)
        strncpy_s(tokens[SrcTokenStream.tokenIndex].data, // destination
                 len,                                     // size of destination buffer
                 scanner.start,                          // source
                 len - 1);                              // number of characters to copy
#else
        strncpy(tokens[SrcTokenStream.tokenIndex].data, scanner.start, len);
#endif
        tokens[SrcTokenStream.tokenIndex].data[len - 1] = '\0';
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
    tokens[SrcTokenStream.tokenIndex].type = TOK_INTEGER_LITERAL;
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
    tokens[SrcTokenStream.tokenIndex].type = TOK_STRING_LITERAL;
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

        switch (peek()) {
            case '#':
                tokens[SrcTokenStream.tokenIndex].type = TOK_HASH; break;
            case '.':
                tokens[SrcTokenStream.tokenIndex].type = TOK_DOT; break;
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
                else if (peekNext() == '>')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_ARROW;
                else //! TODO: CHECK THIS WORKS
                    if (peekNext() >= '0' && peekNext() <= '9') {
                        advance(); // advance pass negative sign
                        immToken(); // get number
                        *((int*)tokens[SrcTokenStream.tokenIndex].data) = *((int*)tokens[SrcTokenStream.tokenIndex].data) * -1; // make negative
                    } else
                        tokens[SrcTokenStream.tokenIndex].type = TOK_MINUS;
                break;
            case '=':
                if (peekNext() == '=') 
                    tokens[SrcTokenStream.tokenIndex].type = TOK_EQ;
                else 
                    tokens[SrcTokenStream.tokenIndex].type = TOK_EQUALS;
                break;
            case '&':
                if (peekNext() == '&')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_AND;
                else if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_AMPERSAND_EQUAL;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_AMPERSAND;
                break;
            case '|':
                if (peekNext() == '|')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_OR;
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
                    tokens[SrcTokenStream.tokenIndex].type = TOK_GTE;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_GT;
                break;
            case '<':
                if (peekNext() == '<')
                    if (peekSuperNext() == '=')
                        tokens[SrcTokenStream.tokenIndex].type = TOK_LSHIFT_EQUAL;
                    else
                        tokens[SrcTokenStream.tokenIndex].type = TOK_LSHIFT;
                else if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_LTE;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_LT;
                break;
            case '!':
                if (peekNext() == '=')
                    tokens[SrcTokenStream.tokenIndex].type = TOK_NEQ;
                else
                    tokens[SrcTokenStream.tokenIndex].type = TOK_NOT;
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
        if (tokens[SrcTokenStream.tokenIndex].type < __TOK__DIVIDER__OPERATORS__ || tokens[SrcTokenStream.tokenIndex].type >= TOK_IDENTIFIER && tokens[SrcTokenStream.tokenIndex].type <= TOK_INTEGER_LITERAL) {
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
            case TOK_ARROW:
            case TOK_NEQ:
            case TOK_EQ:
            case TOK_GTE:
            case TOK_LTE:
            case TOK_OR:
            case TOK_AND:
            case TOK_RSHIFT:
            case TOK_LSHIFT:
                advance();
                column++;
            default:
                break;
        }

        // 5. Increment basic token index
        ts_new_index();
next_character:
        advance();
        column++;
        updateStart();
    }

    // set last ending token to TOT_TOTAL
    tokens[SrcTokenStream.tokenIndex].type = TOK_EOF;
    return 1; // good
}

static char* getFileSourcePointer(const char* path) {
    FILE* file;
    SAFE_FOPEN(file, path, "rb");    
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fclose(file);
        EXIT_FAIL_MSG("NO MEMORY...");
        return NULL;
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}

// Function to print tokens in color

void printColoredToken(int tokenType, const char* tokenString, void* tokenData) {
   printf("❬ ");
   switch (tokenType) {
       case TOK_IDENTIFIER:
           printf(COLOR_BRIGHT_GREEN "ID→ %s" COLOR_RESET, tokenString);
           if (tokenData) printf(COLOR_CYAN " 「%s 」" COLOR_RESET, (char*)tokenData);
           break;
       case TOK_INTEGER_LITERAL:
           printf(COLOR_BRIGHT_YELLOW "INT→ %s" COLOR_RESET, tokenString);
           if (tokenData) printf(COLOR_CYAN " 「%d 」" COLOR_RESET, *(int*)tokenData);
           break;
       default:
           if (tokenType < __TOK__DIVIDER__OPERATORS__)
               printf(COLOR_BRIGHT_BLUE "KW→ %s" COLOR_RESET, tokenString);
           else
               printf(COLOR_BRIGHT_MAGENTA "OP→ %s" COLOR_RESET, tokenString);
   }
   printf("❭");
}

static int getDigits(int num) {
   if (num == 0) return 1;
   return (int)log10(abs(num)) + 1;
}

void printTokens() {
   FILE* file = flexerCfg.verbose_mode ? fopen("tokens_output.txt", "w") : NULL;

   // Calculate max widths
   int maxLine = 0, maxCol = 0, maxIndex = getDigits(SrcTokenStream.tokenIndex);
   for (int i = 0; i < SrcTokenStream.tokenIndex; i++) {
       int lineDigits = getDigits(tokens[i].line);
       int colDigits = getDigits(tokens[i].col);
       maxLine = lineDigits > maxLine ? lineDigits : maxLine;
       maxCol = colDigits > maxCol ? colDigits : maxCol;
   }

   for (int i = 0; i < SrcTokenStream.tokenIndex; i++) {
       if (flexerCfg.verbose_mode) {
           printf("│ %*d:%*d ", maxLine, tokens[i].line, maxCol, tokens[i].col);
           printColoredToken(tokens[i].type, TokenStrings[tokens[i].type], tokens[i].data);
           printf(" %*d", maxIndex, i);
       } else {
           if (tokens[i].type < __TOK__DIVIDER__OPERATORS__)
               printf("📑 ");
           printColoredToken(tokens[i].type, TokenStrings[tokens[i].type], tokens[i].data);
           printf(" №%*d", maxIndex, i);
       }
       printf("\n");
       if (file) fprintf(file, "\n");
   }

   if (file) fclose(file);
}

void ht_print(ht* table) {
    printf("Table length: " SIZE_T_FMT "\n", table->length);
    hti tmp = ht_iterator(table);
    for(int i = 0; i < table->length; i++) {
        if(ht_next(&tmp)) {
            printf("KEY: %s, VAL: %d\n", tmp.key, *(int*)((Token*)tmp.value)->data);
        }
        
    }
}

extern void printAST(ASTNode*);

//* TODO: Add support for INCLUDES
//* TODO: Add support so file path doesn't have to be the first argument
//* TODO: Fix column not being accurate for some tokens like literals
//* TODO: Make sure negative numbers are working
//* TODO: Add support for ampersand in parser

int main(int argc, char** agrv) {

    //! Temporary setup
    flexerCfg.debug_mode = true;
    flexerCfg.verbose_mode = true;
    flexerCfg.no_preprocess = true;
    flexerCfg.dump_tokens = true;
    flexerCfg.dump_ast = true;

    TokenType keyToks[] = {
        TOK_AUTO, TOK_BREAK, TOK_CASE, TOK_CHAR, TOK_CONST, TOK_CONTINUE, TOK_DEFAULT, TOK_DO,
        TOK_DOUBLE, TOK_ELSE, TOK_ENUM, TOK_EXTERN, TOK_FLOAT, TOK_FOR, TOK_GOTO, TOK_IF,
        TOK_INLINE, TOK_INT, TOK_LONG, TOK_REGISTER, TOK_RETURN, TOK_SHORT, TOK_SIGNED, TOK_SIZEOF,
        TOK_STATIC, TOK_STRUCT, TOK_SWITCH, TOK_TYPEDEF, TOK_UNION, TOK_UNSIGNED, TOK_VOID, TOK_VOLATILE, TOK_WHILE,
        TOK_DEFINE, TOK_INCLUDE, TOK_IFDEF, TOK_ELIF, TOK_ENDIF
    };

    if (argc <= 1) 
        EXIT_FAIL_MSG("PROVIDE FILE PATH...");
    
    for(int i = 1; i < argc; i++) {
        if (strcmp(agrv[i], "--no-preprocess") == 0)
            flexerCfg.no_preprocess = true;
        else if(strcmp(agrv[i], "--dump-tokens") == 0)
            flexerCfg.dump_tokens = true;
        else if(strcmp(agrv[i], "--verbose") == 0)
            flexerCfg.verbose_mode = true;
        else if(strcmp(agrv[i], "--debug") == 0)
            flexerCfg.debug_mode = true;
        else if(strcmp(agrv[i], "--dump-ast") == 0)
            flexerCfg.dump_ast = true;
    }

    if (flexerCfg.verbose_mode) {
        printf("Arguments/Flags:\n");
        for(int i = 1; i < argc; i++) {
            printf("-->\t%s\n", agrv[i]);
        }
    }

    if (flexerCfg.verbose_mode)
        printf("Lexing...\n");


    // get pointer to file data 
    const char* source = getFileSourcePointer(agrv[1]);

    // Setup keyword hash table
    keywordTokenConverter = ht_create((int)__TOK__DIVIDER__OPERATORS__);
    for(int i = 0; i < __TOK__DIVIDER__OPERATORS__; i++)
        ht_set(keywordTokenConverter, TokenStrings[i], (void*)&keyToks[i]);

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

    if (flexerCfg.dump_tokens) {
        if (flexerCfg.verbose_mode) {
            printTokens();
        } else
            printTokens();
    
        printf("\n-------------------\n\n");
    }

     if (!hashPreprocessorDirectives(preProcessHt))
         EXIT_FAIL_MSG("PREPROCESS ERROR...");

     if (!preProcessorMacroExpansion(preProcessHt))
         EXIT_FAIL_MSG("PREPROCESS EXPANSION ERROR...");

    printTokens();

    if (flexerCfg.verbose_mode)
        printf("Parsing...\n");

    ASTNode* ast = parse(SrcTokenStream.Tokens);

    if (flexerCfg.dump_ast)
        printAST(ast);

    //* TODO: Type checking

    //* TODO: Code generation / Evaluation / VM

    //* TODO: Free structures

    return 0;
}