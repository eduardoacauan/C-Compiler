#include "compiler.h"
#include "misc.h"

static CNode *_prs_block(CCompiler *cmp);
static CNode *_prs_if(CCompiler *cmp);
static CNode *_prs_do(CCompiler *cmp);
static CNode *_prs_while(CCompiler *cmp);
static CNode *_prs_for(CCompiler *cmp);
static CNode *_prs_brk(CCompiler *cmp);
static CNode *_prs_cnt(CCompiler *cmp);
static CNode *_prs_ret(CCompiler *cmp);
static CNode *_prs_case(CCompiler *cmp);
static CNode *_prs_default(CCompiler *cmp);
static CNode *_prs_switch(CCompiler *cmp);
static CNode *_prs_goto(CCompiler *cmp);

static CNode *_prs_for_init(CCompiler *cmp);
static CNode *_prs_id(CCompiler *cmp);

static void   _check_node(CCompiler *cmp, const char *statement_name, CNode *tree);
CNode *prs_stmt(CCompiler *cmp)
{
    CNode *tree;

    if(!cmp || cmp->token == ';')
        return NULL;

    if(is_typename(cmp) || is_typequalifier(cmp) || is_storage_class(cmp))
        return prs_decl(cmp);

    switch(cmp->token) {
        case '{':
            return _prs_block(cmp);
        case KW_IF:
            return _prs_if(cmp);
        case KW_DO:
            return _prs_do(cmp);
        case KW_WHILE:
            return _prs_while(cmp);
        case KW_FOR:
            return _prs_for(cmp);
        case KW_BREAK:
            return _prs_brk(cmp);
        case KW_CONTINUE:
            return _prs_cnt(cmp);
        case KW_RETURN:
            return _prs_ret(cmp);
        case KW_CASE:
            return _prs_case(cmp);
        case KW_DEFAULT:
            return _prs_default(cmp);
        case KW_SWITCH:
            return _prs_switch(cmp);
        case TK_ID:
            return _prs_id(cmp);
        case KW_GOTO:
            return _prs_goto(cmp);
    }

    tree = prs_expr(cmp, 0);

    expect(cmp, ';');

    return tree;
}

static CNode *_prs_switch(CCompiler *cmp)
{
    CNode *tree;
    CNode **ptr;

    if(!cmp)
        return NULL;

    tree = new_tree(SWITCH, cmp->file->line);

    lex(cmp);

    accept(cmp, '(');

    tree->_switch.cond = prs_expr(cmp, 0);

    accept(cmp, ')');

    accept(cmp, '{');

    ptr = &tree->_switch.cases;

    cmp->switch_count++;

    while(*cmp->file->src && cmp->token != '}') {
        if(cmp->token != KW_CASE && cmp->token != KW_DEFAULT)
            error(cmp, 0, "Invalid statement inside switch body\n");
        *ptr = prs_stmt(cmp);
        if(*ptr)
            ptr = &(*ptr)->next_stmt;
    }

    cmp->switch_count--;

    return tree;
}

static CNode *_prs_ret(CCompiler *cmp)
{
    CNode *tree;

    if(!cmp)
        return NULL;

    tree = new_tree(RETURN, cmp->file->line);

    if(lex(cmp) == ';')
        return tree;

    tree->ret.expr = prs_expr(cmp, 0);

    expect(cmp, ';');

    return tree;
}

static CNode *_prs_for(CCompiler *cmp)
{
    CNode *tree;

    if(!cmp)
        return NULL;

    tree = new_tree(FOR, cmp->file->line);

    lex(cmp);

    accept(cmp, '(');

    tree->_for.init = _prs_for_init(cmp);

    accept(cmp, ';');

    if(cmp->token != ';')
        tree->_for.cond = prs_expr(cmp, 0);

    accept(cmp, ';');

    if(cmp->token != ')')
        tree->_for.step = prs_expr(cmp, 0);

    accept(cmp, ')');

    cmp->loop_count++;
    tree->_for.then = prs_stmt(cmp);
    cmp->loop_count--;

    _check_node(cmp, "for", tree->_for.then);

    return tree;
}

static CNode *_prs_for_init(CCompiler *cmp)
{
    if(!cmp || cmp->token == ';')
        return NULL;

    if(is_typename(cmp) || is_typequalifier(cmp))
       return prs_decl(cmp);

    return prs_expr(cmp, 0);
}

static CNode *_prs_while(CCompiler *cmp)
{
    CNode *tree;

    if(!cmp)
        return NULL;

    tree = new_tree(WHILE, cmp->file->line);

    lex(cmp);

    accept(cmp, '(');

    tree->_while.cond = prs_expr(cmp, 0);

    accept(cmp, ')');

    cmp->loop_count++;

    tree->_while.then = prs_stmt(cmp);

    cmp->loop_count--;

    _check_node(cmp, "while", tree->_while.then);

    return tree;
}

static CNode *_prs_do(CCompiler *cmp)
{
    CNode *tree;

    if(!cmp)
        return NULL;

    tree = new_tree(DO_WHILE, cmp->file->line);

    lex(cmp);

    cmp->loop_count++;
    tree->_while.then = prs_stmt(cmp);
    cmp->loop_count--;

    _check_node(cmp, "do while", tree->_while.then);

    if(lex(cmp) != KW_WHILE)
        error(cmp, 0, "While keyword expected\n");

    lex(cmp);
    accept(cmp, '(');
    tree->_while.cond = prs_expr(cmp, 0);
    accept(cmp, ')');
    expect(cmp, ';');

    return tree;
}

static CNode *_prs_if(CCompiler *cmp)
{
    CNode *tree;

    if(!cmp)
        return NULL;

    tree = new_tree(IF, cmp->file->line);

    lex(cmp);

    accept(cmp, '(');

    tree->_if.cond = prs_expr(cmp, 0);

    accept(cmp, ')');

    tree->_if.then = prs_stmt(cmp);

    _check_node(cmp, "do while", tree->_if.then);

    if(lex(cmp) == KW_ELSE) {
        lex(cmp);
        tree->_if._else = prs_stmt(cmp);
        _check_node(cmp, "else", tree->_if._else);
        return tree;
    }

    cmp->flags |= COMPILER_FLAG_DONT_LEX;

    return tree;
}

static CNode *_prs_block(CCompiler *cmp)
{
    CNode  *blk;
    CNode **ptr;

    if(!cmp)
        return NULL;

    blk = new_tree(BLOCK, cmp->file->line);

    ptr = &blk->blk.head;

    while(*cmp->file->src && lex(cmp) != '}') {
        *ptr = prs_stmt(cmp);
        if(!*ptr)
            continue;
         if((*ptr)->kind == FNPROTO || (*ptr)->kind == FNDECL)
            error(cmp, 0, "Cannot declare a function inside a scope\n");
         ptr = &(*ptr)->next_stmt;
    }

    expect(cmp, '}');

    return blk;
}

static CNode *_prs_brk(CCompiler *cmp)
{
    CNode *tree;

    if(!cmp)
        return NULL;

    tree = new_tree(BREAK, cmp->file->line);

    lex(cmp);

    expect(cmp, ';');

    if(!cmp->loop_count && !cmp->switch_count)
        error(cmp, 0, "Cannot use break statement outside of a loop or switch statement case\n");

    return tree;
}

static CNode *_prs_cnt(CCompiler *cmp)
{
    CNode *tree;

    if(!cmp)
        return NULL;

    tree = new_tree(CONTINUE, cmp->file->line);

    lex(cmp);

    expect(cmp, ';');

    if(!cmp->loop_count)
        error(cmp, 0, "Cannot use continue statement outside of a loop\n");

    return tree;
}

static CNode *_prs_case(CCompiler *cmp)
{
    CNode *tree;
    CNode **ptr;

    if(!cmp)
        return NULL;

    if(!cmp->switch_count)
        error(cmp, 0, "Cannot use case statement outside of a switch\n");

    tree = new_tree(CASE, cmp->file->line);

    lex(cmp);

    tree->_case.cond = prs_expr(cmp, 0);

    accept(cmp, ':');

    ptr = &tree->_case.head;

    while(*cmp->file->src && cmp->token != KW_CASE && cmp->token != KW_DEFAULT && cmp->token != '}') {
        *ptr = prs_stmt(cmp);
        if(*ptr)
            ptr = &(*ptr)->next_stmt;
        lex(cmp);
    }

    return tree;
}

static CNode *_prs_default(CCompiler *cmp)
{
    CNode *tree;
    CNode **ptr;

    if(!cmp)
        return NULL;

    if(!cmp->switch_count)
        error(cmp, 0, "Cannot use case statement outside of a switch\n");

    tree = new_tree(DEFAULT, cmp->file->line);

    lex(cmp);

    accept(cmp, ':');

    ptr = &tree->_case.head;

    while(*cmp->file->src && cmp->token != KW_CASE && cmp->token != KW_DEFAULT && cmp->token != '}') {
        *ptr = prs_stmt(cmp);
        if(*ptr)
            ptr = &(*ptr)->next_stmt;
        lex(cmp);
    }

    return tree;
}

static CNode *_prs_id(CCompiler *cmp)
{
    CNode *tree;

    if(!cmp)
        return NULL;

    tree = prs_expr(cmp, 0);

    expect(cmp, ';');

    return tree;
}

static CNode *_prs_goto(CCompiler *cmp)
{
    CNode *tree;

    if(!cmp)
        return NULL;

    tree = new_tree(GOTO, cmp->file->line);

    lex(cmp);

    expect(cmp, TK_ID);

    tree->misc = new_misc(MISC_ID);

    tree->misc->str = cmp->misc.str;

    lex(cmp);

    expect(cmp, ';');

    return tree;
}

static void _check_node(CCompiler *cmp, const char *statement_name, CNode *tree)
{
    if(!cmp || !statement_name || !tree)
        return;

    if(tree->kind == FNDECL || tree->kind == FNPROTO || tree->kind == VARDECL)
        error(cmp, 0, "Cannot have a declaration as a %s statement body\n");
}
