#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>


enum tokenType {
    NONE,
    PUSH,
    POP,
    READ,
    SUB,
    ADD,
    JUMP_EQ_0,
    JUMP_GR_0,
    JUMP_LS_0,
    PRINT,
    LABEL,
    LITERAL,
    BAND,
    LAND,
    BOR,
    LOR,
    LSH,
    RSH,
};

struct token {
    int line;
    enum tokenType token;
    int token_num; // used for branches, labels, literals
};

int token_count = 0;
struct token tokens[256];

enum tokenType validate_token(char* token_string, int token_len){
    if (memcmp("PUSH", token_string, token_len))
        return PUSH;
    if (memcmp("POP", token_string, token_len))
        return POP;
    if (memcmp("READ", token_string, token_len))
        return READ;
    return NONE;
}



// this might be easier if you break this up, instead of trying to tokenize while 
// making sure that syntax is right, dont care if the syntax is messed up, just tokenize
// then in another step you will see if the syntax is right, when there is a space,
// it means it is a new token, when there is a new line then new token

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


// 0 - good
// 1 - error
bool tokenize(FILE* file) {
    char line_buffer[256];
    int line_number = 0;
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) { // get line
        char* line_ptr = line_buffer; 
        while (*line_ptr != '\0') { // iterate over line
            char line_char = *line_ptr;
            bool token_found = true;
            
            if (line_char == ' ')
                goto next_line;
            switch(line_char) {
            if (line_char == '{')
            if (line_char == '}')
            if (line_char == '[')
            if (line_char == ']')
            if (line_char == '(')
            if (line_char == ')')
            if (line_char == '%')
            if (line_char == ':')

            if (line_char == '&')
                if (*(line_ptr+1) == '&')
                    tokens[token_count].token = BAND;
                else
                    tokens[token_count].token = LAND;

            if (line_char == '>') // greater than
                if (*(line_ptr+1) == '>') // right shift
                    tokens[token_count].token = BAND;
                else if (*(line_ptr+1) == '=') // greater than or equal
                    tokens[token_count].token = LAND;
                else
                    tokens[token_count].token = 

            if (line_char == '!')
            if (line_char == '<')
            if (line_char == '=')
            if (line_char == '|')
                default:
                    goto next_line;
            }

            tokens[token_count].line = line_number;
            tokens[token_count].token_num = token_count;
            token_count++;
next_line:
        }
    }
    printf("\n");
}

int main(int argc, char** agrv) {
    
    // Open file for reading
    char* program_filepath = agrv[1];
    FILE* file = fopen(program_filepath, "r");
    if (file == NULL) {
        perror("Error opening file for reading");
        return 1;
    }

    tokenize(file);

    printf("File path: %s\n", program_filepath);
    return 0;
}