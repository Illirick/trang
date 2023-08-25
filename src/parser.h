#ifndef PARSER_H_
#define PARSER_H_

#include "lexer.h"
#include "util.h"

DA(Token);

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
    FUNC_BPM,
    FUNC_COUNT,
} Func;

Func strtofunc(const Token *t);
Args parse_args(Lexer *l);
void parse_block(Lexer *l, const char *name);
void parse_declaration(Lexer *l, const Token *t);
void parse(const char *filepath);

#endif
