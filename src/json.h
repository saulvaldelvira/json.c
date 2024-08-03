#ifndef _JSON_H_
#define _JSON_H_

#include <stddef.h>
#include <stdint.h>

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
        };
};

json json_deserialize(char *text, size_t len);

void json_print(json j);

void json_free(json j);

#endif
