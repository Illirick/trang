#ifndef PARSR_H_
#define PARSER_H_

#include "lexer.h"
#include "audio.h"

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
