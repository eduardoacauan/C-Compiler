#include "compiler.h"
#include <stdio.h>

static bool _check_ext(const char *path);

CFile *new_file(const char *path, bool check_ext)
{
    CFile *file;
    FILE  *tmp;

    if(!path || (check_ext && !_check_ext(path)))
        return NULL;

    tmp = fopen(path, "rb");

    if(!tmp) {
        fprintf(stderr, "error opening file '%s'\n", path);
        return NULL;
    }

    file = (CFile *)zalloc(sizeof(CFile), ARENA_1);

    file->path     = path;
    file->line     = 1;
    file->warnings = 0;
    file->errors   = 0;
    file->prev     = NULL;
    
    fseek(tmp, 0, SEEK_END);

    file->fsize = ftell(tmp);

    rewind(tmp);

    file->src = (char *)zalloc(sizeof(char) * file->fsize + 1, ARENA_4);

    fread(file->src, sizeof(char), file->fsize, tmp);

    fclose(tmp);

    file->src[file->fsize] = '\0';

    return file;
}

bool _check_ext(const char *path)
{
    const char *ptr;

    if(!path)
        return false;

    for(ptr = NULL; *path; path++)
        if(*path == '.')
            ptr = path + 1;

    if(!ptr) {
        fprintf(stderr, "extension expected\n");
        return false;
    }

    if(!istrcmp(ptr, "c")) {
        fprintf(stderr, "invalid extension, use '.c'\n");
        return false;
    }

    return true;
}
