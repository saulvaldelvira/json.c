#ifndef _LEXER_H_
#define _LEXER_H_

enum {
        LBRACE, RBRACE, LSQUAREB, RSQUAREB,
        NUMBER, STRING, COMMA, COLON, KEYWORD,
        DOT, EOF
};

typedef struct token {
        unsigned start, end;
        int type;
} token;

token* tokenize(char *text);
char* get_type_repr(int type);

#endif
