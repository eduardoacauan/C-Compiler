#include "misc.h"
#include "compiler.h"

size_t get_align(size_t size)
{
    return (size + (sizeof(UAlign) - 1)) & ~(sizeof(UAlign) - 1);
}

bool istrcmp(const char *s1, const char *s2)
{
    if(!s1 || !s2)
        return false;

    while(*s1 && *s2)
        if((*s1++ & 0xDF) != (*s2++ & 0xDF))
            return false;
    return !*s1 && !*s2;
}

void error(CCompiler *cmp, size_t opt_line, const char *msg, ...)
{
    va_list ap;
    size_t  line;

    if(!cmp || !msg)
        return;

    line = opt_line ? opt_line : cmp->file->line;

    va_start(ap, msg);

    fprintf(stderr, "Error('%s', %d): ", cmp->file->path, line);

    vfprintf(stderr, msg, ap);

    cmp->file->errors++;

    cmp->flags |= COMPILER_FLAG_ERROR;
}

CType *make_ptr(CType *base)
{
    CType *ptr = new_type();

    ptr->kind = PTR;
    ptr->base = base;
    ptr->size = sizeof(void *);
    
    return ptr;
}

CType *new_type(void)
{
    CType *type;

    type = (CType *)zalloc(sizeof(CType), ARENA_1);

    memset(type, 0, sizeof(CType));

    return type;
}

void print_type(CType *type)
{
    if(!type)
        return;

    switch(type->kind) {
        case PTR:
            printf("PTR -> ");
            print_type(type->base);
            return;
        case ARRAY:
            printf("ARRAY -> ");
            print_type(type->base);
            return;
        case FUNCTION:
            printf("FUNCTION -> ");
            printf("(");
                for(CParameter *p = type->params; p; p = p->next) {
                    print_type(p->type);
                    if(p->next)
                        printf(", ");
                }
            printf(") -> ");
            print_type(type->base);
            return;
    }

    printf("%s", type->name);
}

void warn(CCompiler *cmp, size_t opt_line, const char *msg, ...)
{
    va_list ap;
    size_t  line;

    if(!cmp || !msg)
        return;

    line = opt_line ? opt_line : cmp->file->line;

    va_start(ap, msg);

    fprintf(stderr, "Warning('%s', %d): ", cmp->file->path, line);

    vfprintf(stderr, msg, ap);

    cmp->file->warnings++;
}

void expect(CCompiler *cmp, int tokenex)
{
    if(!cmp)
        return;

    if(cmp->token == tokenex)
        return;

    if(tokenex == TK_ID) {
        error(cmp, 0, "Identifier expected\n");
        return;
    }

    error(cmp, 0, "'%c' expected\n", (char)tokenex);
}

void accept(CCompiler *cmp, int tokenex)
{
    if(!cmp)
       return;

    if(cmp->token == tokenex) {
        lex(cmp);
        return;
    }

    if(tokenex == TK_ID) {
        error(cmp, 0, "Identifier expected\n");
        return;
    }

    error(cmp, 0, "'%c' expected\n", (char)tokenex);   
}

CType *new_function(CType *base, CParameter *params, size_t param_count)
{
    CType *fun;

    fun               = new_type();
    fun->kind         = FUNCTION;
    fun->base         = base;
    fun->params       = params;
    fun->param_count  = param_count;

    return fun;
}

CParameter  *new_param(void)
{
    CParameter *param;

    param = (CParameter *)zalloc(sizeof(CParameter), ARENA_1);

    param->sym  = NULL;
    param->type = NULL;
    param->next = NULL;

    return param;
}

CSymbol *new_symbol(void)
{
    CSymbol *sym;

    sym = (CSymbol *)zalloc(sizeof(CSymbol), ARENA_1);

    memset(sym, 0, sizeof(CSymbol));

    return sym;
}

CNode *new_tree(TreeKind kind, size_t line)
{
    CNode *tree;

    tree = (CNode *)zalloc(sizeof(CNode), ARENA_2);

    memset(tree, 0, sizeof(CNode));

    tree->kind = kind;
    tree->line = line;

    return tree;
}

CMisc *new_misc(MiscKind kind)
{
    CMisc *misc;

    misc = (CMisc *)zalloc(sizeof(CMisc), ARENA_1);

    misc->kind = kind;
    misc->val  = 0;

    return misc;
}

CType *new_array(CType *base, CNode *size_expr)
{
    CType *array;

    if(!base)
        return NULL;

    array = new_type();

    array->kind            = ARRAY;
    array->base            = base;
    array->array_dimension = size_expr;

    return array;
}

void printf_type(CType *type, FILE *out)
{
    if(!type || !out)
        return;

    switch(type->kind) {
        case PTR:
        case ARRAY:
            printf_type(type->base, out);
            if(type->base->kind != PTR && type->base->kind != ARRAY)
                fprintf(out, " ");
            fprintf(out, "*");
            return;
        case FUNCTION:
            fprintf(out, "function");
            return;
        default:
            fprintf(out, "%s", type->name);
            return;
    }
}

CInstruction *new_instruction(Instruction kind, CMisc *arg1, CMisc *arg2, CMisc *arg3, CType *type, size_t line)
{
    CInstruction *ins;

    ins = (CInstruction *)zalloc(sizeof(CInstruction), ARENA_3);

    ins->kind = kind;
    ins->arg1 = arg1;
    ins->arg2 = arg2;
    ins->arg3 = arg3;
    ins->type = type;
    ins->line = line;
    ins->prev = NULL;
    ins->next = NULL;

    return ins;
}

void add_ir(CCompiler *cmp, CInstruction *ins)
{
    if(!cmp || !ins)
        return;

    if(!cmp->head) {
        cmp->head = ins;
        cmp->tail = ins;
        return;
    }

    ins->prev = cmp->tail;

    cmp->tail->next = ins;
    cmp->tail       = cmp->tail->next;
}

CVirtualReg *new_virtual_register(size_t reg_count)
{
    CVirtualReg *vreg;

    vreg = (CVirtualReg *)zalloc(sizeof(CVirtualReg), ARENA_1);

    vreg->usage = 0;
    vreg->reg   = 0;
    vreg->id    = reg_count;

    return vreg;
}

CLabel *new_label(const char *opt_name, size_t id)
{
    CLabel *lb;

    lb = (CLabel *)zalloc(sizeof(CLabel), ARENA_1);

    lb->opt_name = opt_name;
    lb->address  = 0;
    lb->lbID     = id;

    return lb;
}
