#include "json.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

json json_deserialize(char *text, size_t len) {
        token *tokens = tokenize(text, len);
        json json = parse(text, tokens);
        free(tokens);
        return json;
}

void json_print(json j) {
        switch (j.type) {
        case JSON_ARRAY:
                printf("[");
                for (unsigned long i = 0; i < j.array.len; i++) {
                        json_print(j.array.elems[i]);
                        if (i < j.array.len - 1)
                                printf(", ");
                }
                printf("]");
                break;
        case JSON_NUMBER:
                printf("%f", j.number);
                break;
        case JSON_OBJECT:
                printf("{");
                for (unsigned long i = 0; i < j.object.elems_len; i++) {
                        printf("\"%s\" : ", j.object.elems[i].key);
                        json_print(*j.object.elems[i].val);
                        if (i < j.object.elems_len - 1)
                                printf(", ");
                }
                printf("}");
                break;
        case JSON_STRING:
                printf("\"%s\"", j.string);
                break;
        case JSON_TRUE:
                printf("true");
                break;
        case JSON_FALSE:
                printf("false");
                break;
        case JSON_NULL:
                printf("null");
                break;
        default:
          break;
        }
}

void json_free(json j) {
        switch (j.type) {
        case JSON_ARRAY:
                for (unsigned long i = 0; i < j.array.len; i++)
                        json_free(j.array.elems[i]);
                free(j.array.elems);
                break;
        case JSON_OBJECT:
                for (unsigned long i = 0; i < j.object.elems_len; i++) {
                        json_free(*j.object.elems[i].val);
                        free(j.object.elems[i].key);
                        free(j.object.elems[i].val);
                }
                free(j.object.elems);
                break;
        case JSON_STRING:
                free(j.string);
                break;
        default:
          break;
        }
}
