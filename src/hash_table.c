
#include "hash_table.h"
#include <stdlib.h>
#include <string.h>



static size_t get_string_hash(const char *s) 
{
    size_t hash = 0;
    while(*s) {
        hash += (unsigned char)*s;
        s++;
    }
    return hash;
}

Hash_Table *create_table(size_t size) 
{
    Hash_Table *table = malloc(sizeof(*table));
    if(!table) return NULL;

    table->size  = size;
    table->count = 0;
    table->entries = calloc(size, sizeof(*table->entries));
    if(!table->entries) {
        free(table);
        return NULL;
    }

    for(size_t i = 0; i < size; i++) {
        table->entries[i].state = EMPTY;
    }

    return table;

}



void insert_into_table(Hash_Table *table, const char *key, void *value) 
{
    size_t hash = get_string_hash(key) % table->size;
    // get the hash of the string 
    int first_deleted = -1;
    
    for(size_t i = 0; i < table->size; i++) {
        size_t index = (hash + i) % table->size;

        Entry *entry = &table->entries[index];

        if(entry->state == OCCUPIED) {
            if(strcmp(entry->key,key) == 0){
                entry->value = value;
                return;
            }
        } else if(entry->state == DELETED){
            if(first_deleted == -1) {
                first_deleted = index;
            }
        } else {
            if(first_deleted != -1) {
                index = first_deleted;
            }

            entry = &table->entries[index];
            entry->key = strdup(key);
            entry->value =value;
            entry->state = OCCUPIED;
            table->count++;
            return;
        }
    }
    if(first_deleted != -1) {
        Entry *entry = &table->entries[first_deleted];
        entry->key = strdup(key);
        entry->value = value;
        entry->state = OCCUPIED;
        table->count++;
    }
}

void *get_from_table_or_null(Hash_Table *table, const char *key) 
{
    size_t hash = get_string_hash(key) % table->size;
    for(size_t i = 0; i < table->size; ++i) {
        size_t index = (hash + i ) % table->size;
        Entry *entry = &table->entries[index];

        if(entry->state == EMPTY) {
            return NULL;
        }

        if((entry->state == OCCUPIED) && (strcmp(entry->key,key)==0)){
            return entry->value;
        }
    }
    return NULL;
}

void remove_from_table(Hash_Table *table, const char *key) 
{
    size_t hash = get_string_hash(key) % table->size;

    for(size_t i = 0; i < table->size; ++i) {
        size_t index = (hash + i ) % table->size;
        Entry *entry = &table->entries[index];

        if(entry->state ==  EMPTY) {
            return;
        }

        if((entry->state == OCCUPIED)&& (strcmp(entry->key, key)==0)) {
            free(entry->key);
            entry->key = NULL;
            entry->value = NULL;

            entry->state = DELETED;
            table->count--;
            return;
        }
    }
}

void destroy_table(Hash_Table *table) 
{
    for(size_t i = 0; i < table->size; ++i) {
        Entry *entry = &table->entries[i];
        // make sure to check the state before you 
        // frickin double free
        if(entry->state == OCCUPIED) {
            free(entry->key);
            // free(entry->value);
        }
    }
    free(table->entries);
    free(table);
}