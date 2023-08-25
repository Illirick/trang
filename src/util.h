#ifndef UTIL_H_
#define UTIL_H_

#define DA_INIT_CAP 128

#define DA_APPEND(da, item) do {\
    if ((da)->count >= (da)->capacity) {\
        if ((da)->capacity == 0) {\
            (da)->capacity = DA_INIT_CAP;\
            (da)->items = malloc(DA_INIT_CAP * sizeof(*(da)->items));\
        } else {\
            (da)->capacity *= 2;\
            (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));\
        }\
        assert((da)->items != NULL);\
    }\
    (da)->items[(da)->count++] = item;\
} while (0)

#define DA(type) \
typedef struct {\
    type *items;\
    size_t count;\
    size_t capacity;\
} type##s;

#endif
