#include "compiler.h"

CKeyword keywords[MAX_KEYS];

#define INSERT(pos, key, token) keywords[pos] = (CKeyword) {key, token}

void init_keywords(void)
{
	INSERT(0, atom("void"),     KW_VOID);
	INSERT(1, atom("char"),     KW_CHAR);
	INSERT(2, atom("short"),    KW_SHORT);
	INSERT(3, atom("int"),      KW_INT);
	INSERT(4, atom("long"),     KW_LONG);
	INSERT(5, atom("signed"),   KW_SIGNED);
	INSERT(6, atom("unsigned"), KW_UNSIGNED);
	INSERT(7, atom("float"),    KW_FLOAT);
	INSERT(8, atom("double"),   KW_DOUBLE);
	INSERT(9, atom("typedef"),  KW_TYPEDEF);
	INSERT(10,atom("static"),   KW_STATIC);
	INSERT(11,atom("extern"),   KW_EXTERN);
	INSERT(12,atom("auto"),     KW_AUTO);
	INSERT(13,atom("register"), KW_REGISTER);
	INSERT(14,atom("struct"),   KW_STRUCT);
	INSERT(15,atom("union"),    KW_UNION);
	INSERT(16,atom("while"),    KW_WHILE);
	INSERT(17,atom("return"),   KW_RETURN);
	INSERT(18,atom("for"),      KW_FOR);
	INSERT(19,atom("do"),       KW_DO);
	INSERT(20,atom("if"),       KW_IF);
	INSERT(21,atom("const"),    KW_CONST);
	INSERT(22,atom("inline"),   KW_INLINE);
	INSERT(23,atom("switch"),   KW_SWITCH);
	INSERT(24,atom("case"),     KW_CASE);
	INSERT(25,atom("default"),  KW_DEFAULT);
	INSERT(26,atom("goto"),     KW_GOTO);
	INSERT(27,atom("break"),    KW_BREAK);
	INSERT(28,atom("continue"), KW_CONTINUE);
	INSERT(29,atom("else"),     KW_ELSE);
	INSERT(30,atom("enum"),     KW_ENUM);
	INSERT(31,atom("volatile"), KW_VOLATILE);
	INSERT(32,atom("sizeof"),   KW_SIZEOF);
	//compiler internal keywords
	INSERT(33,atom("__edcc_trace"),KW_EDCCTRC);
}
