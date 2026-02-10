#ifndef HASH_TABLE_H
#define HASH_TABLE_H
#include <stdlib.h>

typedef enum {
    EMPTY,
    OCCUPIED,
    DELETED
} Slot_State;


typedef struct {
    char *key;
    void *value;
    Slot_State state;
} Entry;

typedef struct {
    size_t size;
    size_t count;
    Entry *entries;
} Hash_Table;


Hash_Table *create_table(size_t size);
void insert_into_table(Hash_Table *table, const char *key, void *value) ;
void *get_from_table_or_null(Hash_Table *table, const char *key) ;
void remove_from_table(Hash_Table *table, const char *key) ;
void destroy_table(Hash_Table *table) ;
#endif // HASH_TABLE_H
