#pragma once
#include <stdint.h>

#define KEYWORD  (0x100)
#define TK_ERROR  -1
#define TK_EOF     0
#define TK_INT     1
#define TK_LONG    2 
#define TK_ID      3
#define TK_SHL     4
#define TK_SHR     5
#define TK_GE      6
#define TK_LE      7
#define TK_ANDAND  8
#define TK_OROR    9
#define TK_ADD_EQ  10
#define TK_SUB_EQ  11
#define TK_MUL_EQ  12
#define TK_DIV_EQ  13
#define TK_SHL_EQ  14
#define TK_SHR_EQ  15
#define TK_AND_EQ  16
#define TK_XOR_EQ  17
#define TK_OR_EQ   18
#define TK_FLOAT   19
#define TK_DOUBLE  20
#define TK_EQ_EQ   21
#define TK_NOT_EQ  22
#define TK_PP      23
#define TK_MM      24
#define TK_ARROW   25
#define TK_ELIPSIS 26
#define TK_LDOUBLE 27
#define TK_MOD_EQ  28

#define KW_WHILE    (KEYWORD + 0)
#define KW_CONTINUE (KEYWORD + 1)
#define KW_DO       (KEYWORD + 2)
#define KW_IF       (KEYWORD + 3)
#define KW_ELSE     (KEYWORD + 4)
#define KW_FOR      (KEYWORD + 5)
#define KW_RETURN   (KEYWORD + 6)
#define KW_BREAK    (KEYWORD + 7)
#define KW_GOTO     (KEYWORD + 8)
#define KW_INT      (KEYWORD + 9)
#define KW_CHAR     (KEYWORD + 10)
#define KW_SHORT    (KEYWORD + 11)
#define KW_LONG     (KEYWORD + 12)
#define KW_SIGNED   (KEYWORD + 13)
#define KW_UNSIGNED (KEYWORD + 14)
#define KW_STATIC   (KEYWORD + 15)
#define KW_VOLATILE (KEYWORD + 16)
#define KW_CONST    (KEYWORD + 17)
#define KW_REGISTER (KEYWORD + 18)
#define KW_AUTO     (KEYWORD + 19)
#define KW_EXTERN   (KEYWORD + 20)
#define KW_STRUCT   (KEYWORD + 21)
#define KW_ENUM     (KEYWORD + 22)
#define KW_FLOAT    (KEYWORD + 23)
#define KW_DOUBLE   (KEYWORD + 24)
#define KW_INLINE   (KEYWORD + 25)
#define KW_RESTRICT (KEYWORD + 26)
#define KW_SWITCH   (KEYWORD + 27)
#define KW_CASE     (KEYWORD + 28)
#define KW_DEFAULT  (KEYWORD + 29)
#define KW_SIZEOF   (KEYWORD + 30)
#define KW_VOID     (KEYWORD + 31)
#define KW_TYPEDEF  (KEYWORD + 32)
#define KW_UNION    (KEYWORD + 33)
#define KW_EDCCTRC  (KEYWORD + 34)

#define MASK_VOID     (1 << 2)
#define MASK_CHAR     (1 << 4)
#define MASK_SHORT    (1 << 6)
#define MASK_INT      (1 << 8)
#define MASK_LONG     (1 << 10)
#define MASK_STRUCT   (1 << 12)
#define MASK_OTHER    (1 << 14)
#define MASK_SIGNED   (1 << 18)
#define MASK_UNSIGNED (1 << 20)
#define MASK_FLOAT    (1 << 21)
#define MASK_DOUBLE   (1 << 23)
#define MASK_UNION    (1 << 25)
#define MASK_ENUM     (1 << 27)

typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t dword;

typedef enum TreeKind    TreeKind;
typedef enum TypeKind    TypeKind;
typedef enum MiscKind    MiscKind;
typedef enum Instruction Instruction;

enum TypeKind {
    VOID,
    CHAR,
    UCHAR,
    SHORT,
    USHORT,
    INT,
    UINT,
    LONG,
    ULONG,
    FLOAT,
    DOUBLE,
    LDOUBLE,
    END_PRIMITIVES,
    PTR,
    STRUCT,
    FUNCTION,
    UNION,
    ENUM,
    ARRAY,
    EMPTY
};

enum MiscKind {
    MISC_CONSTANT_INT,
    MISC_CONSTANT_FLOAT,
    MISC_ID,
    MISC_SYMBOL,
    MISC_LABEL,
    MISC_VREG
};

enum TreeKind {
    IDENTIFIER,
    VARDECL,
    FNPROTO,
    FNDECL,
    LITERAL,
    LABEL,
    GOTO,
    BREAK,
    CONTINUE,
    WHILE,
    DO_WHILE,
    FOR,
    IF,
    RETURN,
    SWITCH,
    CASE,
    DEFAULT,
    BINARYEXPR,
    ASSIGN,
    MEMBER_PTR_ACCESS,
    MEMBER_ACCESS,
    POSTFIX,
    PREFIX,
    ADDROF,
    PLUS,
    MINUS,
    NOT,
    BLOCK,
    FNCALL,
    ARRAY_ACCESS,
    DEREFERENCE,
    SIZEOF,
    TYPECAST,
    NEGATION
};

enum Instruction {
    INS_LOAD,
    INS_STORE,
    INS_JMPZ,
    INS_ADD,
    INS_SUB,
    INS_MUL,
    INS_DIV,
    INS_ENTER,
    INS_LEAVE,
    INS_SHL,
    INS_SHR,
    INS_GE,
    INS_LE,
    INS_GT,
    INS_LT,
    INS_JMP,
    INS_LABEL,
    INS_RETVAL,
    INS_RET,
    INS_AND,
    INS_END_MARK
};
