#include "compiler.h"
#include "misc.h"

static const char *ins_name[INS_END_MARK] = {
    "LOAD", "STORE",      "JMPZ",       "ADD",   "SUB",
    "MUL",  "DIV",        "ENTER",      "LEAVE", "SHL",
    "SHR",  "GE",         "LE",         "GT",    "LT",
    "JMP",  NULL/*label*/,"RETVAL",     "RET",   "AND"
};

static void _print_ins(CInstruction *ins);
static void _print_arg(CMisc *arg);

void print_ir(CCompiler *cmp)
{
    if(!cmp)
        return;

    for(CInstruction **ins = &cmp->head; *ins; ins = &(*ins)->next) {
        if((*ins)->kind == INS_LABEL) {
            printf("L%ld:", (*ins)->arg1->label->lbID);
            continue;
        }
        _print_ins(*ins);
    }
}

static void _print_ins(CInstruction *ins)
{
    if(!ins)
        return;

    if(ins->kind == INS_ENTER)
        printf("function '%s'\n", ins->arg1->sym->name);

     printf("\t%s", ins_name[ins->kind]);
        
    _print_arg(ins->arg1);

    if(ins->arg2)
        printf(",");

    _print_arg(ins->arg2);
    
    if(ins->arg3)
        printf(",");

    _print_arg(ins->arg3);

    if(ins->type) {
        printf("\t[");
        printf_type(ins->type, stdout);
        printf("]\t");
    }

    printf("\n");
}

static void _print_arg(CMisc *arg)
{
    if(!arg)
        return;

    switch(arg->kind) {
        case MISC_CONSTANT_FLOAT:
            printf(" %.4lf", arg->dval);
            return;
        case MISC_CONSTANT_INT:
            printf(" %ld", arg->val);
            return;
        case MISC_SYMBOL:
            printf(" %s", arg->sym->name);
            return;
        case MISC_VREG:
            printf(" R%ld", arg->vreg->id);
            return;
        case MISC_LABEL:
            printf(" L%ld", arg->label->lbID);
            return;
    }
}
