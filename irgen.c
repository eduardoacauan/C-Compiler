#include "compiler.h"
#include "misc.h"

static CMisc* _generate_from_tree(CCompiler *cmp, CNode *tree);
static void   _generate_fun(CCompiler *cmp, CNode *tree);
static void   _generate_load(CCompiler *cmp, CNode *tree);
static void   _generate_vdecl(CCompiler *cmp, CNode *tree);
static void   _generate_id(CCompiler *cmp, CNode *tree);
static void   _generate_bin(CCompiler *cmp, CNode *tree);
static void   _generate_assign(CCompiler *cmp, CNode *tree);
static void   _generate_if(CCompiler *cmp, CNode *tree);
static void   _generate_while(CCompiler *cmp, CNode *tree);
static void   _generate_do_while(CCompiler *cmp, CNode *tree);
static void   _generate_return(CCompiler *cmp, CNode *tree);
static void   _generate_for(CCompiler *cmp, CNode *tree);

static CMisc *_new_label(size_t *id);

static Instruction _get_op(int op);

void start_irgen(CCompiler *cmp)
{
    if(!cmp)
        return;

    cmp->misc.val    = 0; // virtual reg count
    cmp->label_count = 0;

    for(CNode **ptr = &cmp->nodes; *ptr; ptr = &(*ptr)->next_stmt)
        _generate_from_tree(cmp, *ptr);
}

static CMisc *_generate_from_tree(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return NULL;

    switch(tree->kind) {
        case FNDECL:
            _generate_fun(cmp, tree);
            break;
        case LITERAL:
        case IDENTIFIER:
            _generate_load(cmp, tree);
            return cmp->tail->arg1;
        case VARDECL:
            _generate_vdecl(cmp, tree);
            break;
        case BLOCK:
            for(CNode **ptr = &tree->blk.head; *ptr; ptr = &(*ptr)->next_stmt)
                _generate_from_tree(cmp, *ptr);
            break;
        case BINARYEXPR:
            _generate_bin(cmp, tree);
            return cmp->tail->arg1;
        case ASSIGN:
            _generate_assign(cmp, tree);
            return cmp->tail->arg1;
        case IF:
            _generate_if(cmp, tree);
            break;
        case WHILE:
            _generate_while(cmp, tree);
            break;
        case DO_WHILE:
            _generate_do_while(cmp, tree);
            break;
        case RETURN:
            _generate_return(cmp, tree);
            break;
        case FOR:
            _generate_for(cmp, tree);
            break;
    }

    return NULL;
}

static void _generate_do_while(CCompiler *cmp, CNode *tree)
{
    CMisc *lb, *lb2;

    if(!cmp || !tree)
        return;

    lb  = _new_label(&cmp->label_count);
    lb2 = _new_label(&cmp->label_count);

    add_ir(cmp, new_instruction(INS_LABEL, lb, NULL, NULL, tree->type, tree->line));
    _generate_from_tree(cmp, tree->_while.then);
    _generate_from_tree(cmp, tree->_while.cond);
    add_ir(cmp, new_instruction(INS_JMPZ,  lb2,  NULL, NULL, tree->type, tree->line));
    add_ir(cmp, new_instruction(INS_JMP,   lb,   NULL, NULL, tree->type, tree->line));
    add_ir(cmp, new_instruction(INS_LABEL, lb2,  NULL, NULL, tree->type, tree->line));
}

static void _generate_while(CCompiler *cmp, CNode *tree)
{
    CMisc *lb, *lb2;

    if(!cmp || !tree)
        return;

    lb   = _new_label(&cmp->label_count);
    lb2  = _new_label(&cmp->label_count);

    add_ir(cmp, new_instruction(INS_LABEL, lb, NULL, NULL, tree->type, tree->line));

    _generate_from_tree(cmp, tree->_while.cond);

    add_ir(cmp, new_instruction(INS_JMPZ, lb2, NULL, NULL, tree->type, tree->line));

    _generate_from_tree(cmp, tree->_while.then);

    add_ir(cmp, new_instruction(INS_JMP,   lb,  NULL, NULL, tree->type, tree->line));
    add_ir(cmp, new_instruction(INS_LABEL, lb2, NULL, NULL, tree->type, tree->line));
}

static void _generate_if(CCompiler *cmp, CNode *tree)
{
    CMisc  *arg;
    CMisc  *lb, *lb2;

    if(!cmp || !tree)
        return;

    lb        = _new_label(&cmp->label_count);
    
    _generate_from_tree(cmp, tree->_if.cond);
    add_ir(cmp, new_instruction(INS_JMPZ, lb, NULL, NULL, tree->type, tree->line));

    _generate_from_tree(cmp, tree->_if.then);

    if(!tree->_if._else) {
        add_ir(cmp, new_instruction(INS_LABEL, lb, NULL, NULL, tree->type, tree->line));
        return;
    }

    lb2 = _new_label(&cmp->label_count);

    add_ir(cmp, new_instruction(INS_JMP,  lb2, NULL, NULL, tree->type, tree->line));
    add_ir(cmp, new_instruction(INS_LABEL, lb, NULL, NULL, tree->type, tree->line));

    _generate_from_tree(cmp, tree->_if._else);
    
    add_ir(cmp, new_instruction(INS_LABEL, lb2, NULL, NULL, tree->type, tree->line));
}

static void _generate_for(CCompiler *cmp, CNode *tree)
{
    CMisc *lb, *lb2;

    if(!cmp || !tree)
        return;

    lb  = _new_label(&cmp->label_count);
    lb2 = _new_label(&cmp->label_count);

    _generate_from_tree(cmp, tree->_for.init);

    add_ir(cmp, new_instruction(INS_LABEL, lb, NULL, NULL, tree->type, tree->line));

    _generate_from_tree(cmp, tree->_for.cond);

    add_ir(cmp, new_instruction(INS_JMPZ, lb2, NULL, NULL, tree->type, tree->line));

    _generate_from_tree(cmp, tree->_for.then);

    _generate_from_tree(cmp, tree->_for.step);

    add_ir(cmp, new_instruction(INS_JMP,   lb,  NULL, NULL, tree->type, tree->line));
    add_ir(cmp, new_instruction(INS_LABEL, lb2, NULL, NULL, tree->type, tree->line));
}

static void _generate_return(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    if(!tree->ret.expr) {
        add_ir(cmp, new_instruction(INS_RET, NULL, NULL, NULL, tree->type, tree->line));
        return;
    }

    add_ir(cmp, new_instruction(INS_RETVAL, _generate_from_tree(cmp, tree->ret.expr), NULL, NULL, tree->type, tree->line));
}

static void _generate_fun(CCompiler *cmp, CNode *tree)
{
    CMisc *arg;
    
    if(!cmp || !tree)
        return;

    arg      = new_misc(MISC_CONSTANT_INT);
    arg->val = 0;
    arg->sym = tree->decl.symbol;

    cmp->misc.val = 0;

    add_ir(cmp, new_instruction(INS_ENTER, arg, NULL, NULL, tree->type, tree->line));
    _generate_from_tree(cmp, tree->decl.init);
    add_ir(cmp, new_instruction(INS_LEAVE, arg, NULL, NULL, tree->type, tree->line));
}

static void _generate_load(CCompiler *cmp, CNode *tree)
{
    CMisc *arg1;

    if(!cmp || !tree)
        return;

    arg1 = new_misc(MISC_VREG);

    arg1->vreg = new_virtual_register(cmp->misc.val);

    cmp->misc.val++;

    add_ir(cmp, new_instruction(INS_LOAD, arg1, tree->misc, NULL, tree->type, tree->line));
}

static void _generate_vdecl(CCompiler *cmp, CNode *tree)
{
    CMisc *arg1;

    if(!cmp || !tree)
        return;

    for(CNode **ptr = &tree; *ptr; ptr = &(*ptr)->next) {
        if(!(*ptr)->decl.init)
            continue;
        arg1 = new_misc(MISC_SYMBOL);
        arg1->sym = (*ptr)->decl.symbol;

        add_ir(cmp, new_instruction(INS_STORE, arg1, _generate_from_tree(cmp, (*ptr)->decl.init), NULL, (*ptr)->type, (*ptr)->line));
    }
}

static void _generate_bin(CCompiler *cmp, CNode *tree)
{
    CMisc *arg;

    if(!cmp || !tree)
        return;

    add_ir(cmp, new_instruction(_get_op(tree->bin.op), _generate_from_tree(cmp, tree->bin.lhs), _generate_from_tree(cmp, tree->bin.rhs), NULL, tree->type, tree->line));
}

static Instruction _get_op(int op)
{
    switch(op) {
        case '+':
            return INS_ADD;
        case '-':
            return INS_SUB;
        case '*':
            return INS_MUL;
        case '/':
            return INS_DIV;
        case TK_SHL:
            return INS_SHL;
        case TK_SHR:
            return INS_SHR;
        case TK_GE:
            return INS_GE;
        case '>':
            return INS_GT;
        case TK_LE:
            return INS_LE;
        case '<':
            return INS_LT;
        case '&':
            return INS_AND;
    }

    return INS_ADD;
}

static void _generate_assign(CCompiler *cmp, CNode *tree)
{
    CMisc *arg1, *arg2;

    if(!cmp || !tree)
        return;

    if(tree->bin.lhs->kind != IDENTIFIER)
        arg1 = _generate_from_tree(cmp, tree->bin.lhs);
    else
        arg1 = tree->bin.lhs->misc;
    arg2 = _generate_from_tree(cmp, tree->bin.rhs);

    add_ir(cmp, new_instruction(INS_STORE, arg1, arg2, NULL, tree->type, tree->line));
}

static CMisc *_new_label(size_t *id)
{
    CMisc *misc;

    if(!id)
        return NULL;

    misc = new_misc(MISC_LABEL);

    misc->label = new_label(NULL, (*id)++);

    return misc;
}

