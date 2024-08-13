#ifndef _JSON_H_
#define _JSON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct json json_t;

typedef struct pair {
        char *key;
        json_t *val;
} pair;

typedef struct json_object {
        pair *elems;
       unsigned long elems_len;
} json_object_t;

typedef struct json_array {
        json_t *elems;
        unsigned int len;
} json_array_t;

enum {
        JSON_OBJECT, JSON_ARRAY, JSON_STRING,
        JSON_NUMBER, JSON_TRUE, JSON_FALSE, JSON_NULL,
        /** Indicates failure in the JSON parsing. */
        JSON_ERROR
};

typedef struct json {
        int type;
        union {
                json_object_t object;
                json_array_t array;
                char *string;
                double number;
                int error_code;
        };
} json_t;

typedef struct json_options {
        int max_depth;
        bool recover_errors;
        FILE *debug_output_file;
} json_options_t;

/**
 * Deserializes the given input into a json structure
 */
json_t json_deserialize(char *text);
json_t json_deserialize_with_options(char *text, json_options_t opts);

/**
 * Validates that the given JSON input is valid.
 * This is the same as calling json_deserialize and then checking
 * if the returned value is of type JSON_ERROR, but skips any unnecesary
 * memory allocations.
 *
 * NOTE: This function is only recommended when you do NOT need the actual
 * JSON output. The lexer does allocate memory.
 *
 * So doing something like
 * ```c
 * if (!json_validate(text))
 *      return -1;
 * json j = json_deserialize(text);
 * ```
 *
 * is probably worse than
 * ```
 * json j = json_deserialize(text);
 * if (j.type == JSON_ERROR)
 *      return -1;
 * ```
 */
bool json_validate(char *text);

/**
 * Prints the json structure
 */
void json_print(json_t j);

/**
 * Frees all memory associated with this json structure
 */
void json_free(json_t j);

typedef enum json_error {
        JSON_ERROR_ERROR = -0xE001,
        JSON_ERROR_MAX_RECURSION = -0xE002,
        JSON_ERROR_UNEXPECTED_TOKEN = -0xE003,
        JSON_ERROR_UNKNOWN_KEYWORD = -0xE004,
} json_error_t;

/**
 * Returns a description of the given error code
 */
const char* json_get_error_msg(int code);

#endif
