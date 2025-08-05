#include "compiler.h"

byte opTable[ASCII_MAX] = {0};

extern void       _init_op_table(void);
extern CCompiler *_new_compiler(const char *path);
extern void       _compile_file(const char *path);
extern void       _parse_file(CCompiler *cmp);

void boot(int argc, char **argv)
{
    if(!argc || !argv)
        return;

    init_keywords();
    _init_op_table();

    for(int i = 1; i < argc; i++)
        _compile_file(argv[i]);
}

void _compile_file(const char *path)
{
    CCompiler *cmp;

    if(!(cmp = _new_compiler(path)))
       return;

    _parse_file(cmp);

    if(cmp->flags & COMPILER_FLAG_ERROR)
        return;

    start_semantic_analyser(cmp);

    if(cmp->flags & COMPILER_FLAG_ERROR)
        return;

    start_irgen(cmp);

    print_ir(cmp);
}

void _parse_file(CCompiler *cmp)
{
    CNode **ptr;

    ptr = &cmp->nodes;

    while(lex(cmp)) {
        *ptr = prs_translation_unit(cmp);
        if(*ptr)
            ptr = &(*ptr)->next_stmt;
    }
}

CCompiler *_new_compiler(const char *path)
{
    CCompiler *cmp;
    
    if(!path)
        return NULL;

    cmp = (CCompiler *)zalloc(sizeof(CCompiler), ARENA_1);

    memset(cmp, 0, sizeof(CCompiler));

    cmp->file = new_file(path, true);

    if(!cmp->file)
        return NULL;

    for(int i = 0; i < MAX_TABLES; i++)
        cmp->tables[i] = new_table();

    return cmp;
}

void _init_op_table(void)
{
    opTable['*']       = 13;
	opTable['/']       = 13;
	opTable['%']       = 13;
	opTable['+']       = 12;
	opTable['-']       = 12;
	opTable[TK_SHL]    = 11;
	opTable[TK_SHR]    = 11;
	opTable['<']       = 10;
	opTable['>']       = 10;
	opTable[TK_LE]     = 10;
	opTable[TK_GE]     = 10;
	opTable[TK_EQ_EQ]  = 9;
	opTable[TK_NOT_EQ] = 9;
	opTable['&']       = 8;
	opTable['^']       = 7;
	opTable['|']       = 6;
	opTable[TK_ANDAND] = 5;
	opTable[TK_OROR]   = 4;
	opTable[TK_SHL_EQ] = 2;
	opTable[TK_SHR_EQ] = 2;
	opTable[TK_ADD_EQ] = 2;
	opTable[TK_SUB_EQ] = 2;
	opTable[TK_MUL_EQ] = 2;
	opTable[TK_DIV_EQ] = 2;
	opTable['=']       = 2;
	opTable[',']       = 1;   
}
