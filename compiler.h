// by Eduardo Acauan
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "misc.h"
#include <assert.h>
#include <string.h>
#include <limits.h>

/**************************************************
*      C Compiler Memory MAP                      *
*                                                 *
* ARENA 1 -----> Miscellaneous(atoms, tables...)  *
* ARENA 2 -----> Abstract Syntax Tree             *
* ARENA 3 -----> Intermediate Code Representation *
* ARENA 4 -----> Files Buffer area                *
* ARENA 5 -----> RAW Machine code (x86_64)        *
***************************************************/

#define ARENA_DEFAULT_SIZE (8 * 1024) // 8 KB
                                
#define ARENA_1 0                  
#define ARENA_2 1
#define ARENA_3 2
#define ARENA_4 3
#define ARENA_5 4

#define MAX_ARENAS 5

#define MAX_KEYS   33
#define ASCII_MAX  0x7F

#define SYMBOLS  0
#define TYPEDEFS 1
#define STRUCTS  2
#define UNIONS   3
#define ENUMS    4
#define LABELS   5
#define MAX_TABLES 6

#define COMPILER_FLAG_ERROR           (1 << 0)
#define COMPILER_FLAG_DONT_LEX        (1 << 1)
#define COMPILER_FLAG_DONT_NEED_ID    (1 << 2)
#define COMPILER_FLAG_DONT_PUSH_SCOPE (1 << 3)
#define COMPILER_FLAG_GLOBAL_SCOPE    (1 << 4)
#define COMPILER_FLAG_LOCAL_SCOPE     (1 << 5)

#define SYMBOL_HAS_BEEN_PROTOTYPED    (1 << 0)
#define SYMBOL_HAS_BEEN_INITIALIZED   (1 << 1)

typedef union   UAlign       UAlign;
typedef struct  CFile        CFile;
typedef struct  CType        CType;
typedef struct  CSymbol      CSymbol;
typedef struct  CParameter   CParameter;
typedef struct  CMisc        CMisc;
typedef struct  CSymbolTable CSymbolTable;
typedef struct  CEntry       CEntry;
typedef struct  CKeyword     CKeyword;
typedef struct  CCompiler    CCompiler;
typedef struct  CNode        CNode;
typedef struct  CInstruction CInstruction;
typedef struct  CVirtualReg  CVirtualReg;
typedef struct  CLabel       CLabel;

struct CMisc {
    MiscKind kind;
    CSymbol    *sym;
    union {
        const char  *str;
        int64_t      val;
        float        fval;
        double       dval;
        CType       *type;
        CVirtualReg *vreg;
        CLabel      *label;
    };
};

struct CVirtualReg {
    size_t usage;
    size_t id;
    int    reg;
};

struct CLabel {
    const char *opt_name;
    size_t      address;
    size_t      lbID;
};

struct CCompiler {
    CSymbolTable  *tables[MAX_TABLES];
    CFile         *file;
    CMisc          misc;
    int            token;
    int            flags;
    CNode         *nodes;
    size_t         switch_count;
    size_t         loop_count;
    size_t         label_count;
    CInstruction  *head;
    CInstruction  *tail;
};

struct CInstruction {
    Instruction   kind;
    size_t        line;
    CType        *type;
    CMisc        *arg1;
    CMisc        *arg2;
    CMisc        *arg3;
    CInstruction *prev;
    CInstruction *next;
};

struct CEntry {
    void       *data;
    const char *key;
    CEntry     *prev;
};

struct CSymbolTable {
    CSymbolTable *prev;
    CEntry       *buckets[0x100];
    size_t        item_count;
};

struct CParameter {
    CSymbol    *sym;
    CType      *type;
    CParameter *next;
};

struct CSymbol {
    const char *name;
    CType      *type;
    int         offset;
    int         scope;
    size_t      usage;
    int         flags;
};

struct CType {
    const char *name;
    TypeKind    kind;
    CType      *base;
    size_t      size;
    size_t      param_count;
    union {
        CParameter   *params;
        CSymbolTable *members;
        CNode        *array_dimension;
    };
};

struct CFile {
    const char *path;
    char       *src;
    size_t      line;
    size_t      fsize;
    size_t      errors;
    size_t      warnings;
    CFile      *prev;
};

struct CKeyword {
    const char *keyname;
    int         token;
};

union UAlign {
    void  *p;
    long   i;
    double d;
    void  (*f)(void);
};

struct CNode {
    TreeKind kind;
    CType   *type;
    size_t   line;

    union {
        CMisc *misc;

        struct {
            CNode *lhs;
            CNode *rhs;
            int    op;
        }bin;

        struct {
            CNode *cond;
            CNode *then;
            CNode *_else;
        }_if;

        struct {
            CSymbol *symbol;
            CNode   *init;
        }decl;

        struct {
            CNode *base;
            int    op;
        }unary;

        struct {
            CNode *cond;
            CNode *then;
        }_while;

        struct {
            CNode *init;
            CNode *cond;
            CNode *step;
            CNode *then;
        }_for;

        struct {
            CNode *cond;
            CNode *cases;
        }_switch;

        struct {
            CNode *cond;
            CNode *head;
        }_case;

        struct {
            CNode *expr;
        }ret;

        struct {
            CNode *base;
            CNode *args;
            size_t count;
        }fncall;

        struct {
            CNode *base;
            CNode *member;
        }member_access;

        struct {
            CType *type;
            CNode *expr;
        }_sizeof;

        struct {
            CType *to;
        }_typecast;

        struct {
            CNode *head;
        }blk;
    };
    CNode *next;
    CNode *next_stmt;
};

extern CType       *cmp_primitives[END_PRIMITIVES];
extern CKeyword     keywords[MAX_KEYS];
extern byte         opTable[ASCII_MAX];

//zalloc.c
extern void        *zalloc(size_t nbytes, size_t idx);
extern void         zfree(void);
//misc.c
extern size_t       get_align(size_t size);
extern bool         istrcmp(const char *s1, const char *s2);
extern void         error(CCompiler *cmp, size_t opt_line, const char *msg, ...);
extern void         warn(CCompiler  *cmp, size_t opt_line, const char *msg, ...);
extern void         print_type(CType *type);
extern CType       *make_ptr(CType *base);
extern CType       *new_type(void);
extern CType       *new_function(CType *base, CParameter *params, size_t param_count);
extern CType       *new_array(CType *base, CNode *size_expr);
extern CParameter  *new_param(void);
extern void         expect(CCompiler *cmp, int tokenex);
extern void         accept(CCompiler *cmp, int tokenex);
extern CSymbol     *new_symbol(void);
extern CNode       *new_tree(TreeKind kind, size_t line);
extern CMisc       *new_misc(MiscKind kind);
extern void         printf_type(CType *type, FILE *out);
extern CInstruction *new_instruction(Instruction kind, CMisc *arg1, CMisc *arg2, CMisc *arg3, CType *type, size_t line);
extern void         add_ir(CCompiler *cmp, CInstruction *ins);
extern CVirtualReg *new_virtual_register(size_t reg_count);
extern CLabel      *new_label(const char *opt_name, size_t id);
//file.c
extern CFile        *new_file(const char *path, bool check_ext);
//atom.c
extern const char   *atom(const char *string);
extern const char   *atom_range(const char *string, size_t len);
//table.c
extern CSymbolTable *new_table(void);
extern void          insert(CSymbolTable *table, const char *key, void *data);
extern void         *get(CSymbolTable *table, const char *key);
extern void         *get_local(CSymbolTable *table, const char *key);
extern void          enter_scope(CSymbolTable **table);
extern void          clear_scope(CSymbolTable **table);
//keywords.c
extern void          init_keywords(void);
//boot.c
extern void          boot(int argc, char **argv);
//lexer.c
extern int           lex(CCompiler *cmp);
//decl.c
extern bool          is_typename(CCompiler *cmp);
extern bool          is_typequalifier(CCompiler *cmp);
extern CNode        *prs_decl(CCompiler *cmp);
extern CType        *prs_decl_lvl0(CCompiler *cmp, int *sclass, int *typeq);
extern CType        *prs_decl_lvl1(CCompiler *cmp, CType *base, const char **name);
extern CNode        *prs_translation_unit(CCompiler *cmp);
extern bool          is_storage_class(CCompiler *cmp);
//expr.c
extern CNode        *prs_expr(CCompiler *cmp, int power);
//stmt.c
extern CNode        *prs_stmt(CCompiler *cmp);
//semantic.c
extern void          start_semantic_analyser(CCompiler *cmp);
//irgen.c
extern void          start_irgen(CCompiler *cmp);
//ir_print.c
extern void          print_ir(CCompiler *cmp);
