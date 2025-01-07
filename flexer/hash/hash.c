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

    // Keep track of the old entries
    ht_entry* old_entries = table->entries;
    size_t old_capacity = table->capacity;

    // Update table capacity and entries
    table->entries = new_entries;
    table->capacity = new_capacity;
    table->length = 0;

    // Rehash and insert all old entries into the new table
    for (size_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].key != NULL) {
            ht_set(table, old_entries[i].key, old_entries[i].value);
        }
    }

    // Free old entries array
    free(old_entries);
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

hti ht_iterator(ht* table) {
    hti it;
    it._table = table;
    it._index = 0;
    return it;
}

bool ht_next(hti* it) {
    // Loop till we've hit end of entries array.
    ht* table = it->_table;
    while (it->_index < table->capacity) {
        size_t i = it->_index;
        it->_index++;
        if (table->entries[i].key != NULL) {
            // Found next non-empty item, update iterator key and value.
            ht_entry entry = table->entries[i];
            it->key = entry.key;
            it->value = entry.value;
            return true;
        }
    }
    return false;
}
