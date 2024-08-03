#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include "array.h"

ARR_DEF(token);

struct lexer {
        char *text;
        int curr, start, text_len;
        __token_array tokens;
        unsigned int n_errors;
};

#define _self struct lexer *self


static inline bool is_finished(_self) { return self->curr >= self->text_len || self->n_errors > 0; }

static char advance(_self);
static char peek(_self);
static void parse_next(_self);
static void push_token(_self,int type);

static void number(_self);
static void string(_self);
static void keyword(_self);

token* tokenize(char *_text, unsigned long size) {
        struct lexer lexer = {
                .text = _text,
                .text_len = size,
        };
        while (!is_finished(&lexer)) {
                parse_next(&lexer);
        }

        token t = {.type = EOF};
        ARR_PUSH(lexer.tokens, t);

        return ARR_SHINK_TO_FIT(lexer.tokens);
}

static inline void error (_self, const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
        self->n_errors++;
}

static void parse_next(_self) {
        self->start = self->curr;
        int c = advance(self);
        switch (c) {
                case '{': push_token(self,LBRACE); break;
                case '}': push_token(self,RBRACE); break;
                case '[': push_token(self,LSQUAREB); break;
                case ']': push_token(self,RSQUAREB); break;
                case ',': push_token(self,COMMA); break;
                case ':': push_token(self,COLON); break;
                case '.': push_token(self,DOT); break;
                case '"': string(self); break;
                case ' ':
                case '\n':
                case '\r':
                case '\t':
                          break;
                default:
                        if (isdigit(c))
                                number(self);
                        else if (isalpha(c))
                                keyword(self);
                        else
                                error(self, "Unknown: '%c'\n", c);
        }

}

static char advance(_self) {
        if (is_finished(self))
                return '\0';
        return self->text[self->curr++];
}

static char peek(_self) {
        if (is_finished(self))
                return '\0';
        return self->text[self->curr];
}

static void push_token(_self, int type) {
        token t = {
                .type = type,
                .start = self->start,
                .end = self->curr,
        };
        ARR_PUSH(self->tokens, t);
}

static void number(_self) {
        while (!is_finished(self)) {
                if (isdigit(peek(self)))
                        advance(self);
                else break;
        }
        push_token(self,NUMBER);
}

static void string(_self) {
        bool scaping = false;
        for (;;) {
                if (peek(self) == '"' && !scaping)
                        break;
                scaping = peek(self) == '\\';
                advance(self);
        }
        if (peek(self) != '"')
                error(self, "Unclosed string\n");
        else advance(self);
        push_token(self,STRING);
}

static void keyword(_self) {
        while (peek(self) >= 'a' && peek(self) <= 'z')
                advance(self);
        push_token(self,KEYWORD);
}

char* get_type_repr(int type) {
        static struct __repr {
                int t;
                char *s;
        } reprs [] = {
                {LBRACE,"LBRACE"},
                {RBRACE,"RBRACE"},
                {LSQUAREB,"LSQUAREB"},
                {RSQUAREB,"RSQUAREB"},
                {NUMBER,"NUMBER"},
                {STRING,"STRING"},
                {COMMA,"COMMA"},
                {COLON,"COLON"},
                {KEYWORD,"KEYWORD"},
                {DOT,"DOT"},
                {EOF,"EOF"},
                {-1,NULL}
        };

        for (struct __repr *i = reprs; i->s; i++) {
                if (i->t == type)
                        return i->s;
        }
        return NULL;
}
