#ifndef PARSR_H_
#define PARSER_H_

#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include "lexer.h"
#include "audio.h"

#define DA_INIT_CAP 128

#define DA_APPEND(da, item) do {\
    if ((da).count >= (da).capacity) {\
        if ((da).capacity == 0) {\
            (da).capacity = DA_INIT_CAP;\
            (da).items = malloc(DA_INIT_CAP);\
        } else {\
            (da).capacity *= 2;\
            (da).items = realloc((da).items, (da).capacity * sizeof(*(da).items));\
        }\
        assert((da).items != NULL);\
    }\
    (da).items[(da).count++] = item;\
} while (0)

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} Tokens;

typedef struct {
    Tokens *items;
    size_t count;
    size_t capacity;
} Args;

typedef enum {
    UnknownFunction,
    Load,
    Play,
    FUNC_COUNT,
} Func;

Func str_to_func(const char *str);
Args parse_args(Lexer *l);
void parse_block(Lexer *l);
void parse(const char *filepath);

#endif
