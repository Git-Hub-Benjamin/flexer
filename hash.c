#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL
#define INITAL_CAPACITY 16

typedef struct {
    const char* key;
    void* value;
} ht_entry;

typedef struct {
    ht_entry* entries;
    size_t capacity;
    size_t length;
} ht;

ht* ht_create(void) {
    ht* table = malloc(sizeof(ht));
    if (!table)
        return NULL;    
    table->length = 0;
    table->capacity = INITAL_CAPACITY;
    table->entries = calloc(table->capacity, sizeof(ht_entry));
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

const char* 

int main(){
    int res = 16 % INITAL_CAPACITY;
    printf("Result: %d\n", res);

}
