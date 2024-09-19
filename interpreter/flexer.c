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
    
    "_TOK_DIVIDER_",

    // Operators
    "+", "-", "*", "/", "%", "++", "--", "==", "!=", ">", "<", ">=", "<=",
    "&&", "||", "!", "&", "|", "^", "~", "<<", ">>", "=", "+=", "-=", "*=",
    "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",

    "_TOK_DIVIDER_",
    
    // Punctuation
    "{", "}", "[", "]", "(", ")", ";", ",", ".", "->", ":", "#", "#include", "#define",
    
    // Additional common tokens
    "main"
}; // 63

ht* keywordTokenConverter;
TokenStream SrcTokenStream; // Source file token stream
static Token* tokens; // tmp pointer to the same address as (SrcTokenStream.tokens)
static AttrToken* attr_tokens; // tmp pointer to the same address as (SrcTokenStream.AttrTokens)

// make new stream 2 times the size of the last, copy data and free old memory 
static bool ts_expand(void* stream, size_t* capacity) {
    size_t new_capacity = *capacity * 2;
    if (new_capacity < *capacity)
        return false; // overflow
    
    // 0 out new memory
    void* new_stream = calloc(new_capacity, (stream == SrcTokenStream.Tokens) ? sizeof(Token) : sizeof(AttrToken));
    if (new_stream == NULL)
        return false;
    
    // copy old data to new pointer
    memcpy(stream, new_stream,(stream == SrcTokenStream.Tokens) ? sizeof(Token) : sizeof(AttrToken));

    // update tmp pointer for lexer to new pointer
    if (stream == SrcTokenStream.Tokens)
        tokens = SrcTokenStream.Tokens;
    else
        attr_tokens = SrcTokenStream.AttrTokens;

    free(stream);
    *capacity = new_capacity;
    return true; // successful
}

// see if we need to expand the stream
enum ts_type {BASIC, ATTR};
static void ts_new_index(enum ts_type type) {
    if (type == BASIC) {
        if(SrcTokenStream.basicTokenIndex >= SrcTokenStream.basicTokenCapacity && !ts_expand(tokens, &SrcTokenStream.basicTokenCapacity))
            exit(EXIT_FAILURE);
        SrcTokenStream.basicTokenIndex++;
    } else if (type == ATTR) {
        if(SrcTokenStream.attrTokenIndex >= SrcTokenStream.attrTokenCapacity && !ts_expand(attr_tokens, &SrcTokenStream.attrTokenCapacity))
            exit(EXIT_FAILURE);
        SrcTokenStream.attrTokenIndex++;
    }    
}

static char* keywordIdToken(char* line_ptr) {

    // get pointer to end of string
    char* tmp = line_ptr;
    do {
        line_ptr++; 
    } while ((*line_ptr >= 'A' && *line_ptr <= 'Z') || (*line_ptr >= 'a' && *line_ptr <= 'z') || *line_ptr == '_');

    // save the char here
    char saved = *line_ptr;
    // null terminate temporarily
    *line_ptr = '\0';

    // search for keyword
    void* tokdata = ht_get(keywordTokenConverter, tmp);
    if (tokdata != NULL) { // found keyword 
        tokens[SrcTokenStream.basicTokenIndex].token = *(TokenType*)tokdata;
    } else { // copy the identifer into attr token
        tokens[SrcTokenStream.basicTokenIndex].token = TOK_IDENTIFER; // set basic token
        attr_tokens[SrcTokenStream.attrTokenIndex].data = malloc(sizeof(StringToken)); // malloc memory
        if (attr_tokens[SrcTokenStream.attrTokenIndex].data == NULL) {
            printf("No memory...\n");
            exit(EXIT_FAILURE);
        }
        (*(StringToken*)(attr_tokens[SrcTokenStream.attrTokenIndex].data)).data = (char*)malloc(line_ptr - tmp + 1); // malloc size of string
        if ((*(StringToken*)(attr_tokens[SrcTokenStream.attrTokenIndex].data)).data == NULL) {
            printf("No memory...\n");
            exit(EXIT_FAILURE);
        }
        strcpy((*(StringToken*)(attr_tokens[SrcTokenStream.attrTokenIndex].data)).data, tmp); // copy 
        (*(StringToken*)(attr_tokens[SrcTokenStream.attrTokenIndex].data)).len = line_ptr - tmp; // set length
        attr_tokens[SrcTokenStream.attrTokenIndex].token = TOK_IDENTIFER; // set attr token too
    }

    // restore saved char
    *line_ptr = saved;

    // return new pointer position
    return line_ptr;
}


// does not support (FLOATS, DOUBLE)
char* immToken(char* line_ptr) {
    enum immType {HEX, BIN, OCT, DEC} type;
    if (*line_ptr == '0')
        if (*(line_ptr+1) == 'x' || *(line_ptr+1) == 'X')
            type = HEX;
        else if (*(line_ptr+1) == 'b' || *(line_ptr+1) == 'B')
            type = BIN;
        else
            type = OCT;
    else
        type = DEC;
    switch(type){
        case HEX:
        case BIN:
            line_ptr++;
        case OCT:
            line_ptr++;
        default:
            break;
    }

    // get pointer to end of immediate value
    char* beggining = line_ptr;
    if (type == BIN || type == DEC) // BINARY & DECIAML
        while(*line_ptr >= '0' && *line_ptr <= '9') line_ptr++;
    else if (type == HEX) // HEX
        while((*line_ptr >= '0' && *line_ptr <= '9') || (*line_ptr >= 'A' && *line_ptr <= 'F') || (*line_ptr >= 'a' && *line_ptr <= 'f')) line_ptr++;
    else if(type == BIN)
        while(*line_ptr >= '0' && *line_ptr <= '1') line_ptr++;
    else // OCTAL
        while(*line_ptr >= '0' && *line_ptr <= '7') line_ptr++;

    line_ptr--; // move to the last digit

    uint32_t num = 0;
    int digit_count = 0;

    // line_ptr pointing char in num, we are going from back to front
    while(line_ptr >= beggining) {
        if (type == DEC)
            num += (*line_ptr - '0') * (uint32_t)pow(10, digit_count);
        else if(type == HEX)
            if (*line_ptr >= 'A' && *line_ptr <= 'F')
                num += (*line_ptr - 55) * (uint32_t)pow(16, digit_count);
            else if (*line_ptr >= 'a' && *line_ptr <= 'f')
                num += (*line_ptr - 87) * (uint32_t)pow(16, digit_count);
            else
                num += (*line_ptr - 48) * (uint32_t)pow(16, digit_count);
        else if(type == BIN) {
            if (*line_ptr == '1')
                num += (uint32_t)pow(2, digit_count);
        } else 
            num += (*line_ptr - 48) * (uint32_t)pow(8, digit_count);
        digit_count++;
        line_ptr--;
    }

    // get position to after last digit
    line_ptr += digit_count + 1; 

    // store the result in a table that is associated witht he imm token (token count that is stored)
    tokens[SrcTokenStream.basicTokenIndex].token = TOK_IMMEDIATE;
    attr_tokens[SrcTokenStream.attrTokenIndex].token = TOK_IMMEDIATE;
    attr_tokens[SrcTokenStream.attrTokenIndex].data = malloc(sizeof(int));
    *(int*)(attr_tokens[SrcTokenStream.attrTokenIndex].data) = (int)num;

    // return new pos
    return line_ptr; 
}

// enum token_errors {
//     none,
//     no_empty_string_literals,
//     no_multline_string_literals,
//     no_never_ending_string_literals
// };

char* literalToken(char* line_ptr) {
    return NULL;
#ifdef A
    if (*((*line_ptr)+1) == '"')
        return no_empty_string_literals;
    int len = 0;
    *line_ptr++;
    char* tmp = *line_ptr;
    while (**line_ptr != '"') {
        if (**line_ptr == '\0') // check for null in case user never closes quotes
            return no_never_ending_string_literals;                    
        else if (**line_ptr == '\n') 
            return no_multline_string_literals;
        len++;
        (*line_ptr)++;
    } 
    attr_tokens[SrcTokenStream.attrTokenIndex].data = (struct string_token*)malloc(sizeof(StringToken));
    StringToken* temp = (StringToken*)attr_tokens[SrcTokenStream.attrTokenIndex].data;
    temp->len = len;
    temp->data = malloc(len + 1); // + 1 for null char
    memcpy(tmp, temp->data, len);
    attr_tokens[SrcTokenStream.attrTokenIndex].token = TOK_LITERAL;
    tokens[SrcTokenStream.basicTokenIndex].token = TOK_LITERAL;
    (*(StringToken*)attr_tokens[SrcTokenStream.attrTokenIndex].data).data[len] = '\0'; // null terminating
    (*line_ptr)++; // get off ending quote
#endif
}

int tokenize(FILE* file) {
    int line_number = 0;
    int column = 0;
    char line_buffer[256];
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) { // get line
        char* line_ptr = line_buffer; 
        while (*line_ptr != '\0') { // iterate over line
            if (*line_ptr == '\n')
                break;
            switch (*line_ptr) {
                case ' ':
                    goto next_character;
                case '#':
                    tokens[SrcTokenStream.basicTokenIndex].token = TOK_HASH; break;
                case '(':
                    tokens[SrcTokenStream.basicTokenIndex].token = TOK_LPAREN; break;
                case ')':
                    tokens[SrcTokenStream.basicTokenIndex].token = TOK_RPAREN; break;
                case '[':
                    tokens[SrcTokenStream.basicTokenIndex].token = TOK_LBRACKET; break;
                case ']':
                    tokens[SrcTokenStream.basicTokenIndex].token = TOK_RBRACKET; break;
                case '{':
                    tokens[SrcTokenStream.basicTokenIndex].token = TOK_LBRACE; break;
                case '}':
                    tokens[SrcTokenStream.basicTokenIndex].token = TOK_RBRACE; break;
                case ',':
                    tokens[SrcTokenStream.basicTokenIndex].token = TOK_COMMA; break;
                case ':':
                    tokens[SrcTokenStream.basicTokenIndex].token = TOK_COLON; break;
                case ';':
                    tokens[SrcTokenStream.basicTokenIndex].token = TOK_SEMICOLON; break;
                case '~':
                    tokens[SrcTokenStream.basicTokenIndex].token = TOK_TILDE; break;
                case '*':
                    if (*(line_ptr + 1) == '=')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_STAR_EQUAL;
                    else
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_STAR;
                    break;
                case '/':
                    if (*(line_ptr + 1) == '/')
                        break; // Comment, end of line
                    else if (*(line_ptr + 1) == '=')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_SLASH_EQUAL;
                    else
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_SLASH;
                    break;
                case '+':
                    if (*(line_ptr + 1) == '+')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_PLUSPLUS;
                    else if (*(line_ptr + 1) == '=') 
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_PLUS_EQUAL;
                    else
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_PLUS;
                    break;
                case '-':
                    if (*(line_ptr + 1) == '-')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_MINUSMINUS;
                    else if (*(line_ptr + 1) == '=')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_MINUS_EQUAL;
                    else
                        if (*(line_ptr + 1) >= '0' && *(line_ptr + 1) <= '9') {
                            line_ptr++; // advance pass negative sign
                            line_ptr = immToken(line_ptr); // get number
                            *((int*)attr_tokens[SrcTokenStream.attrTokenIndex].data) = *((int*)attr_tokens[SrcTokenStream.attrTokenIndex].data) * -1; // make negative
                        } else
                            tokens[SrcTokenStream.basicTokenIndex].token = TOK_MINUS;
                    break;
                case '=':
                    if (*(line_ptr + 1) == '=') {
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_EQUAL_EQUAL;
                        line_ptr++;
                    } else {
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_EQUAL;
                    }
                    break;
                case '&':
                    if (*(line_ptr + 1) == '&')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_AND_AND;
                    else if (*(line_ptr + 1) == '=')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_AMPERSAND_EQUAL;
                    else
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_AMPERSAND;
                    break;
                case '|':
                    if (*(line_ptr + 1) == '|')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_OR_OR;
                    else if (*(line_ptr + 1) == '=')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_PIPE_EQUAL;
                    else
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_PIPE;
                    break;
                case '^':
                    if (*(line_ptr + 1) == '=')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_CARET_EQUAL;
                    else
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_CARET;
                    break;
                case '%':
                    if (*(line_ptr + 1) == '=')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_PERCENT_EQUAL;
                    else
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_PERCENT;
                    break;
                case '>':
                    if (*(line_ptr + 1) == '>')
                        if (*(line_ptr + 2) == '=')
                            tokens[SrcTokenStream.basicTokenIndex].token = TOK_RSHIFT_EQUAL;
                        else
                            tokens[SrcTokenStream.basicTokenIndex].token = TOK_RSHIFT;
                    else if (*(line_ptr + 1) == '=')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_GREATER_EQUAL;
                    else
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_GREATER;
                    break;
                case '<':
                    if (*(line_ptr + 1) == '<')
                        if (*(line_ptr + 2) == '=')
                            tokens[SrcTokenStream.basicTokenIndex].token = TOK_LSHIFT_EQUAL;
                        else
                            tokens[SrcTokenStream.basicTokenIndex].token = TOK_LSHIFT;
                    else if (*(line_ptr + 1) == '=')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_LESS_EQUAL;
                    else
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_LESS;
                    break;
                case '!':
                    if (*(line_ptr + 1) == '=')
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_BANG_EQUAL;
                    else
                        tokens[SrcTokenStream.basicTokenIndex].token = TOK_BANG;
                    break;
                default:
                    if (*line_ptr >= '0' && *line_ptr <= '9') {
                        line_ptr = immToken(line_ptr); // this does NOT support negative
                    } else if (*line_ptr == '"') {
                        line_ptr = literalToken(line_ptr);
                    } else if ((*line_ptr >= 'A' && *line_ptr <= 'Z') || (*line_ptr >= 'a' && *line_ptr <= 'z') || *line_ptr == '_') {    
                        line_ptr = keywordIdToken(line_ptr);
                    } else {
                        printf("Unknown charcter...\n");
                        exit(EXIT_FAILURE);
                    }
                    break;
            }

            // 1. Token has been set
            
            // 2. Set line and col of token
            tokens[SrcTokenStream.basicTokenIndex].col = column + 1;
            tokens[SrcTokenStream.basicTokenIndex].line = line_number + 1;

            // 3. Set token number for attr tokens and non attr tokens

            // 4. Update column for special cases           (if keyword)                                                (or if ID, LITERAL, IMM)
            if (tokens[SrcTokenStream.basicTokenIndex].token < __TOK__DIVIDER__OPERATORS__ || tokens[SrcTokenStream.basicTokenIndex].token >= TOK_IDENTIFER && tokens[SrcTokenStream.basicTokenIndex].token <= TOK_IMMEDIATE) {
                column += line_ptr - (line_buffer + column) - 1; // (-1) because its going to be incremented automatically 
                line_ptr--; // because its going to be incremented automatically too
            }

            // 5. Set token num for attr tokens
            if (tokens[SrcTokenStream.basicTokenIndex].token >= TOK_IDENTIFER && tokens[SrcTokenStream.basicTokenIndex].token <= TOK_IMMEDIATE) {
                tokens[SrcTokenStream.basicTokenIndex].token_num.other_attr_token_num = SrcTokenStream.attrTokenIndex;
                attr_tokens[SrcTokenStream.attrTokenIndex].other_basic_token_num = SrcTokenStream.basicTokenIndex;
            } else // non attr token numss
                tokens[SrcTokenStream.basicTokenIndex].token_num.self_basic_token_num = SrcTokenStream.basicTokenIndex;

            // 6. Increment line_ptr & column for Operators that need it 
            switch(tokens[SrcTokenStream.basicTokenIndex].token) {
                case TOK_RSHIFT_EQUAL: // 3 character operators
                case TOK_LSHIFT_EQUAL:
                case TOK_CARET_EQUAL:
                    line_ptr++;
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
                    line_ptr++;
                    column++;
                default:
                    break;
            }

            // 6. Increment basic token count regardless but attr only when it makes sense
            if (tokens[SrcTokenStream.basicTokenIndex].token >= TOK_IDENTIFER && tokens[SrcTokenStream.basicTokenIndex].token <= TOK_IMMEDIATE)
                ts_new_index(ATTR);
            ts_new_index(BASIC);
next_character:
            line_ptr++;
            column++;
        }

        if (*line_ptr == '\n') {
            line_number++;
            column = 0; // reset column
        }

        //! if the line has more than 256 characters then the token will be fucked up
        //! you can either increase the buffer size to a larger number or really try to fix it
    }

    // set last ending token to TOT_TOTAL
    tokens[SrcTokenStream.basicTokenIndex].token = TOK_TOTAL;
    return 1; // good
}

/*

void tokenize(FILE* file) {
    int line = 0;
    char buffer[256];
    while(fgets(buffer, sizeof(buffer), file)) {
        int col = 0;
        char* left = buffer;
        while(*left != '\0') {
            char* beggining = left;
            while(*left == ' ')
                left++;
            char* right = left;
            bool ID_KW = false;
            if (*right >= 'A' && *right <= 'Z' || *right >= 'a' && *right <= 'z' || *right == '_') { // identifer or keyword
                do right++; while (*right >= 'A' && *right <= 'Z' || *right >= 'a' && *right <= 'z' || *right == '_'); ID_KW = true;
            } else if (*right == '"') { // literal
                goto store_token;
            } else if (*right >= '0' && *right <= '9') { // immediate
                goto store_token;
            } else { // operator or puncation
                while (!(*right >= 'A' && *right <= 'Z' || *right >= 'a' && *right <= 'z' || *right == '_' || *right >= '0' && *right <= '9' || *right == ' ' || *right == '"' || *right == '\0' || *right == '\n')) 
                    right++;
            }
            char saved = *right;
            *right = '\0';
            TokenType* t_data = ht_get(tokenConverter, left);
            TokenType token = t_data == NULL ? TOK_TOTAL : *t_data;
            if (token == TOK_TOTAL) {
                if (ID_KW) {
                    token = TOK_IDENTIFER;
                    memcpy(attr_tokens[SrcTokenStream.a_index].data, left, right - left);
                    attr_tokens[SrcTokenStream.a_index].other_basic_token_num = SrcTokenStream.basicTokenIndex;
                    attr_tokens[SrcTokenStream.a_index].token = token;
                    ts_new_index(ATTR);
                } else {
                    // unknown token
                }
            }
            *right = saved;
store_token:
            // basic token
            tokens[SrcTokenStream.basicTokenIndex].token = token;
            tokens[SrcTokenStream.basicTokenIndex].line = line;
            tokens[SrcTokenStream.basicTokenIndex].col = col + (left - beggining);
            tokens[SrcTokenStream.basicTokenIndex].token_num.self_basic_token_num = SrcTokenStream.basicTokenIndex;
            ts_new_index(BASIC);

            printf("Token: %d\n", *(int*)token);
next_token:
            left = right;
            col = col + (left - beggining);
        }
    }
    line++;
}

*/

extern bool preProcess();

// [0] path to file, [1], loaded keyword file
int main(int argc, char** agrv) {

    printf("Sizeof: %lu\n", sizeof(SrcTokenStream.Tokens));

    // Open file for reading
    char* program_filepath = agrv[1];
    FILE* file = fopen(program_filepath, "r");
    if (file == NULL) {
        perror("Error opening file for reading");
        return 1;
    }

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
    SrcTokenStream.basicTokenCapacity = TOKEN_STREAM_INITAL_CAPACITY;
    SrcTokenStream.Tokens = (Token*)calloc(TOKEN_STREAM_INITAL_CAPACITY, sizeof(Token));
    if (SrcTokenStream.Tokens == NULL) {
        printf("No memory...\n");
        exit(EXIT_FAILURE);
    }

    // setup token attr stream
    SrcTokenStream.attrTokenCapacity = TOKEN_STREAM_INITAL_CAPACITY;
    SrcTokenStream.AttrTokens = (AttrToken*)calloc(TOKEN_STREAM_INITAL_CAPACITY, sizeof(AttrToken));
    if (SrcTokenStream.AttrTokens == NULL) {
        printf("No memory...\n");
        exit(EXIT_FAILURE);
    }

    tokens = SrcTokenStream.Tokens;
    attr_tokens = SrcTokenStream.AttrTokens;

    if (!tokenize(file)) {
        printf("Syntax error.\n");
        return 1;
    }

    if (!preProcess()) {
        printf("Preprocess error.\n");
        return 1;
    }

    for (int i = 0; i < SrcTokenStream.basicTokenIndex; i++) {
        printf("<line: %d, col: %d, token: ", tokens[i].line, tokens[i].col);
        if ((TokenType)tokens[i].token <= TOK_MAIN)
            printf("%s", TokenStrings[tokens[i].token]);
        else if((TokenType)i == TOK_IDENTIFER)
            printf("identifer");
        else if((TokenType)i == TOK_LITERAL)
            printf("literal");
        else
            printf("immediate");
        if (tokens[i].token >= TOK_IDENTIFER && tokens[i].token <= TOK_IMMEDIATE) {
            if (tokens[i].token == TOK_IMMEDIATE)
                printf(", data: %d", (*(int*)(attr_tokens[tokens[i].token_num.other_attr_token_num].data)));
            else
                printf(", data: %s", (*(StringToken*)(attr_tokens[tokens[i].token_num.other_attr_token_num].data)).data);
        }
        printf(", token_num: %d>\n", i);
    }

    printf("File path: %s\n", program_filepath);
    return 0;
}