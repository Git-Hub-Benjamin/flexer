#include <stdint.h>

typedef struct {
    const char* key;
    void* value;
} ht_entry;

typedef struct {
    ht_entry* entries;
    size_t capacity;
    size_t length;
} ht;

// Create hash table and return pointer to it, or NULL if out of memory.
// Pass inital capacity, if <= 0 is passed default value will be provided
ht* ht_create(int);

// Free memory allocated for hash table, including allocated keys.
void ht_destroy(ht* table);

// Get item with given key (NUL-terminated) from hash table. Return
// value (which was set with ht_set), or NULL if key not found.
void* ht_get(ht* table, const char* key);

// Set item with given key (NUL-terminated) to value (which must not
// be NULL). If not already present in table, key is copied to newly
// allocated memory (keys are freed automatically when ht_destroy is
// called). Return address of copied key, or NULL if out of memory.
const char* ht_set(ht* table, const char* key, void* value);
