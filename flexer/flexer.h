#ifndef FLEXER_H
#define FLEXER_H

typedef struct {
    bool debug_mode;
    bool verbose_mode;
    bool no_preprocess;
    bool dump_tokens;
    char* output_file;
    char* input_file;
} Config;

#endif