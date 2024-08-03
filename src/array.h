#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <stdlib.h>

#define ARR_DEF(type) \
        typedef struct __ ## type ## _array { \
                type *elems; \
                unsigned long curr, cap; \
        } __ ## type ## _array

#define ARR_PUSH(arr, val) \
        do { \
                if (arr.curr >= arr.cap) { \
                        arr.cap *= 2; \
                        if (arr.cap == 0) arr.cap = 1024; \
                        arr.elems = realloc(arr.elems, arr.cap * sizeof(*arr.elems)); \
                } \
                arr.elems[arr.curr++] = val; \
        } while (0)

#define ARR_SHINK_TO_FIT(arr) \
        ( arr.elems = realloc(arr.elems, arr.curr * sizeof(*arr.elems)) )

#endif
