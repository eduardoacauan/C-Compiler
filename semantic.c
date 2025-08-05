#include "compiler.h"
#include "misc.h"
#include <stdio.h>

static void   _analyse_tree(CCompiler *cmp, CNode *tree);
static void   _analyse_fn_proto(CCompiler *cmp, CNode *tree);
static void   _analyse_fn_decl(CCompiler *cmp, CNode *tree);
static void   _analyse_blk(CCompiler *cmp, CNode *tree);
static void   _analyse_vdecl(CCompiler *cmp, CNode *tree);
static void   _analyse_id(CCompiler *cmp, CNode *tree);
static void   _analyse_fn_call(CCompiler *cmp, CNode *tree);
static void   _analyse_dereference(CCompiler *cmp, CNode *tree);
static void   _analyse_addr(CCompiler *cmp, CNode *tree);
static void   _analyse_bin(CCompiler *cmp, CNode *tree);
static void   _analyse_assign(CCompiler *cmp, CNode *tree);
static void   _analyse_return(CCompiler *cmp, CNode *tree);
static void   _analyse_if(CCompiler *cmp, CNode *tree);
static void   _analyse_while(CCompiler *cmp, CNode *tree);
static void   _analyse_do_while(CCompiler *cmp, CNode *tree);
static void   _analyse_for(CCompiler *cmp, CNode *tree);
static void   _analyse_minus(CCompiler *cmp, CNode *tree);
static void   _analyse_not(CCompiler *cmp, CNode *tree);
static void   _analyse_prefix_postfix(CCompiler *cmp, CNode *tree);
static void   _analyse_plus(CCompiler *cmp, CNode *tree);

static void   _print_incompatible_types(CCompiler *cmp, CType *t1, CType *t2, size_t line);
static void   _print_warn_loss_of_info(CCompiler *cmp, CType *t1, CType *t2, size_t line);
static void   _verify_bitwise_with_float(CCompiler *cmp, CNode *bin);

static bool   _is_same_type(CType *t1, CType *t2);
static bool   _is_lvalue(CNode *tree);

static bool   _can_convert(CCompiler *cmp, CType *from, CType *to, size_t line);
static CType *_promote(CType *t1, CType *t2);
static bool   _can_operate(CType *t1, CType *t2);
static void   _valid_condition(CCompiler *cmp, CType *cond, size_t line);

void start_semantic_analyser(CCompiler *cmp)
{
    if(!cmp)
        return;

    for(CNode **ptr = &cmp->nodes; *ptr; ptr = &(*ptr)->next_stmt)
        _analyse_tree(cmp, *ptr);
}

static void _analyse_tree(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    switch(tree->kind) {
        case FNPROTO:
            _analyse_fn_proto(cmp, tree);
            return;
        case FNDECL:
            _analyse_fn_decl(cmp, tree);
            return;
        case BLOCK:
            _analyse_blk(cmp, tree);
            return;
        case VARDECL:
            _analyse_vdecl(cmp, tree);
            return;
        case IDENTIFIER:
            _analyse_id(cmp, tree);
            return;
        case FNCALL:
            _analyse_fn_call(cmp, tree);
            return;
        case DEREFERENCE:
            _analyse_dereference(cmp, tree);
            return;
        case ADDROF:
            _analyse_addr(cmp, tree);
            return;
        case BINARYEXPR:
            _analyse_bin(cmp, tree);
            return;
        case ASSIGN:
            _analyse_assign(cmp, tree);
            return;
        case RETURN:
            _analyse_return(cmp, tree);
            return;
        case IF:
            _analyse_if(cmp, tree);
            return;
        case WHILE:
            _analyse_while(cmp, tree);
            return;
        case DO_WHILE:
            _analyse_do_while(cmp, tree);
            return;
        case FOR:
            _analyse_for(cmp, tree);
            return;
        case MINUS:
            _analyse_minus(cmp, tree);
            return;
        case NOT:
            _analyse_not(cmp, tree);
            return;
        case PREFIX:
        case POSTFIX:
            _analyse_prefix_postfix(cmp, tree);
            break;
        case PLUS:
            _analyse_plus(cmp, tree);
            break;
    }
}

static void _analyse_prefix_postfix(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    _analyse_tree(cmp, tree->unary.base);

    _valid_condition(cmp, tree->unary.base->type, tree->line);

    tree->type = tree->unary.base->type;
}

static void _analyse_not(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    _analyse_tree(cmp, tree->unary.base);

    _valid_condition(cmp, tree->unary.base->type, tree->line);

    tree->type = tree->unary.base->type;
}

static void _analyse_minus(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;
    _analyse_tree(cmp, tree->unary.base);

    _valid_condition(cmp, tree->unary.base->type, tree->line);

    tree->type = tree->unary.base->type;

    if(tree->type->kind == UCHAR || tree->type->kind == USHORT || tree->type->kind == UINT || tree->type->kind == ULONG)
        error(cmp, tree->line, "Cannot use '-' in a unsigned expression\n");
}

static void _analyse_for(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    cmp->flags |= COMPILER_FLAG_DONT_PUSH_SCOPE;

    enter_scope(&cmp->tables[SYMBOLS]);

    _analyse_tree(cmp, tree->_for.init);
    _analyse_tree(cmp, tree->_for.cond);

    _valid_condition(cmp, tree->_for.cond->type, tree->line);

    _analyse_tree(cmp, tree->_for.step);
    _analyse_tree(cmp, tree->_for.then);

    if(cmp->flags & COMPILER_FLAG_DONT_PUSH_SCOPE) {
        cmp->flags &= ~COMPILER_FLAG_DONT_PUSH_SCOPE;
        clear_scope(&cmp->tables[SYMBOLS]);
    }
}

static void _analyse_do_while(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    _analyse_tree(cmp, tree->_while.then);

    _analyse_tree(cmp, tree->_while.cond);

    _valid_condition(cmp, tree->_while.cond->type, tree->line);
}

static void _analyse_while(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    _analyse_tree(cmp, tree->_while.cond);

    _valid_condition(cmp, tree->_while.cond->type, tree->line);

    _analyse_tree(cmp, tree->_while.then);
}

static void _analyse_if(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    _analyse_tree(cmp, tree->_if.cond);

    _valid_condition(cmp, tree->_if.cond->type, tree->line);

    _analyse_tree(cmp, tree->_if.then);
    _analyse_tree(cmp, tree->_if._else);
}

static void _analyse_fn_call(CCompiler *cmp, CNode *tree)
{
    CNode      *arg;
    CParameter *params;

    if(!cmp || !tree)
        return;

    _analyse_tree(cmp, tree->fncall.base);

    if(tree->fncall.base->type->kind != FUNCTION)
        error(cmp, tree->line, "Function expected at function call\n");

    tree->type = tree->fncall.base->type->base;

    if(tree->fncall.count != tree->fncall.base->type->param_count)
        error(cmp, tree->line, "Argument count mismatch at function call\n");

    params = tree->fncall.base->type->params;
    
    for(arg = tree->fncall.args; arg; arg = arg->next_stmt) {
        _analyse_tree(cmp, arg);
        if(!params)
            continue;
        if(!_is_same_type(arg->type, params->type))
           _print_incompatible_types(cmp, arg->type, params->type, tree->line);
        params = params->next;
    }
}

static void _analyse_id(CCompiler *cmp, CNode *tree)
{
    CSymbol *sym;

    if(!cmp || !tree)
        return;

    sym = get(cmp->tables[SYMBOLS], tree->misc->str);

    if(!sym) {
        error(cmp, tree->line, "Undefined reference to identifier '%s'\n", tree->misc->str);
        tree->type = cmp_primitives[INT];
        return;
    }

    tree->type = sym->type;

    tree->misc->kind = MISC_SYMBOL;
    tree->misc->sym  = sym;
}

static void _analyse_vdecl(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    for(CNode **ptr = &tree; *ptr; ptr = &(*ptr)->next) {

        CSymbol *var = (*ptr)->decl.symbol;

        if(get_local(cmp->tables[SYMBOLS], var->name))
            error(cmp, tree->line, "Variable '%s' already declared in this scope\n", var->name);
        else
            insert(cmp->tables[SYMBOLS], var->name, var);

        if(!(*ptr)->decl.init)
            continue;

        _analyse_tree(cmp, (*ptr)->decl.init);

        if(!_can_convert(cmp, var->type, (*ptr)->decl.init->type, (*ptr)->line))
            _print_incompatible_types(cmp, var->type, (*ptr)->decl.init->type, (*ptr)->line);
        (*ptr)->type = var->type;
    }
}

static void _analyse_blk(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    cmp->flags &= ~COMPILER_FLAG_GLOBAL_SCOPE;
    cmp->flags |=  COMPILER_FLAG_LOCAL_SCOPE;

    if(!(cmp->flags & COMPILER_FLAG_DONT_PUSH_SCOPE))
        enter_scope(&cmp->tables[SYMBOLS]);

    cmp->flags &= ~COMPILER_FLAG_DONT_PUSH_SCOPE;

    for(CNode **ptr = &tree->blk.head; *ptr; ptr = &(*ptr)->next_stmt)
        _analyse_tree(cmp, *ptr);
    clear_scope(&cmp->tables[SYMBOLS]);
}

static void _analyse_bin(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    _analyse_tree(cmp, tree->bin.lhs);
    _analyse_tree(cmp, tree->bin.rhs);

    if(!_can_operate(tree->bin.lhs->type, tree->bin.rhs->type))
        error(cmp, tree->line, "Arithmetic or pointer expression expected\n");

    tree->type = _promote(tree->bin.lhs->type, tree->bin.rhs->type);
    
    _verify_bitwise_with_float(cmp, tree);
}

static void _analyse_fn_decl(CCompiler *cmp, CNode *tree)
{
    CSymbol    *proto        = NULL;
    CParameter *params_proto = NULL;
    CSymbol    *fun          = NULL;

    if(!cmp || !tree)
        return;

    fun = tree->decl.symbol;
    
    proto = get(cmp->tables[SYMBOLS], tree->decl.symbol->name);

    if(proto) {
        if(proto->flags & SYMBOL_HAS_BEEN_PROTOTYPED) {
            params_proto = proto->type->params;
            if(!_is_same_type(proto->type, fun->type))
                error(cmp, tree->line, "Function '%s' return type mismatch\n", fun->name);
        }
        else
            error(cmp, tree->line, "Function '%s' already have a body\n", fun->name);
    }

    insert(cmp->tables[SYMBOLS], fun->name, fun);

    enter_scope(&cmp->tables[SYMBOLS]);

    cmp->flags |= COMPILER_FLAG_DONT_PUSH_SCOPE;

    for(CParameter *param = fun->type->params; param; param = param->next) {
        if(!param->sym)
            error(cmp, tree->line, "Cannot have a unnamed parameter inside a function with a body\n");
        else {
            if(get_local(cmp->tables[SYMBOLS], param->sym->name))
                error(cmp, tree->line, "Parameter '%s' already declared in function '%s'\n", param->sym->name, fun->name);
            else
                insert(cmp->tables[SYMBOLS], param->sym->name, param->sym);
        }
        if(params_proto) {
            if(!_is_same_type(param->type, params_proto->type))
                error(cmp, tree->line, "Parameter '%s' type mismatch in function '%s'\n", param->sym->name, fun->name);
            params_proto = params_proto->next;
        }
    }

    cmp->misc.type = tree->decl.symbol->type->base;

    _analyse_tree(cmp, tree->decl.init);

    cmp->misc.type = NULL;

    tree->type = tree->decl.symbol->type->base;
}

static void _analyse_fn_proto(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    insert(cmp->tables[SYMBOLS], tree->decl.symbol->name, tree->decl.symbol);

    tree->decl.symbol->flags |= SYMBOL_HAS_BEEN_PROTOTYPED;
}

static bool _is_same_type(CType *t1, CType *t2)
{
    if(!t1 || !t2)
        return false;

    if(t1->kind != t2->kind)
        return false;

    switch(t1->kind) {
        case PTR:
        case ARRAY:
            return _is_same_type(t1->base, t2->base);
        case FUNCTION:
            for(CParameter *p1 = t1->params, *p2 = t2->params; p1 && p2; p1 = p1->next, p2 = p2->next)
                if(!_is_same_type(p1->type, p2->type))
                    return false;
            if(t1->param_count != t2->param_count)
                return false;
            return _is_same_type(t1->base, t2->base);
        default:
            return true;
    }
}

static bool _can_convert(CCompiler *cmp, CType *from, CType *to, size_t line)
{
    if(!from || !to)
        return false;

    if(from->kind > VOID && from->kind < END_PRIMITIVES && to->kind > VOID && to->kind < END_PRIMITIVES) {
        if(from->kind < FLOAT && to->kind > ULONG)
            _print_warn_loss_of_info(cmp, to, from, line);
        return true;
    }

    if(from->kind == PTR && to->kind == PTR) {
        if(from->base->kind == VOID || to->base->kind == VOID)
            return true;
        return _is_same_type(from, to);
    }

    if(from->kind == ARRAY && to->kind == PTR)
        return _is_same_type(from->base, to->base);

    return _is_same_type(from, to);
}

static CType *_promote(CType *t1, CType *t2)
{
    if(!t1 || !t2)
        return NULL;

    return t1->kind > t2->kind ? t1 : t2;
}

static void _print_incompatible_types(CCompiler *cmp, CType *t1, CType *t2, size_t line)
{
    if(!cmp || !t1 || !t2)
        return;

    error(cmp, line, "Incompatible types '");

    printf_type(t1, stderr);

    fprintf(stderr, "' and '");

    printf_type(t2, stderr);

    fprintf(stderr, "'\n");
}

static void _print_warn_loss_of_info(CCompiler *cmp, CType *t1, CType *t2, size_t line)
{
    if(!cmp || !t1 || !t2)
        return;

    error(cmp, line, "Converting from '");

    printf_type(t1, stderr);

    fprintf(stderr, "' to '");

    printf_type(t2, stderr);

    fprintf(stderr, "' can cause loss of information\n");
}

static bool _can_operate(CType *t1, CType *t2)
{
    if(!t1 || !t2)
        return false;

    return t1->kind > VOID && t1->kind <= PTR && t2->kind > VOID && t2->kind <= PTR;
}

static void _analyse_dereference(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    _analyse_tree(cmp, tree->unary.base);

    if(tree->unary.base->type->kind != PTR) {
        error(cmp, tree->line, "Cannot dereference a non pointer\n");
        tree->type = tree->unary.base->type;
        return;
    }

    tree->type = tree->unary.base->type->base;
}

static void _analyse_addr(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    _analyse_tree(cmp, tree->unary.base);

    tree->type = make_ptr(tree->unary.base->type);
}

static void _analyse_assign(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    _analyse_tree(cmp, tree->bin.lhs);
    _analyse_tree(cmp, tree->bin.rhs);

    if(!_is_lvalue(tree->bin.lhs))
        error(cmp, tree->line, "Invalid lvalue\n");

    if(!_can_convert(cmp, tree->bin.lhs->type, tree->bin.rhs->type, tree->line))
        _print_incompatible_types(cmp, tree->bin.lhs->type, tree->bin.rhs->type, tree->line);

    tree->type = tree->bin.lhs->type;

    _verify_bitwise_with_float(cmp, tree);
}

static bool _is_lvalue(CNode *tree)
{
    if(!tree)
        return false;

    switch(tree->kind) {
        case DEREFERENCE:
        case IDENTIFIER:
        case ARRAY_ACCESS:
        case MEMBER_ACCESS:
        case MEMBER_PTR_ACCESS:
            return true;
        default:
            return false;
    }
}

static void _verify_bitwise_with_float(CCompiler *cmp, CNode *bin)
{
    if(!cmp || !bin)
        return;

    switch(bin->bin.op) {
        case TK_SHL:
        case TK_SHR:
        case '&':
        case '|':
        case '^':
        case TK_SHL_EQ:
        case TK_SHR_EQ:
        case TK_AND_EQ:
        case TK_OR_EQ:
        case TK_XOR_EQ:
            if(bin->type->kind >= FLOAT && bin->type->kind < END_PRIMITIVES)
                error(cmp, bin->line, "Cannot do bitwise operations with float/double/long double\n");
        default:
            return;
    }
}

static void _analyse_return(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    if(!tree->ret.expr)
        tree->type = cmp_primitives[VOID];
    else {
        _analyse_tree(cmp, tree->ret.expr);
        tree->type = tree->ret.expr->type;
    }

    if(!_can_convert(cmp, cmp->misc.type, tree->type, tree->line))
        _print_incompatible_types(cmp, tree->type, cmp->misc.type, tree->line);

    tree->type = cmp->misc.type;
}

static void _valid_condition(CCompiler *cmp, CType *cond, size_t line)
{
    if(!cmp || !cond || cond->kind <= PTR)
        return;

    error(cmp, line, "Arithmetic or pointer expression expected\n");
}

static void _analyse_plus(CCompiler *cmp, CNode *tree)
{
    if(!cmp || !tree)
        return;

    _analyse_tree(cmp, tree->unary.base);

    _valid_condition(cmp, tree->unary.base->type, tree->line);

    tree->type = tree->unary.base->type;
}
