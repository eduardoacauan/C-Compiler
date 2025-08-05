#include "compiler.h"

typedef struct CString CString;

struct CString {
    char    *str;
    size_t   len;
    CString *prev;
};

static CString *buckets[1024] = {NULL};

const char *atom(const char *string)
{
    const char *ptr;

    assert(string);

    for(ptr = string; *ptr; ptr++);

    return atom_range(string, ptr - string);
}

const char *atom_range(const char *string, size_t len)
{
    const char *end = string;
    CString    *entry;
    unsigned    hash = 0;

    assert(string && len);

    for(size_t i = 0; i < len; i++)
        hash = ((hash << 5) + hash) + *end++;

    hash &= 0x3FF;

    for(entry = buckets[hash]; entry; entry = entry->prev) {
        const char *s1, *s2;

        if(entry->len != len)
            continue;

        s1 = string, s2 = entry->str;

        do 
            if(s1 == end)
                return entry->str;
        while(*s1++ == *s2++);
    }

    entry         = (CString *)zalloc(sizeof(CString), ARENA_1);
    entry->str    = (char *)zalloc(sizeof(char) * len + 1, ARENA_1);
    memcpy(entry->str, string, len);

    entry->len    = len;
    entry->prev   = buckets[hash];
    buckets[hash] = entry;

    return entry->str;
}
