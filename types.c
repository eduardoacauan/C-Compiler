#include "compiler.h"
#include "misc.h"

#define TYPE(kind, name, size) &(CType){name, kind, NULL, size, 0, NULL}

CType *cmp_primitives[END_PRIMITIVES] = {
    TYPE(VOID,  "void",         0),  TYPE(CHAR,   "char",           1), TYPE(UCHAR,   "unsigned char", 1),
    TYPE(SHORT, "short",        2),  TYPE(USHORT, "unsigned short", 2), TYPE(INT,     "int",           4),
    TYPE(UINT,  "unsigned int", 4),  TYPE(LONG,   "long",           8), TYPE(ULONG,   "unsigned long", 8),
    TYPE(FLOAT, "float",        4),  TYPE(DOUBLE, "double",         8), TYPE(LDOUBLE, "long double",   16)
};
