#include "../src/json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define assert(expr) do {\
        if (!(expr)) {\
                fprintf(stderr, "[%d] ASSERT FAILED: %s\n", __LINE__, #expr);\
                abort(); \
        } } while(0)

#define stringify(...) # __VA_ARGS__

bool array_cmp(json_array_t arr, int *expected, size_t expected_len) {
        if (arr.len != expected_len)
                return false;
        for (int i = 0; i < arr.len; i++) {

        }
}

void test_deserialize(void) {
        json_t json = json_deserialize(stringify(
                {
                   "list": [12,13,14],
                   "true": true
                }
        ));
        assert(json.type == JSON_OBJECT);

        assert(json.object.elems_len == 2);
        struct pair pair;
        pair = json.object.elems[0];
        assert(strcmp("list", pair.key) == 0);

        pair = json.object.elems[1];
        assert(strcmp("true", pair.key) == 0);

}

int main(void) {
        printf("Testing...");
        fflush(stdout);

        test_deserialize();

        printf("Done\n");
}
