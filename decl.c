#include "compiler.h"
#include "misc.h"

static CType *_prs_decl_lvl2(CCompiler *cmp, CType *base);
static CType *_prs_array(CCompiler *cmp, CType *base);
static CType *_prs_fun_params(CCompiler *cmp, CType *base);

static CNode *_prs_function(CCompiler *cmp, CSymbol *sym);

CNode *prs_translation_unit(CCompiler *cmp)
{
    CNode *tree;

    if(!cmp)
        return NULL;

    if(is_typename(cmp) || is_typequalifier(cmp) || is_storage_class(cmp))
        return prs_decl(cmp);

    tree = prs_expr(cmp, 0);

    expect(cmp, ';');

    return tree;
}

static CNode *_prs_function(CCompiler *cmp, CSymbol *sym)
{
    CNode *tree;

    if(!cmp || !sym)
        return NULL;

    tree = new_tree(FNPROTO, cmp->file->line);

    tree->decl.symbol = sym;

    if(cmp->token == ';')
        return tree;

    tree->kind = FNDECL;

    expect(cmp, '{');

    tree->decl.init = prs_stmt(cmp);

    return tree;
}

CNode *prs_decl(CCompiler *cmp)
{
    CType      *base   = NULL;
    int         sclass = 0;
    int         typeq  = 0;
    const char *name   = NULL;
    CNode      *decl   = NULL;
    CNode     **ptr    = &decl;

    if(!cmp)
        return NULL;

    base = prs_decl_lvl0(cmp, &sclass, &typeq);

    do {
        CType   *final = prs_decl_lvl1(cmp, base, &name);
        CSymbol *sym;

        if(sclass == KW_TYPEDEF) {
            if(get(cmp->tables[TYPEDEFS], name))
                error(cmp, 0, "Type '%s' already declared\n", name);
            else
                insert(cmp->tables[TYPEDEFS], name, final);
            if(cmp->token != ',')
                break;
            continue;
        }

        if(final->kind == VOID)
            error(cmp, 0, "Cannot declare a variable with type void\n");

        sym = new_symbol();

        sym->name = name;
        sym->type = final;

        if(final->kind == FUNCTION)
            return _prs_function(cmp, sym);

        *ptr = new_tree(VARDECL, cmp->file->line);

        (*ptr)->decl.symbol = sym;

        if (cmp->token == '=') {
            lex(cmp);
            (*ptr)->decl.init = prs_expr(cmp, opTable[',']);
        }

        if(cmp->token != ',')
            break;
        ptr = &(*ptr)->next;
    }while(lex(cmp));

    expect(cmp, ';');

    return decl;
}

static CType *_prs_fun_params(CCompiler *cmp, CType *base)
{
    CParameter  *params  = NULL;
    CParameter **ptr     = &params;
    int          sclass  = 0;
    int          typeq   = 0;
    const char  *name    = NULL;
    size_t       count   = 0;

    if(!cmp || !base)
        return NULL;

    cmp->flags |= COMPILER_FLAG_DONT_NEED_ID;

    while(*cmp->file->src && lex(cmp) != ')') {
        CType *type = prs_decl_lvl0(cmp, &sclass, &typeq);
        type        = prs_decl_lvl1(cmp, type, &name);

        if(sclass)
            error(cmp, 0, "Cannot specify a storage class here\n");

        if(type->kind == VOID) {
            if(!count)
                break;
            error(cmp, 0, "Cannot declare a variable with type void\n");
        }

        *ptr = new_param();

        (*ptr)->type = type;

        count++;

        if(name) {
            (*ptr)->sym       = new_symbol();
            (*ptr)->sym->name = name;
            (*ptr)->sym->type = type;
            name              = NULL;
        }

        if(cmp->token != ',')
            break;

        ptr = &(*ptr)->next;
    }

    cmp->flags &= ~COMPILER_FLAG_DONT_NEED_ID;

    accept(cmp, ')');

    return new_function(base, params, count);
}

static CType *_prs_array(CCompiler *cmp, CType *base)
{
    CType *final = base;

    if(!cmp || !base)
        return NULL;

    do {
        if(lex(cmp) == ']') {
            final = new_array(final, NULL);
            if(lex(cmp) != '[')
                return final;
            continue;
        }
        final = new_array(final, prs_expr(cmp, 0));
        accept(cmp, ']');
        if(cmp->token != '[')
            break;
    }while(*cmp->file->src);

    return final;    
}

static CType *_prs_decl_lvl2(CCompiler *cmp, CType *base)
{
    if(!cmp || !base)
        return NULL;

    if(cmp->token == '(')
        return _prs_fun_params(cmp, base);
    if(cmp->token == '[')
        return _prs_array(cmp, base);

    return base;
}

CType *prs_decl_lvl1(CCompiler *cmp, CType *base, const char **name)
{
    CType *final = base;
    size_t stars = 0;
    
    if(!cmp || !base || !name)
        return NULL;

    while(cmp->token == '*') {
        stars++;
        lex(cmp);
    }

    if(cmp->token == '(') {
        CType *prefix, *suffix, *ptr, *empty_type;

        empty_type       = new_type();
        empty_type->kind = EMPTY;

        lex(cmp);

        prefix = prs_decl_lvl1(cmp, empty_type, name);

        for(; stars; stars--)
            final = make_ptr(final);

        accept(cmp, ')');

        suffix = _prs_decl_lvl2(cmp, final);

        for(ptr = prefix; ptr; ptr = ptr->base) {
            if(ptr->base && ptr->base->kind == EMPTY) {
                ptr->base = suffix;
                break;
            }
        }

        if(!ptr && suffix)
            return suffix;

        return prefix;
    }

    if(cmp->token == TK_ID || !(cmp->flags & COMPILER_FLAG_DONT_NEED_ID)) {
        expect(cmp, TK_ID);
        if(name)
            *name = cmp->misc.str;
        lex(cmp);
    }

    for(; stars; stars--)
        final = make_ptr(final);

    return _prs_decl_lvl2(cmp, final);
}

CType *prs_decl_lvl0(CCompiler *cmp, int *sclass, int *typeq)
{
    const char *usertype = NULL;
    CType      *type     = cmp_primitives[INT];
    int         sum      = 0;

    if(!cmp || !sclass || !typeq)
        return NULL;


    while(is_typename(cmp) || is_typequalifier(cmp) || is_storage_class(cmp)) {
        switch(cmp->token) {
            case KW_AUTO:
            case KW_STATIC:
            case KW_EXTERN:
            case KW_TYPEDEF:
                if(*sclass)
                    error(cmp, 0, "Storage class already specified\n");
                *sclass = cmp->token;
                 break;
            case KW_UNSIGNED:
                sum += MASK_UNSIGNED;
                break;
            case KW_SIGNED:
                sum += MASK_SIGNED;
                break;
            case KW_VOID:
                sum += MASK_VOID;
                break;
            case KW_CHAR:
                sum += MASK_CHAR;
                break;
            case KW_SHORT:
                sum += MASK_SHORT;
                break;
            case KW_INT:
                sum += MASK_INT;
                break;
            case KW_LONG:
                sum += MASK_LONG;
                break;
            case KW_STRUCT:
                sum += MASK_STRUCT;
                break;
            case KW_UNION:
                sum += MASK_UNION;
                break;
            case KW_ENUM:
                sum += MASK_ENUM;
                break;
            case TK_ID:
                sum += MASK_OTHER;
                usertype = cmp->misc.str;
                break;
            case KW_FLOAT:
                sum += MASK_FLOAT;
                break;
            case KW_DOUBLE:
                sum += MASK_DOUBLE;
                break;
        }
        lex(cmp);
    }

    switch(sum) {
        case MASK_VOID:
            type = cmp_primitives[VOID];
            break;
        case MASK_CHAR:
        case MASK_SIGNED + MASK_CHAR:
            type = cmp_primitives[CHAR];
            break;
        case MASK_UNSIGNED + MASK_CHAR:
            type = cmp_primitives[UCHAR];
            break;
        case MASK_SHORT:
        case MASK_SIGNED + MASK_SHORT:
        case MASK_SHORT  + MASK_INT:
        case MASK_SIGNED + MASK_SHORT + MASK_INT:
            type = cmp_primitives[SHORT];
            break;
        case MASK_UNSIGNED + MASK_SHORT:
        case MASK_UNSIGNED + MASK_SHORT + MASK_INT:
            type = cmp_primitives[USHORT];
            break;
        case MASK_SIGNED:
        case MASK_INT:
        case MASK_SIGNED + MASK_INT:
            break;
        case MASK_UNSIGNED:
        case MASK_UNSIGNED + MASK_INT:
            type = cmp_primitives[UINT];
            break;
        case MASK_SIGNED + MASK_LONG:
        case MASK_SIGNED + MASK_LONG + MASK_INT:
        case MASK_SIGNED + MASK_LONG + MASK_LONG + MASK_INT:
        case MASK_LONG:
        case MASK_LONG + MASK_INT:
        case MASK_LONG + MASK_LONG + MASK_INT:
            type = cmp_primitives[LONG];
            break;
        case MASK_UNSIGNED + MASK_LONG:
        case MASK_UNSIGNED + MASK_LONG + MASK_INT:
        case MASK_UNSIGNED + MASK_LONG + MASK_LONG + MASK_INT:
            type = cmp_primitives[ULONG];
            break;
        case MASK_FLOAT:
            type = cmp_primitives[FLOAT];
            break;
        case MASK_DOUBLE:
            type = cmp_primitives[DOUBLE];
            break;
        case MASK_LONG + MASK_DOUBLE:
            type = cmp_primitives[LDOUBLE];
            break;
        case MASK_OTHER:
            if((type = get(cmp->tables[TYPEDEFS], usertype)))
                break;
            error(cmp, 0, "Undefined type '%s'\n", usertype);
            break;
    }

    return type;
}

bool is_typename(CCompiler *cmp)
{
    if(!cmp)
        return false;

    if(cmp->token == TK_ID)
        return get(cmp->tables[TYPEDEFS], cmp->misc.str) ? true : false;

    return cmp->token == KW_UNSIGNED || cmp->token == KW_SIGNED || cmp->token == KW_VOID   ||
           cmp->token == KW_CHAR     || cmp->token == KW_SHORT  || cmp->token == KW_INT    ||
           cmp->token == KW_LONG     || cmp->token == KW_FLOAT  || cmp->token == KW_DOUBLE ||
           cmp->token == KW_STRUCT   || cmp->token == KW_UNION  || cmp->token == KW_ENUM;
}

bool is_typequalifier(CCompiler *cmp)
{
    if(!cmp)
        return false;

    return cmp->token == KW_CONST || cmp->token == KW_REGISTER || cmp->token == KW_VOLATILE || cmp->token == KW_RESTRICT;
}

bool is_storage_class(CCompiler *cmp)
{
    if(!cmp)
        return false;

    return cmp->token == KW_AUTO || cmp->token == KW_STATIC || cmp->token == KW_TYPEDEF || cmp->token == KW_EXTERN;
}
