#include "compiler.h"
#include <string.h>

static int _get_hash(const char *key);

CSymbolTable *new_table(void)
{
    CSymbolTable *table;

    table = (CSymbolTable *)zalloc(sizeof(CSymbolTable), ARENA_1);

    memset(table, 0, sizeof(CSymbolTable));

    return table;
}

void insert(CSymbolTable *table, const char *key, void *data)
{
    CEntry *entry;
    int     hash;

    if(!table || !key)
        return;

    hash = _get_hash(key);

    entry = (CEntry *)zalloc(sizeof(CEntry), ARENA_1);

    entry->key           = key;
    entry->data          = data;
    entry->prev          = table->buckets[hash];
    table->buckets[hash] = entry;

    table->item_count++;
}

void *get(CSymbolTable *table, const char *key)
{
    CEntry *entry;
    int     hash;

    if(!table || !key)
        return NULL;

    hash = _get_hash(key);

    while(table) {
        for(entry = table->buckets[hash]; entry; entry = entry->prev)
            if(entry->key == key)
                return entry->data;
        table = table->prev;
    }

    return NULL;
}

void *get_local(CSymbolTable *table, const char *key)
{
    CEntry *entry = NULL;
    int     hash  = 0;

    if(!table || !key)
        return NULL;

    hash = _get_hash(key);

    for(entry = table->buckets[hash]; entry; entry = entry->prev)
        if(entry->key == key)
            return entry->data;
    return NULL;
}

void enter_scope(CSymbolTable **table)
{
    CSymbolTable *tmp;

    if(!table || !*table)
        return;

    tmp = new_table();

    tmp->prev = *table;

    *table = tmp;
}

void clear_scope(CSymbolTable **table)
{
    if(!table || !*table)
        return;

    *table = (*table)->prev;
}

static int _get_hash(const char *key)
{
    if(!key)
        return 0;

    return ((unsigned)key << 1) & 0xFF;
}
