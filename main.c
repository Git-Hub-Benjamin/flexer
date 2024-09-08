#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <math.h>


enum tokenType {
    NONE = 0,
    SUB,
    ADD, 
    BAND,
    LAND,
    BOR,
    LOR,
    LSH,
    RSH,
    DEC, // decrement 1
    INC,
    LTHEN,
    LTHENEQ,
    GTHEN,
    GTHENEQ,
    NEG,
    LCURLY,
    RCURLY,
    LBRAC,
    RBRAC,
    LPARA,
    RPARA,
    MOD,
    COLON,
    SEMI,
    NOTEQUAL,
    ASSIGN,
    EQUAL,
    ADDEQ,
    LSHEQ,
    RSHEQ,
    XOREQ,
    BOREQ,
    BANDEQ,
    SUBEQ,
    DIVEQ,
    MULEQ,
    MODEQ,
    BCOMP, // 1s compliment "~"
    XOR,
    DIV,
    MUL,
    COMMA, 
    IMM,
    IF,
    WHILE,
    ELSE,
    CHAR,
    RETURN,
    MAIN,
    VOID,
    INT,
    IDENTIFER,
    LABEL,
    LITERAL
};

void printTokenType(enum tokenType type) {
    switch (type) {
        case NONE:         printf("NONE"); break;
        case SUB:          printf("SUB"); break;
        case ADD:          printf("ADD"); break;
        case BAND:         printf("BAND"); break;
        case LAND:         printf("LAND"); break;
        case BOR:          printf("BOR"); break;
        case LOR:          printf("LOR"); break;
        case LSH:          printf("LSH"); break;
        case RSH:          printf("RSH"); break;
        case DEC:          printf("DEC"); break;
        case INC:          printf("INC"); break;
        case LTHEN:        printf("LTHEN"); break;
        case LTHENEQ:      printf("LTHENEQ"); break;
        case GTHEN:        printf("GTHEN"); break;
        case GTHENEQ:      printf("GTHENEQ"); break;
        case NEG:          printf("NEG"); break;
        case LCURLY:       printf("LCURLY"); break;
        case RCURLY:       printf("RCURLY"); break;
        case LBRAC:        printf("LBRAC"); break;
        case RBRAC:        printf("RBRAC"); break;
        case LPARA:        printf("LPARA"); break;
        case RPARA:        printf("RPARA"); break;
        case MOD:          printf("MOD"); break;
        case COLON:        printf("COLON"); break;
        case SEMI:         printf("SEMI"); break;
        case NOTEQUAL:     printf("NOTEQUAL"); break;
        case ASSIGN:       printf("ASSIGN"); break;
        case EQUAL:        printf("EQUAL"); break;
        case ADDEQ:        printf("ADDEQ"); break;
        case LSHEQ:        printf("LSHEQ"); break;
        case RSHEQ:        printf("RSHEQ"); break;
        case XOREQ:        printf("XOREQ"); break;
        case BOREQ:        printf("BOREQ"); break;
        case BANDEQ:      printf("BANDEQ"); break;
        case SUBEQ:        printf("SUBEQ"); break;
        case DIVEQ:        printf("DIVEQ"); break;
        case MULEQ:        printf("MULEQ"); break;
        case MODEQ:        printf("MODEQ"); break;
        case BCOMP:        printf("BCOMP"); break;
        case XOR:          printf("XOR"); break;
        case DIV:          printf("DIV"); break;
        case MUL:          printf("MUL"); break;
        case COMMA:        printf("COMMA"); break;
        case IMM:          printf("IMM"); break;
        case IF:           printf("IF"); break;
        case WHILE:        printf("WHILE"); break;
        case ELSE:         printf("ELSE"); break;
        case CHAR:         printf("CHAR"); break;
        case RETURN:       printf("RETURN"); break;
        case MAIN:         printf("MAIN"); break;
        case VOID:         printf("VOID"); break;
        case INT:          printf("INT"); break;
        case IDENTIFER:    printf("IDENTIFER"); break;
        case LABEL:        printf("LABEL"); break;
        case LITERAL:      printf("LITERAL"); break;
        default:           printf("UNKNOWN"); break;
    }
}

struct token {
    int line;
    int col;
    enum tokenType token;
    union {
        int self_basic_token_num; // just for readability (if this token has no attribute then the token number will be itself)
        int other_attr_token_num; // if it has an attribute, the token num here will be the attribute token num and will be stored here
    } token_num;
};

struct attr_token {
    int other_basic_token_num;
    void* data;
    enum tokenType token;
};

struct string_token {
    char* data;
    int len;
};

int basic_token_count = 0;  
int attr_token_count = 0; 
struct token tokens[256] = {0};
struct attr_token attr_tokens[256] = {0};

/*

Times to start new token

(assuming not in quotes/literal)

- space (" ")
- &&, &, |, !, !=, ==, etc. 
- {([])} any of these
- : (colon)

(assuming in quotes/literal)

- " (that does not have a "\" before it)

*/

enum numType {HEX = 100, BIN, OCT, DECI};

void keywordIdToken(char** line_ptr) {
    int len = 0;
    char* tmp = *line_ptr;
    do { // get length
        len++;
        (*line_ptr)++;
    } while ((**line_ptr >= 'A' && **line_ptr <= 'Z') || (**line_ptr >= 'a' && **line_ptr <= 'z') || **line_ptr == '_');
    attr_tokens[attr_token_count].data = (struct string_token*)malloc(sizeof(struct string_token));
    (*(struct string_token*)attr_tokens[attr_token_count].data).len = len;
    (*(struct string_token*)attr_tokens[attr_token_count].data).data = malloc(len + 1); // + 1 for null char
    for (int i = 0; i < len; i++) { // copy keyword/id into data
        (*(struct string_token*)attr_tokens[attr_token_count].data).data[i] = *tmp;
        tmp++;
    }
    (*(struct string_token*)attr_tokens[attr_token_count].data).data[len] = '\0'; // null terminating

    tmp = (*(struct string_token*)attr_tokens[attr_token_count].data).data;
    bool keyword_found = true;
    if (memcmp(tmp, "if", 2) == 0) {
        tokens[basic_token_count].token = IF;
    } else if (memcmp(tmp, "while", 5) == 0) {
        tokens[basic_token_count].token = WHILE;
    } else if (memcmp(tmp, "else", 4) == 0) {
        tokens[basic_token_count].token = ELSE;
    } else if (memcmp(tmp, "int", 3) == 0) {
        tokens[basic_token_count].token = INT;
    } else if (memcmp(tmp, "main", 4) == 0) {
        tokens[basic_token_count].token = MAIN;
    } else if (memcmp(tmp, "void", 4) == 0) {
        tokens[basic_token_count].token = VOID;
    } else if (memcmp(tmp, "return", 6) == 0) {
        tokens[basic_token_count].token = RETURN;
    } else if (memcmp(tmp, "char", 4) == 0) {
        tokens[basic_token_count].token = CHAR;
    } else 
        keyword_found = false;
        
    if (!keyword_found)
        tokens[basic_token_count].token = IDENTIFER;

    attr_tokens[attr_token_count].token = tokens[basic_token_count].token;
}


// does not support (FLOATS, DOUBLE, NEGATIVES)
void immToken(char** line_ptr) {
    enum numType type;
    if (**line_ptr == '0')
        if (*((*line_ptr)+1) == 'x' || *((*line_ptr)+1) == 'x')
            type = HEX;
        else if (*((*line_ptr)+1) == 'b' || *((*line_ptr)+1) == 'B')
            type = BIN;
        else
            type = OCT;
    else
        type = DECI;
    switch(type){
        case HEX:
        case BIN:
            (*line_ptr)++;
        case OCT:
            (*line_ptr)++;
        default:
            break;
    }
    char* beggining = *line_ptr;
    // get immediate values (0xff, 0b0101, 100, 012) (hex, bin, dec, oct)
    
    if (type < HEX)
        while(**line_ptr >= '0' && **line_ptr <= '9') { 
            (*line_ptr)++;
        }
    else {
        while((**line_ptr >= '0' && **line_ptr <= '9') || (**line_ptr >= 'A' && **line_ptr <= 'F') || (**line_ptr >= 'a' && **line_ptr <= 'f')) { 
            (*line_ptr)++;
        }
        if (beggining < *line_ptr)
            (*line_ptr)--;
    }

    int num = 0;
    int digit_count = 0;

    while(*line_ptr >= beggining) {
        switch (type) {
            case DECI:
                num += (**line_ptr - 48) * (digit_count + 1);
                break;
            case HEX:
                if ((**line_ptr >= 'A' && **line_ptr <= 'F') || (**line_ptr >= 'a' && **line_ptr <= 'f'))
                    num += (**line_ptr - 87) * pow(16, digit_count);
                else
                    num += (**line_ptr - 48) * pow(16, digit_count);
                break;
            case BIN:
                if (**line_ptr == '1')
                    num += pow(2, digit_count);
                break;
            case OCT:
                break;

        }
        digit_count++;
        (*line_ptr)--;
    }



    *line_ptr += digit_count + 1; // I KNOW +1 FOR HEX, CHECK FOR OTHERS

    // store the result in a table that is associated witht he imm token (token count that is stored)
    tokens[basic_token_count].token = IMM;
    attr_tokens[attr_token_count].token = IMM;
    attr_tokens[attr_token_count].data = malloc(sizeof(int));
    *(int*)(attr_tokens[attr_token_count].data) = num;
}

enum token_errors {
    none,
    no_empty_string_literals,
    no_multline_string_literals,
    no_never_ending_string_literals
};

enum token_errors literalToken(char** line_ptr) {
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
    attr_tokens[attr_token_count].data = (struct string_token*)malloc(sizeof(struct string_token));
    struct string_token* temp = (struct string_token*)attr_tokens[attr_token_count].data;
    temp->len = len;
    temp->data = malloc(len + 1); // + 1 for null char
    memcpy(tmp, temp->data, len);
    attr_tokens[attr_token_count].token = LITERAL;
    tokens[basic_token_count].token = LITERAL;
    (*(struct string_token*)attr_tokens[attr_token_count].data).data[len] = '\0'; // null terminating
    (*line_ptr)++; // get off ending quote
}

enum token_errors tokenize(FILE* file) {
    char line_buffer[256];
    int line_number = 0;
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) { // get line
        char* line_ptr = line_buffer; 
        int column = 0;
        while (*line_ptr != '\0') { // iterate over line
            char line_char = *line_ptr;            
            if (line_char == ' ')
                goto next_character;
            else if (line_char == '{')
                tokens[basic_token_count].token = LCURLY;
            else if (line_char == '}')
                tokens[basic_token_count].token = RCURLY;
            else if (line_char == '[')
                tokens[basic_token_count].token = LBRAC;
            else if (line_char == ']')
                tokens[basic_token_count].token = RBRAC;
            else if (line_char == '(')
                tokens[basic_token_count].token = LPARA;
            else if (line_char == ')')
                tokens[basic_token_count].token = RPARA;
            else if (line_char == '~')
                tokens[basic_token_count].token = BCOMP;
            else if (line_char == '^')
                if (*(line_ptr+1) == '=')
                    tokens[basic_token_count].token = XOREQ;
                else
                    tokens[basic_token_count].token = XOR;
            else if (line_char == '%')
                if (*(line_ptr+1) == '=')
                    tokens[basic_token_count].token = MODEQ;
                else 
                    tokens[basic_token_count].token = MOD;
            else if (line_char == ':')
                tokens[basic_token_count].token = COLON;
            else if (line_char == ';')
                tokens[basic_token_count].token = SEMI;
            else if (line_char == ',')
                tokens[basic_token_count].token = COMMA;
            else if (line_char == '&')
                if (*(line_ptr+1) == '&')
                    if(*(line_ptr+2) == '=')
                        tokens[basic_token_count].token = BANDEQ;
                    else
                        tokens[basic_token_count].token = BAND;
                else
                    tokens[basic_token_count].token = LAND;
            else if (line_char == '>') 
                if (*(line_ptr+1) == '>') 
                    if (*(line_ptr+2) == '=') // can safely do 2 ahead because if +1 was '>' then it means '\0' is somewhere else
                        tokens[basic_token_count].token = RSHEQ;
                    else
                        tokens[basic_token_count].token = RSH;
                else if (*(line_ptr+1) == '=') 
                    tokens[basic_token_count].token = GTHENEQ;
                else
                    tokens[basic_token_count].token = GTHEN;
            else if (line_char == '!')
                if (*(line_ptr+1) == '=')
                    tokens[basic_token_count].token = NOTEQUAL;
                else
                    tokens[basic_token_count].token = NEG;
            else if (line_char == '<') 
                if (*(line_ptr+1) == '<') 
                    if (*(line_ptr+2) == '=')
                        tokens[basic_token_count].token = LSHEQ;
                    else
                        tokens[basic_token_count].token = LSH;
                else if (*(line_ptr+1) == '=') 
                    tokens[basic_token_count].token = LTHENEQ;
                else
                    tokens[basic_token_count].token = LTHEN;
            else if (line_char == '=')
                if (*(line_ptr+1) == '=')
                    tokens[basic_token_count].token = EQUAL;
                else
                    tokens[basic_token_count].token = ASSIGN;
            else if (line_char == '|')
                if (*(line_ptr+1) == '|')
                    tokens[basic_token_count].token = LOR;
                else if(*(line_ptr+1) == '=')
                    tokens[basic_token_count].token = BOREQ;
                else
                    tokens[basic_token_count].token = BOR;
            else if (line_char == '+') 
                if (*(line_ptr+1) == '+') 
                    tokens[basic_token_count].token = INC;
                else if (*(line_ptr+1) == '=') 
                    tokens[basic_token_count].token = ADDEQ;
                else
                    tokens[basic_token_count].token = ADD;
            else if (line_char == '-') 
                if (*(line_ptr+1) == '-') 
                    tokens[basic_token_count].token = DEC;
                else if (*(line_ptr+1) == '=') 
                    tokens[basic_token_count].token = SUBEQ;
                else
                    tokens[basic_token_count].token = SUB;
            else if (line_char == '/')
                if(*(line_ptr+1) == '/') 
                    break;
                else if(*(line_ptr+1) == '=') 
                    tokens[basic_token_count].token = DIVEQ;
                else 
                    tokens[basic_token_count].token = DIV;
            else if (line_char == '*') // could be pointer 
                if (*(line_ptr+1) == '=') 
                    tokens[basic_token_count].token = MULEQ;
                else
                    tokens[basic_token_count].token = MUL;
            else if (line_char >= '0' && line_char <= '9') {
                immToken(&line_ptr); // this does NOT support negative
            } else if (line_char == '"') {
                return literalToken(&line_ptr);
            } else if ((*line_ptr >= 'A' && *line_ptr <= 'Z') || (*line_ptr >= 'a' && *line_ptr <= 'z') || *line_ptr == '_'){    
                keywordIdToken(&line_ptr);
            } else {
                goto next_character;
            }
            
            // for every token regardless of if needed attribute
            tokens[basic_token_count].col = column + 1;
            tokens[basic_token_count].line = line_number + 1;

            // token has already been set , for attribute token too

            // set token num for basic tokens & attr tokens
            if (tokens[basic_token_count].token >= IMM) {
                tokens[basic_token_count].token_num.other_attr_token_num = attr_token_count;
                attr_tokens[attr_token_count].other_basic_token_num = basic_token_count;
                column += line_ptr - (line_buffer + column);
                basic_token_count++;
                attr_token_count++;
                continue;
            } else 
                tokens[basic_token_count].token_num.self_basic_token_num = basic_token_count;

            switch(tokens[basic_token_count].token) {
                case LSHEQ:
                case RSHEQ:
                    line_ptr++;
                    column++;
                case XOREQ:
                case MODEQ:
                case BOREQ:
                case BANDEQ:
                case BAND:
                case RSH:
                case GTHENEQ:
                case LSH:
                case LTHENEQ:
                case EQUAL:
                case LOR:
                case INC:
                case ADDEQ:
                case DEC:
                case SUBEQ:
                case DIVEQ:
                case MULEQ:
                    line_ptr++;
                    column++;
                default:
                    break;
            }

            // increment regardless
            basic_token_count++;
next_character:
            line_ptr++;
            column++;
        }
    line_number++;
    }
    return none;
}

int main(int argc, char** agrv) {
    
    // Open file for reading
    char* program_filepath = agrv[1];
    FILE* file = fopen(program_filepath, "r");
    if (file == NULL) {
        perror("Error opening file for reading");
        return 1;
    }

    if (!tokenize(file)) {
        printf("Syntax error.\n");
        return 1;
    }

    for (int i = 0; i < basic_token_count; i++) {
        printf("Line: %d, Col: %d, Token: ", tokens[i].line, tokens[i].col);
        printTokenType(tokens[i].token);
        if (tokens[i].token >= IMM)
            if (tokens[i].token == IMM)
                printf(", Data: %d", (*(int*)(attr_tokens[tokens[i].token_num.other_attr_token_num].data)));
            else
                printf(", Data: %s", (*(struct string_token*)(attr_tokens[tokens[i].token_num.other_attr_token_num].data)).data);
        printf(", Token num: %d\n", i);
    }

    printf("File path: %s\n", program_filepath);
    return 0;
}