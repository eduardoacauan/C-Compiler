#include "compiler.h"

typedef struct CBlock CBlock;

struct CBlock {
    byte   *base;
    byte   *avail;
    byte   *limit;
    CBlock *prev;
};

static CBlock *arena[MAX_ARENAS] = {NULL};

static void   *_malloc(size_t nbytes);
static void    _free(void *ptr);
static CBlock *_new_block(size_t nbytes);

static void    _free_by_id(size_t idx);

void *zalloc(size_t nbytes, size_t idx)
{
    CBlock *blk;
    size_t  align;

    assert(idx >= ARENA_1 && idx < MAX_ARENAS && nbytes);

    blk   = arena[idx];
    align = get_align(nbytes);

    while(blk) {
        if(blk->avail + align < blk->limit) {
            blk->avail += align;
            return blk->avail - align;
        }
        blk = blk->prev;
    }

    nbytes = (nbytes > ARENA_DEFAULT_SIZE) ? nbytes : ARENA_DEFAULT_SIZE;

    blk = _new_block(nbytes);

    blk->prev = arena[idx];

    arena[idx] = blk;

    blk->avail += align;

    return blk->avail - align;
}

void zfree(void)
{
    _free_by_id(ARENA_1);
    _free_by_id(ARENA_2);
    _free_by_id(ARENA_3);
    _free_by_id(ARENA_4);
    _free_by_id(ARENA_5);
}

static void _free_by_id(size_t idx)
{
    assert(idx >= ARENA_1 && idx < MAX_ARENAS);

    while(arena[idx]) {
        CBlock *tmp = arena[idx]->prev;
        _free(arena[idx]->base);
        _free(arena[idx]);
        arena[idx] = tmp;
    }

    arena[idx] = NULL;
}

static void *_malloc(size_t nbytes)
{
   void  *ptr;

    ptr = malloc(nbytes);

    assert(ptr);

    return ptr;
}

static void _free(void *ptr)
{
    if(!ptr)
        return;

    free(ptr);
}

static CBlock *_new_block(size_t nbytes)
{
    CBlock *blk;

    blk        = (CBlock *)_malloc(sizeof(CBlock));
    blk->base  = (byte *)_malloc(sizeof(byte) * nbytes);
    blk->avail = blk->base;
    blk->limit = blk->avail + nbytes;
    blk->prev  = NULL;

    return blk;
}
