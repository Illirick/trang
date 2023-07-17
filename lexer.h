#ifndef LEXER_H_
#define LEXER_H_

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#define WORD_MAX_SZ 64
#define STR_MAX_SZ 256

#define BUF_SZ 1024

#define BUF_EOF(buf) ((buf)->pos >= (buf)->size)

typedef struct {
    char *data;
    size_t size;
    size_t pos;
} Buffer;

typedef enum {
    TT_EOF,
    TT_EOL,
    TT_INVALID,
    TT_WORD,
    TT_STRLIT,
    TT_EQ,
    TT_OB,
    TT_CB,
    TT_OCB,
    TT_CCB,
    TT_COMMA,
    TT_COUNT,
} TokenType;

typedef struct {
    TokenType type;
    char *value;
} Token;

char* printable_value(const Token *t);
void readfile(FILE *f, Buffer *buf);
char buf_peek(const Buffer *buf);
char buf_getc(FILE *f, Buffer *buf);
char buf_nextc(FILE *f, Buffer *buf);
bool skipws(FILE *f, Buffer *buf);
bool readword(FILE *f, Buffer *buf, char word[WORD_MAX_SZ]);
bool readstrlit(FILE *f, Buffer *buf, char str[STR_MAX_SZ]);
Token lex_next(FILE *f, Buffer *buf);
void lex_expect(FILE *f, Buffer *buf, TokenType t);

#endif
