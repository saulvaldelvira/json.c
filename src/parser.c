#include "parser.h"
#include "json.h"
#include "lexer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include "util.h"

struct parser {
        token *toks;
        char *text;
        int curr;
        unsigned int n_errors;
        struct json_options opts;
        int depth;
};

#define _self struct parser *self

static INLINE bool is_finished(_self) { return self->toks[self->curr].type == EOF || self->n_errors > 0; }

static void error (_self, const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
        self->n_errors++;
}

static INLINE token prev(_self) {
        return self->toks[self->curr - 1];
}

 bool match_next(_self,int type) {
        if (is_finished(self)) return false;
        if (self->toks[self->curr].type == type) {
                self->curr++;
                return true;
        }
        return false;
}

static INLINE bool peek_type(_self,int type) {
        if (is_finished(self)) return false;
        return self->toks[self->curr].type == type;
}

#define expect(_t) do { \
        if (!match_next(self,_t)) \
                error(self,"[%d] Expected %s found %s\n", __LINE__, get_type_repr(_t), get_type_repr(self->toks[self->curr].type)); \
} while (0)

static json value(_self);
static json array(_self);
static json number(_self);
static json object(_self);
static json string(_self);
static json keyword(_self);

json parse(char *_txt, token *_toks, struct json_options opts) {
        struct parser parser = {
                .text = _txt,
                .toks = _toks,
                .opts = opts,
        };
        json json = value(&parser);
        if (parser.n_errors > 0)
                json.type = JSON_ERROR;
        return json;
}

static json value(_self) {
        if (self->depth > self->opts.max_depth) {
                return (json) { .type = JSON_ERROR,
                                .error_code = JSON_ERROR_MAX_RECURSION };
        }
        if (match_next(self,LSQUAREB)) {
                self->depth++;
                json j = array(self);
                self->depth--;
                return j;
        }
        if (match_next(self,NUMBER))
                return number(self);
        if (match_next(self,LBRACE)) {
                self->depth++;
                json j = object(self);
                self->depth--;
                return j;
        }
        if (match_next(self,STRING))
                return string(self);
        if (match_next(self,KEYWORD))
                return keyword(self);
        error(self, "Unknown %d\n", self->toks[self->curr].type);
        return (json) {
                .type = JSON_ERROR
        };
}

ARR_DEF(json);

static json array(_self) {
        __json_array arr = {0};

        bool start = true;
        while (!(peek_type(self, RSQUAREB) || is_finished(self))) {
                if (!start)
                        expect(COMMA);
                start = false;

                json j = value(self);
                if (j.type == JSON_ERROR)
                        return j;
                ARR_PUSH(arr, j);
        }

        expect(RSQUAREB);

        json *elems = ARR_SHINK_TO_FIT(arr);

        return (json) {
                .type = JSON_ARRAY,
                .array = (json_array) {
                        .elems = elems,
                        .len = arr.curr,
                }
        };
}

static json number(_self) {
        char *t = self->text + prev(self).start;
        double d = atof(t);
        return (json) {
                .type = JSON_NUMBER,
                .number = d
        };
}

 char* unwrap_str(_self, token t) {
        size_t len = t.end - t.start - 2;
        char *str = malloc(len + 1);
        strncpy(str, self->text + t.start + 1, len);
        str[len] = '\0';
        return str;
}

static json string(_self) {
        token t = prev(self);
        return (json) {
                .type = JSON_STRING,
                .string = unwrap_str(self,t)
        };
}

ARR_DEF(pair);

static json object(_self) {
        __pair_array arr = {0};

        bool start = true;
        while (!peek_type(self,RBRACE) && !is_finished(self)) {
                if (!start)
                        expect(COMMA);
                start = false;

                expect(STRING);
                token key = prev(self);
                expect(COLON);
                json j = value(self);
                if (j.type == JSON_ERROR)
                        return j;

                json *val = malloc(sizeof(json));
                *val = j;

                pair p = {
                        .key = unwrap_str(self,key),
                        .val = val
                };
                ARR_PUSH(arr, p);
        }

        expect(RBRACE);

        ARR_SHINK_TO_FIT(arr);

        return (json) {
                .type = JSON_OBJECT,
                .object = (json_object) {
                        .elems = arr.elems,
                        .elems_len = arr.curr
               }
        };
}

static json keyword(_self) {
        token t = prev(self);
        char *txt = self->text + t.start;
        size_t len = t.end - t.start;
#define cmp(w, val) if ( strncmp(txt, w, len) == 0 ) \
        return (json) { .type =  val };

        cmp("true", JSON_TRUE);
        cmp("false", JSON_FALSE);
        cmp("null", JSON_NULL);

#undef cmp

        error(self, "Unknown keyword: %*s\n", t.end - t.start, txt);
        return (json){ .type = JSON_ERROR };
}

