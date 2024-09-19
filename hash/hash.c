#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include "hash.h"



#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL
#define INITAL_CAPACITY 2

ht* ht_create(int INITAL) {
    ht* table = (ht*)malloc(sizeof(ht));
    if (!table)
        return NULL;    
    table->length = 0;
    table->capacity = INITAL <= 0 ? INITAL_CAPACITY : INITAL;
    table->entries = (ht_entry*)calloc(table->capacity, sizeof(ht_entry));
    if (!table->entries) {
        free(table);
        return NULL;
    }
    return table;
}

void ht_destroy(ht* table) {
    for(size_t i = 0; i < table->capacity; i++) 
        free((void*)table->entries[i].key);
    free(table->entries);
    free(table);
}


static uint64_t hash_key(const char* key) {
    uint64_t hash = FNV_OFFSET;
    for (; *key != '\0'; key++) {
        hash ^= (uint64_t)(unsigned char)(*key);
        hash *= FNV_PRIME;
    }
    return hash;
}

void* ht_get(ht* table, const char* key) {
    size_t index = hash_key(key) % table->capacity;
    while (table->entries[index].key != NULL) {
        if (strcmp(key, table->entries[index].key) == 0) 
            return table->entries[index].value;
        index++;
        if (index >= table->capacity)
            index = 0;
    }
    return NULL;
}

static const char* ht_set_entry(ht* table, const char* key, void* value, size_t* plength) {
    ht_entry* entries = table->entries;
    size_t index = hash_key(key) % table->capacity;
    
    // find empty entry
    while (entries[index].key != NULL) {
        if (strcmp(key, entries[index].key) == 0) {
            entries[index].value = value;
            return key;
        }

        // key was not here , linear prob
        index++;
        if (index >= table->capacity)
            index = 0;
    }

    if (plength != NULL) {
        key = (key);
        if (key == NULL) {
            return NULL;
        }
        (*plength)++;
    }

    entries[index].key = (char*)key;
    entries[index].value = value;
    return key;
}

static bool ht_expand(ht* table) {
    // Allocate new entries array.
    size_t new_capacity = table->capacity * 2;
    if (new_capacity < table->capacity) {
        return false;  // overflow (capacity would be too big)
    }
    ht_entry* new_entries = (ht_entry*)calloc(new_capacity, sizeof(ht_entry));
    if (new_entries == NULL) {
        return false;
    }

    // Iterate entries, move all non-empty ones to new table's entries.
    for (size_t i = 0; i < table->capacity; i++) {
        ht_entry entry = table->entries[i];
        if (entry.key != NULL) {
            ht_set_entry(table, entry.key,
                         entry.value, NULL);
        }
    }

    // Free old entries array and update this table's details.
    free(table->entries);
    table->entries = new_entries;
    table->capacity = new_capacity;
    return true;
}

const char* ht_set(ht* table, const char* key, void* value) {
    assert(value != NULL);
    if (value == NULL) {
        return NULL;
    }

    if (table->length >= table->capacity / 2)
        if (!ht_expand(table)) 
            return NULL;

    return ht_set_entry(table, key, value, &table->length);
}

// int main(){
//     char* abc = "abc";
//     int abc_v = 100;

//     char* def = "def";
//     int def_v = 200;

//     char* strings[] = {
//         // Keywords
//         "auto", "break", "case", "char", "const", "continue", "default", "do",
//         "double", "else", "enum", "extern", "float", "for", "goto", "if",
//         "inline", "int", "long", "register", "return", "short", "signed", "sizeof",
//         "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
        
//         // Operators
//         "+", "-", "*", "/", "%", "++", "--", "==", "!=", ">", "<", ">=", "<=",
//         "&&", "||", "!", "&", "|", "^", "~", "<<", ">>", "=", "+=", "-=", "*=",
//         "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
        
//         // Punctuation
//         "{", "}", "[", "]", "(", ")", ";", ",", ".", "->", ":", "#", "#include", "#define",
        
//         // Additional common tokens
//         "main", "printf", "scanf", "return", "int", "float", "double", "char", "void"
//     };

//     size_t num_strings = sizeof(strings) / sizeof(strings[0]);
//     printf("Count: %ld, Strings[0]: %lu\n", num_strings, sizeof(strings));

//     int collision_track[128] = {0};
//     int collision_count = 0;

//     for (int j = 0; j < num_strings; j++) {
//         uint64_t hash = FNV_OFFSET;
//         for(char* tmp = strings[j]; *tmp != '\0'; tmp++) {
//             hash ^= (uint64_t)(unsigned char)(*tmp);
//             hash *= FNV_PRIME;
//         }
//         size_t index = hash % 128;
//         printf("String: %s, Hash: %" PRIu64 ", Index: %ld", strings[j], hash, index);        
//         if (collision_track[index] == 0) 
//             collision_track[index] = 1;
//         else {
//             printf(" -------> collision at %ld", index);        
//             collision_count++;
//         }
//         printf("\n");
//     }

//     printf("Total collisions: %d\n", collision_count);

//     ht* my_ht = ht_create(-1);
//     uint64_t hash = hash_key(abc);
//     size_t index = hash % my_ht->capacity;

//     return 0;


//     if (ht_set(my_ht, abc, (void*)&abc_v) == NULL) {
//         exit(EXIT_FAILURE);
//     }

//     if (ht_get(my_ht, abc) == NULL) {
//         printf("Could not find hash value...\n");
//     } else {
//         printf("Value stored here: %d\n", abc_v);
//     }

//     printf("Table length: %ld\n", my_ht->length);

//     if (ht_set(my_ht, def, (void*)&def_v) == NULL) {
//         exit(EXIT_FAILURE);
//     }

//     if (ht_get(my_ht, def) == NULL) {
//         printf("Could not find hash value...\n");
//     } else {
//         printf("Value stored here: %d\n", def_v);
//     }

//     printf("Table length: %ld\n", my_ht->length);

//     return 0;
// }
