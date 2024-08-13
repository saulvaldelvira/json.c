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
        bool validate;
};

#define _self struct parser *self

static INLINE bool is_finished(_self) { return self->toks[self->curr].type == EOF || self->n_errors > 0; }

static void error (_self, FILE *f, const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
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

static bool expect(_self, int type, int line) {
        if (!match_next(self,type)) {
                FILE *out = self->opts.debug_output_file;
                if (out)
                        error(self, out, "[%d] Expected %s found %s\n", line, get_type_repr(type), get_type_repr(self->toks[self->curr].type));
                return false;
        }
        return true;
}

#define EXPECT(_t) do { if (! expect(self, _t, __LINE__) ) { CLEANUP() ; return ERROR(JSON_ERROR_UNEXPECTED_TOKEN); } } while (0)

static json_t value(_self);
static json_t array(_self);
static json_t number(_self);
static json_t object(_self);
static json_t string(_self);
static json_t keyword(_self);

json_t parse(char *_txt, token *_toks, struct json_options opts, bool validate) {
        struct parser parser = {
                .text = _txt,
                .toks = _toks,
                .opts = opts,
                .validate = validate,
        };
        json_t json = value(&parser);
        if (parser.n_errors > 0)
                json.type = JSON_ERROR;
        return json;
}

static json_t value(_self) {
        if (self->depth > self->opts.max_depth) {
                return (json_t) { .type = JSON_ERROR,
                                .error_code = JSON_ERROR_MAX_RECURSION };
        }
        if (match_next(self,LSQUAREB)) {
                self->depth++;
                json_t j = array(self);
                self->depth--;
                return j;
        }
        if (match_next(self,NUMBER))
                return number(self);
        if (match_next(self,LBRACE)) {
                self->depth++;
                json_t j = object(self);
                self->depth--;
                return j;
        }
        if (match_next(self,STRING))
                return string(self);
        if (match_next(self,KEYWORD))
                return keyword(self);
        return (json_t) {
                .type = JSON_ERROR,
                .error_code = JSON_ERROR_UNEXPECTED_TOKEN,
        };
}

ARR_DEF(json_t);

#define ERROR(code) (json_t) { .type = JSON_ERROR, .error_code = code }

static json_t array(_self) {
        __json_t_array arr = {0};

#define CLEANUP() \
                for (unsigned long i = 0; i < arr.curr; i++) \
                        json_free(arr.elems[i]); \
                free(arr.elems);

        bool start = true;
        while (!(peek_type(self, RSQUAREB) || is_finished(self))) {
                if (!start)
                        EXPECT(COMMA);
                start = false;

                if (peek_type(self, RSQUAREB) && self->opts.recover_errors)
                        continue;

                json_t j = value(self);
                if (j.type == JSON_ERROR) {
                        CLEANUP();
                        return j;
                }
                if (self->validate)
                        continue;

                ARR_PUSH(arr, j);
        }

        EXPECT(RSQUAREB);

        json_t *elems = ARR_SHINK_TO_FIT(arr);

        return (json_t) {
                .type = JSON_ARRAY,
                .array = (json_array_t) {
                        .elems = elems,
                        .len = arr.curr,
                }
        };

#undef CLEANUP
}

static json_t number(_self) {
        char *t = self->text + prev(self).start;
        double d = atof(t);
        return (json_t) {
                .type = JSON_NUMBER,
                .number = d
        };
}

 char* unwrap_str(_self, token t) {
         if (self->validate)
                 return NULL;
        size_t len = t.end - t.start - 2;
        char *str = malloc(len + 1);

        #define escp(c,rep) case c: \
                *dst = rep; \
                break;

        char *dst = str;
        char *src = self->text + t.start + 1;
        for (size_t i = 0; i < len; i++) {
                if (*src == '\\') {
                        src++;
                        i++;
                        switch (*src) {
                        escp('n', '\n')
                        escp('r', '\r')
                        escp('b', '\b')
                        escp('f', '\f')
                        escp('"', '\"')
                        escp('\'', '\'')
                        escp('\\', '\\')
                        }
                } else {
                        *dst = *src;
                }
                dst++;
                src++;
        }
        *dst = '\0';
        return str;
}

static json_t string(_self) {
        token t = prev(self);
        return (json_t) {
                .type = JSON_STRING,
                .string = unwrap_str(self,t)
        };
}

ARR_DEF(pair);

static json_t object(_self) {
        __pair_array arr = {0};

#define CLEANUP() \
                for (unsigned long i = 0; i < arr.curr; i++) { \
                        json_free(*arr.elems[i].val); \
                        free(arr.elems[i].key); \
                        free(arr.elems[i].val); \
                } \
                free(arr.elems);

        bool start = true;
        while (!peek_type(self,RBRACE) && !is_finished(self)) {
                if (!start) {
                        EXPECT(COMMA);
                }
                start = false;

                if (peek_type(self, RBRACE) && self->opts.recover_errors)
                        continue;

                EXPECT(STRING);
                token key = prev(self);
                EXPECT(COLON);
                json_t j = value(self);
                if (j.type == JSON_ERROR) {
                        CLEANUP();
                        return j;
                }

                if (self->validate)
                        continue;

                json_t *val = malloc(sizeof(json_t));
                *val = j;

                pair p = {
                        .key = unwrap_str(self,key),
                        .val = val
                };
                ARR_PUSH(arr, p);
        }

        EXPECT(RBRACE);

        ARR_SHINK_TO_FIT(arr);

        return (json_t) {
                .type = JSON_OBJECT,
                .object = (json_object_t) {
                        .elems = arr.elems,
                        .elems_len = arr.curr
               }
        };

#undef CLEANUP
}

static json_t keyword(_self) {
        token t = prev(self);
        char *txt = self->text + t.start;
        size_t len = t.end - t.start;
#define cmp(w, val) if ( strncmp(txt, w, len) == 0 ) \
        return (json_t) { .type =  val };

        cmp("true", JSON_TRUE);
        cmp("false", JSON_FALSE);
        cmp("null", JSON_NULL);

#undef cmp

        FILE *f = self->opts.debug_output_file;
        if (f)
                error(self, f, "Unknown keyword: %*s\n", t.end - t.start, txt);
        return ERROR(JSON_ERROR_UNKNOWN_KEYWORD);
}

