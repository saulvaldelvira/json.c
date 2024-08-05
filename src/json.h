#ifndef _JSON_H_
#define _JSON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct json json;

typedef struct pair {
        char *key;
        json *val;
} pair;

typedef struct json_object {
        pair *elems;
       unsigned long elems_len;
} json_object;

typedef struct json_array {
        json *elems;
        unsigned int len;
} json_array;

enum {
        JSON_OBJECT, JSON_ARRAY, JSON_STRING,
        JSON_NUMBER, JSON_TRUE, JSON_FALSE, JSON_NULL,
        JSON_ERROR
};
struct json {
        int type;
        union {
                json_object object;
                json_array array;
                char *string;
                double number;
                int error_code;
        };
};

typedef struct json_options {
        int max_depth;
        bool recover_errors;
} json_options;

json json_deserialize(char *text);
json json_deserialize_with_options(char *text, struct json_options opts);

void json_print(json j);

void json_free(json j);

enum json_error {
        JSON_ERROR_ERROR = -0xE001,
        JSON_ERROR_MAX_RECURSION = -0xE002,
};

const char* json_get_error_msg(int code);

#endif
