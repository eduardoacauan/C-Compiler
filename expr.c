#include "compiler.h"
#include "misc.h"

static CNode *_prs_prefix(CCompiler *cmp);
static CNode *_prs_postfix(CCompiler *cmp, CNode *base);

static CNode *_prs_fn_call(CCompiler *cmp, CNode *base);
static CNode *_prs_array_access(CCompiler *cmp, CNode *base);
static CNode *_prs_member_access(CCompiler *cmp, CNode *base);
static CNode *_prs_member_ptr_access(CCompiler *cmp, CNode *base);
static CNode *_prs_sizeof(CCompiler *cmp);
static CNode *_prs_unary(CCompiler *cmp, TreeKind kind);

CNode *prs_expr(CCompiler *cmp, int power)
{
    CNode *lhs = NULL;

    if(!cmp)
        return NULL;

    lhs = _prs_prefix(cmp);

    while(*cmp->file->src && cmp->token < KEYWORD && power < opTable[cmp->token]) {
        int op     = cmp->token;
        CNode *rhs = NULL;
        CNode *bin = NULL;

        bin = new_tree(BINARYEXPR, cmp->file->line);

        lex(cmp);

        if(op == '=' || op >= TK_ADD_EQ && op <= TK_OR_EQ) {
            bin->kind = ASSIGN;
            rhs       = prs_expr(cmp, opTable[op] - 1);
        }
        else
            rhs = prs_expr(cmp, opTable[op]);

        bin->bin.lhs = lhs;
        bin->bin.rhs = rhs;
        bin->bin.op  = op;

        lhs          = bin;
    }

    return lhs;
}

static CNode *_prs_prefix(CCompiler *cmp)
{
    CNode   *tree = NULL;
    uint64_t u    = 0;

    if(!cmp)
        return NULL;

    switch(cmp->token) {
        case TK_INT:
            tree       = new_tree(LITERAL, cmp->file->line);
            tree->misc = new_misc(MISC_CONSTANT_INT);

            tree->misc->val = cmp->misc.val;
            u               = cmp->misc.val;

            if(tree->misc->val >= INT_MIN && tree->misc->val <= INT_MAX)
                tree->type = cmp_primitives[INT];
            else if(u <= UINT_MAX && tree->misc->val >= 0)
                tree->type = cmp_primitives[UINT];
            else if(tree->misc->val >= LLONG_MIN && tree->misc->val <= LLONG_MAX)
                tree->type = cmp_primitives[LONG];
            else if(u <= ULLONG_MAX && tree->misc->val >= 0)
                tree->type = cmp_primitives[ULONG];
            lex(cmp);
            break;
        case TK_ID:
            tree            = new_tree(IDENTIFIER, cmp->file->line);
            tree->misc      = new_misc(MISC_ID);
            tree->misc->str = cmp->misc.str;
            lex(cmp);
            break;
        case TK_PP:
        case TK_MM:
            tree = new_tree(PREFIX, cmp->file->line);
            tree->unary.op = cmp->token;
            lex(cmp);
            tree->unary.base = _prs_prefix(cmp);
            if(tree->unary.base && tree->unary.base->kind == POSTFIX || tree->unary.base->kind == PREFIX)
                error(cmp, 0, "Invalid expression\n");
            break;
        case '&':
            tree = new_tree(ADDROF, cmp->file->line);
            lex(cmp);
            tree->unary.base = _prs_prefix(cmp);
            if(tree->unary.base && tree->unary.base->kind == POSTFIX || tree->unary.base->kind == PREFIX)
                error(cmp, 0, "Invalid expression\n");
            break;
        case '*':
            tree = _prs_unary(cmp, DEREFERENCE);
            break;
        case '-':
            tree = _prs_unary(cmp, MINUS);
            break;
        case '+':
            tree = _prs_unary(cmp, MINUS);
            break;
        case '(':
            lex(cmp);
            tree = prs_expr(cmp, 0);
            accept(cmp, ')');
            break;
        case KW_SIZEOF:
            tree = _prs_sizeof(cmp);
            break;
        case TK_FLOAT:
            tree             = new_tree(LITERAL, cmp->file->line);
            tree->misc       = new_misc(MISC_CONSTANT_FLOAT);
            tree->type       = cmp_primitives[FLOAT];
            tree->misc->fval = cmp->misc.fval;
            lex(cmp);
            break;
        case TK_DOUBLE:
            tree             = new_tree(LITERAL, cmp->file->line);
            tree->misc       = new_misc(MISC_CONSTANT_FLOAT);
            tree->type       = cmp_primitives[DOUBLE];
            tree->misc->dval = cmp->misc.dval;
            lex(cmp);
            break;
        case '!':
            tree = _prs_unary(cmp, NOT);
            break;
        case '~':
            tree = _prs_unary(cmp, NEGATION);
            break;
        default:
            error(cmp, 0, "Expression expected\n");
            lex(cmp);
            break;
    }

    return _prs_postfix(cmp, tree);
}

static CNode *_prs_sizeof(CCompiler *cmp)
{
    CNode      *tree   = NULL;
    const char *name   = NULL;
    int         sclass = 0;
    int         typeq  = 0;

    if(!cmp)
        return NULL;

    tree = new_tree(SIZEOF, cmp->file->line);

    lex(cmp);

    if(cmp->token != '(') {
        tree->_sizeof.expr = prs_expr(cmp, 0);
        return tree;
    }

    accept(cmp, '(');

    if(!is_typename(cmp) && !is_typequalifier(cmp)) {
        tree->_sizeof.expr = prs_expr(cmp, 0);
        accept(cmp, ')');
        return tree;
    }
    cmp->flags |= COMPILER_FLAG_DONT_NEED_ID;
    tree->_sizeof.type = prs_decl_lvl0(cmp, &sclass, &typeq);
    tree->_sizeof.type = prs_decl_lvl1(cmp, tree->_sizeof.type, &name);
    cmp->flags &= ~COMPILER_FLAG_DONT_NEED_ID;

    accept(cmp, ')');

    if(name)
        error(cmp, 0, "Cannot have a name here\n");

    if(sclass)
        error(cmp, 0, "Cannot specify a storage class here\n");

    return tree;
}

static CNode *_prs_postfix(CCompiler *cmp, CNode *base)
{
    CNode *left;
    CNode *tmp;

    if(!cmp || !base)
        return NULL;

    left = base;

    while(*cmp->file->src) {
        switch(cmp->token) {
            case TK_PP:
            case TK_MM:
                if(left->kind == POSTFIX || left->kind == PREFIX)
                    error(cmp, 0, "Invalid expression\n");
                tmp = new_tree(POSTFIX, cmp->file->line);
                tmp->unary.base = left;
                tmp->unary.op   = cmp->token;
                left            = tmp;
                break;
            case '(':
                left = _prs_fn_call(cmp, left);
                break;
            case '[':
                left = _prs_array_access(cmp, left);
                break;
            case '.':
                left = _prs_member_access(cmp, left);
                break;
            case TK_ARROW:
                left = _prs_member_ptr_access(cmp, left);
                break;
            default:
                return left;
        }
        lex(cmp);
    }
}

static CNode *_prs_fn_call(CCompiler *cmp, CNode *base)
{
    CNode *tree;
    CNode **ptr;
    
    if(!cmp || !base)
        return NULL;

    tree = new_tree(FNCALL, cmp->file->line);

    tree->fncall.base = base;
    ptr = &tree->fncall.args;

    while(*cmp->file->src && lex(cmp) != ')') {
        tree->fncall.count++;
        *ptr = prs_expr(cmp, 0);
        if(*ptr)
            ptr = &(*ptr)->next_stmt;
        if(cmp->token != ',')
            break;
    }

    return tree;
}

static CNode *_prs_array_access(CCompiler *cmp, CNode *base)
{
    CNode *tree;

    if(!cmp || !base)
        return NULL;

    tree = new_tree(ARRAY_ACCESS, cmp->file->line);

    lex(cmp);

    tree->unary.base = prs_expr(cmp, 0);

    expect(cmp, ']');

    return tree;
}

static CNode *_prs_member_access(CCompiler *cmp, CNode *base)
{
    CNode *tree;

    if(!cmp || !base)
        return NULL;

    tree = new_tree(MEMBER_ACCESS, cmp->file->line);

    tree->member_access.base = base;

    lex(cmp);

    expect(cmp, TK_ID);

    tree->member_access.member = new_tree(IDENTIFIER, cmp->file->line);

    tree->member_access.member->misc = new_misc(MISC_ID);

    tree->member_access.member->misc->str = cmp->misc.str;

    return tree;
}

static CNode *_prs_member_ptr_access(CCompiler *cmp, CNode *base)
{
    CNode *tree;

    if(!cmp || !base)
        return NULL;

    tree = new_tree(MEMBER_PTR_ACCESS, cmp->file->line);

    lex(cmp);

    expect(cmp, TK_ID);

    tree->member_access.member            = new_tree(IDENTIFIER, cmp->file->line);
    tree->member_access.member->misc      = new_misc(MISC_ID);
    tree->member_access.member->misc->str = cmp->misc.str;

    return tree;
}

static CNode *_prs_unary(CCompiler *cmp, TreeKind kind)
{
    CNode *tree;

    tree = new_tree(kind, cmp->file->line);
    lex(cmp);
    tree->unary.base = _prs_prefix(cmp);

    return tree;
}
