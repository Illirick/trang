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
    FUNC_UNKNOWN,
    FUNC_LOAD,
    FUNC_PLAY,
    FUNC_ADDPAT,
    FUNC_COUNT,
} Func;

Func strtofunc(const char *str);
Args parse_args(Lexer *l);
void parse_block(Lexer *l, const char *name);
void parse_declaration(Lexer *l, Token t);
void parse(const char *filepath);

#endif
