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

static json value(_self);
static json array(_self);
static json number(_self);
static json object(_self);
static json string(_self);
static json keyword(_self);

json parse(char *_txt, token *_toks, struct json_options opts, bool validate) {
        struct parser parser = {
                .text = _txt,
                .toks = _toks,
                .opts = opts,
                .validate = validate,
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
        return (json) {
                .type = JSON_ERROR,
                .error_code = JSON_ERROR_UNEXPECTED_TOKEN,
        };
}

ARR_DEF(json);

#define ERROR(code) (json) { .type = JSON_ERROR, .error_code = code }

static json array(_self) {
        __json_array arr = {0};

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

                json j = value(self);
                if (j.type == JSON_ERROR) {
                        CLEANUP();
                        return j;
                }
                if (self->validate)
                        continue;

                ARR_PUSH(arr, j);
        }

        EXPECT(RSQUAREB);

        json *elems = ARR_SHINK_TO_FIT(arr);

        return (json) {
                .type = JSON_ARRAY,
                .array = (json_array) {
                        .elems = elems,
                        .len = arr.curr,
                }
        };

#undef CLEANUP
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
         if (self->validate)
                 return NULL;
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
                json j = value(self);
                if (j.type == JSON_ERROR) {
                        CLEANUP();
                        return j;
                }

                if (self->validate)
                        continue;

                json *val = malloc(sizeof(json));
                *val = j;

                pair p = {
                        .key = unwrap_str(self,key),
                        .val = val
                };
                ARR_PUSH(arr, p);
        }

        EXPECT(RBRACE);

        ARR_SHINK_TO_FIT(arr);

        return (json) {
                .type = JSON_OBJECT,
                .object = (json_object) {
                        .elems = arr.elems,
                        .elems_len = arr.curr
               }
        };

#undef CLEANUP
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

        FILE *f = self->opts.debug_output_file;
        if (f)
                error(self, f, "Unknown keyword: %*s\n", t.end - t.start, txt);
        return ERROR(JSON_ERROR_UNKNOWN_KEYWORD);
}

