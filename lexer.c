#include "compiler.h"
#include "misc.h"

#define INVALID 0
#define BLANK   1
#define NEWLINE 2
#define LETTER  4
#define DIGIT   8
#define HEX     16
#define STROPEN 32
#define CHROPEN 64
#define OTHER   128

static int _lex_double(CCompiler *cmp);
static int _lex_hex(CCompiler *cmp);
static int _lex_bin(CCompiler *cmp);

static unsigned char map[] = {
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/BLANK,
    /*null*/NEWLINE,
    /*null*/INVALID,
    /*null*/BLANK,
    /*null*/BLANK,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*null*/INVALID,
    /*' '*/ BLANK,
    /*'!'*/ OTHER,
    /*'"'*/ STROPEN,
    /*'#'*/ OTHER,
    /*'$'*/ INVALID,
    /*'%'*/ OTHER,
    /*'&'*/ OTHER,
    /*'''*/ CHROPEN,
    /*'('*/ OTHER,
    /*')'*/ OTHER,
    /*'*'*/ OTHER,
    /*'+'*/ OTHER,
    /*','*/ OTHER,
    /*'-'*/ OTHER,
    /*'.'*/ OTHER,
    /*'/'*/ OTHER,
    /*'0'*/ DIGIT | HEX,
    /*'1'*/ DIGIT | HEX,
    /*'2'*/ DIGIT | HEX,
    /*'3'*/ DIGIT | HEX,
    /*'4'*/ DIGIT | HEX,
    /*'5'*/ DIGIT | HEX,
    /*'6'*/ DIGIT | HEX,
    /*'7'*/ DIGIT | HEX,
    /*'8'*/ DIGIT | HEX,
    /*'9'*/ DIGIT | HEX,
    /*':'*/ OTHER,
    /*';'*/ OTHER,
    /*'<'*/ OTHER,
    /*'='*/ OTHER,
    /*'>'*/ OTHER,
    /*'?'*/ OTHER,
    /*'@'*/ OTHER,
    /*'A'*/ LETTER | HEX,
    /*'B'*/ LETTER | HEX,
    /*'C'*/ LETTER | HEX,
    /*'D'*/ LETTER | HEX,
    /*'E'*/ LETTER | HEX,
    /*'F'*/ LETTER | HEX,
    /*'G'*/ LETTER,
    /*'H'*/ LETTER,
    /*'I'*/ LETTER,
    /*'J'*/ LETTER,
    /*'K'*/ LETTER,
    /*'L'*/ LETTER,
    /*'M'*/ LETTER,
    /*'N'*/ LETTER,
    /*'O'*/ LETTER,
    /*'P'*/ LETTER,
    /*'Q'*/ LETTER,
    /*'R'*/ LETTER,
    /*'S'*/ LETTER,
    /*'T'*/ LETTER,
    /*'U'*/ LETTER,
    /*'V'*/ LETTER,
    /*'W'*/ LETTER,
    /*'X'*/ LETTER,
    /*'Y'*/ LETTER,
    /*'Z'*/ LETTER,
    /*'['*/ OTHER,
    /*'\'*/ OTHER,
    /*']'*/ OTHER,
    /*'^'*/ OTHER,
    /*'_'*/ LETTER,
    /*'`'*/ OTHER,
    /*'a'*/ LETTER | HEX,
    /*'b'*/ LETTER | HEX,
    /*'c'*/ LETTER | HEX,
    /*'d'*/ LETTER | HEX,
    /*'e'*/ LETTER | HEX,
    /*'f'*/ LETTER | HEX,
    /*'g'*/ LETTER,
    /*'h'*/ LETTER,
    /*'i'*/ LETTER,
    /*'j'*/ LETTER,
    /*'k'*/ LETTER,
    /*'l'*/ LETTER,
    /*'m'*/ LETTER,
    /*'n'*/ LETTER,
    /*'o'*/ LETTER,
    /*'p'*/ LETTER,
    /*'q'*/ LETTER,
    /*'r'*/ LETTER,
    /*'s'*/ LETTER,
    /*'t'*/ LETTER,
    /*'u'*/ LETTER,
    /*'v'*/ LETTER,
    /*'w'*/ LETTER,
    /*'x'*/ LETTER,
    /*'y'*/ LETTER,
    /*'z'*/ LETTER,
    /*'{'*/ OTHER,
    /*'|'*/ OTHER,
    /*'}'*/ OTHER,
    /*'~'*/ OTHER,
};

static void _lex_multiline_comment(CCompiler *cmp);
static int  _lookup_key(CCompiler *cmp);

int lex(CCompiler *cmp)
{
    if(!cmp)
        return TK_EOF;

    if(cmp->flags & COMPILER_FLAG_DONT_LEX) {
        cmp->flags &= ~COMPILER_FLAG_DONT_LEX;
        return cmp->token;
    }

    while(true) {
        while(*cmp->file->src && map[*cmp->file->src] & BLANK)
            cmp->file->src++;

        switch(*cmp->file->src++) {
            case 0:
                cmp->file->src--;
                return cmp->token = TK_EOF;
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case ';':
            case ',':
            case '?':
            case ':':
                return cmp->token = *(cmp->file->src - 1);
            case '.':
                if(*cmp->file->src == '.' && cmp->file->src[1] == '.') { cmp->file->src += 2; return cmp->token = TK_ELIPSIS; }
                if(!(map[*cmp->file->src] & DIGIT))
                    return cmp->token = '.';
                cmp->misc.val = 0;
                cmp->file->src--;
                return _lex_double(cmp);
            case '\n':
                cmp->file->line++;
                continue;
            case '+':
                if(*cmp->file->src == '+') { cmp->file->src++; return cmp->token = TK_PP; }
                if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_ADD_EQ; }
                return cmp->token = '+';
            case '-':
                if(*cmp->file->src == '-') { cmp->file->src++; return cmp->token = TK_MM; }
                if(*cmp->file->src == '>') { cmp->file->src++; return cmp->token = TK_ARROW; }
                if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_SUB_EQ; }
                return cmp->token = '-';
            case '*':
                if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_MUL_EQ; }
                return cmp->token = '*';
            case '&':
                if(*cmp->file->src == '&') { cmp->file->src++; return cmp->token = TK_ANDAND; }
                if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_AND_EQ; }
                return cmp->token = '&';
            case '|':
                if(*cmp->file->src == '|') { cmp->file->src++; return cmp->token = TK_OROR; }
                if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_OR_EQ; }
                return cmp->token = '|';
            case '^':
                if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_XOR_EQ; }
                return cmp->token = '^';
            case '/':
                if(*cmp->file->src == '/') {
                    while(*cmp->file->src && map[*cmp->file->src] ^ NEWLINE)
                        cmp->file->src++;
                    continue;
                }
                if(*cmp->file->src == '*') {_lex_multiline_comment(cmp); continue;}
                if(*cmp->file->src == '=') {cmp->file->src++; return cmp->token = TK_DIV_EQ; }
                return cmp->token = '/';
            case '>':
                if(*cmp->file->src == '>') {
                    cmp->file->src++;
                    if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_SHR_EQ; }
                    return cmp->token = TK_SHR;
                }
                if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_GE; }
                return cmp->token = '>';
            case '<':
                if(*cmp->file->src == '<') {
                    cmp->file->src++;
                    if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_SHL_EQ; }
                    return cmp->token = TK_SHL;
                }
                if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_LE; }
                return cmp->token = '<';
            case '=':
                if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_EQ_EQ; }
                return cmp->token = '=';
            case '!':
                if(*cmp->file->src == '=') { cmp->file->src++; return cmp->token = TK_NOT_EQ; }
                return cmp->token = '!';
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                cmp->file->src--;
                cmp->misc.val = 0;

                if(*cmp->file->src == '0' && (cmp->file->src[1] & 0xDF) == 'X')
                    return _lex_hex(cmp);
                if(*cmp->file->src == '0' && (cmp->file->src[1] & 0xDF) == 'B')
                    return _lex_bin(cmp);

                do
                    cmp->misc.val = cmp->misc.val * 10 + (*cmp->file->src++ & 0xF);
                while (*cmp->file->src && map[*cmp->file->src] & DIGIT);

                if(*cmp->file->src == '.')
                    return _lex_double(cmp);
                
                return cmp->token = TK_INT;
            case '_':
            case 'a':
            case 'A':
            case 'b':
            case 'B':
            case 'c':
            case 'C':
            case 'd':
            case 'D':
            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'h':
            case 'H':
            case 'i':
            case 'I':
            case 'j':
            case 'J':
            case 'k':
            case 'K':
            case 'l':
            case 'L':
            case 'm':
            case 'M':
            case 'n':
            case 'N':
            case 'o':
            case 'O':
            case 'p':
            case 'P':
            case 'q':
            case 'Q':
            case 'r':
            case 'R':
            case 's':
            case 'S':
            case 't':
            case 'T':
            case 'u':
            case 'U':
            case 'v':
            case 'V':
            case 'w':
            case 'W':
            case 'x':
            case 'X':
            case 'y':
            case 'Y':
            case 'z':
            case 'Z':
                cmp->file->src--;
                cmp->misc.str = cmp->file->src;

                while(*cmp->file->src && map[*cmp->file->src] & (LETTER | DIGIT))
                    cmp->file->src++;

                cmp->misc.str = atom_range(cmp->misc.str, cmp->file->src - cmp->misc.str);

                return cmp->token = _lookup_key(cmp);
            default:
                error(cmp, 0, "Invalid token '%c'\n", *(cmp->file->src - 1));
                return cmp->token = TK_ERROR;
        }
    }
}

static void _lex_multiline_comment(CCompiler *cmp)
{
    if(!cmp)
        return;

    do {
        if(map[*cmp->file->src++] & NEWLINE)
            cmp->file->line++;
    }while(*cmp->file->src && (cmp->file->src[0] != '*' || cmp->file->src[1] != '/'));
    
    if(!*cmp->file->src) {
        error(cmp, 0, "Missing '*/'\n");
        return;
    }

    cmp->file->src += 2;
}

static int _lookup_key(CCompiler *cmp)
{
    if(!cmp)
        return 0;

    for(int i = 0; i < MAX_KEYS; i++)
        if(keywords[i].keyname == cmp->misc.str)
            return keywords[i].token;
    return TK_ID;
}

static int _lex_double(CCompiler *cmp)
{
    double value = 0;
    int    exp;
    double constant = 0.1;

    if(!cmp)
        return TK_EOF;

    cmp->file->src++;

    exp = cmp->misc.val;

    do {
        value = value + (*cmp->file->src++ & 0xF) * constant;
        constant *= 0.1;
    }while(*cmp->file->src && map[*cmp->file->src] & DIGIT);

    cmp->misc.dval = value + exp;

    if((*cmp->file->src & 0xDF) == 'F') {
        cmp->file->src++;
        cmp->misc.fval = (float)cmp->misc.dval;
        return cmp->token = TK_FLOAT;
    }
    return cmp->token = TK_DOUBLE;
}

static int _lex_hex(CCompiler *cmp)
{
    cmp->file->src += 2;

    if(!*cmp->file->src) {
        error(cmp, 0, "Hex number expected\n");
        return cmp->token = TK_EOF;
    }

    do {
        if(map[*cmp->file->src] & DIGIT) {
            cmp->misc.val = (cmp->misc.val << 4) | (*cmp->file->src++ & 0xF);
            continue;
        }

        if(map[*cmp->file->src] & HEX) {
            cmp->misc.val = (cmp->misc.val << 4) | ((*cmp->file->src++ & 0xDF) - 'A' + 10);
            continue;
        }

        error(cmp, 0, "Invalid hex digit '%c'\n", *cmp->file->src++);
    }while(*cmp->file->src && map[*cmp->file->src] & (LETTER | DIGIT));

    return cmp->token = TK_INT;
}

static int _lex_bin(CCompiler *cmp)
{
    if(!cmp)
        return TK_EOF;

    cmp->misc.val = 0;

    cmp->file->src += 2;

    if(!*cmp->file->src) {
        error(cmp, 0, "Binary number expected\n");
        return cmp->token = TK_EOF;
    }

    while(*cmp->file->src && map[*cmp->file->src] & (LETTER | DIGIT)) {
        cmp->misc.val <<= 1;
        if(*cmp->file->src == '1') {
            cmp->misc.val |= 1;
            cmp->file->src++;
            continue;
        }
        if(*cmp->file->src == '0') {
            cmp->file->src++;
            continue;
        }

        error(cmp, 0, "Invalid binary number '%c'\n", *cmp->file->src++);
    }

    return cmp->token = TK_INT;
}
