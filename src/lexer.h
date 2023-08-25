#ifndef LEXER_H_
#define LEXER_H_

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define WORD_MAX_SZ 64
#define STR_MAX_SZ 256

#define BUF_SZ 1024

#define BUF_EOF(buf) ((buf)->pos >= (buf)->size)
#define ARRLEN(arr) sizeof(arr)/sizeof(arr[0])

typedef enum {
    TT_EOF,
    TT_EOL,
    TT_INVALID,
    TT_WORD,
    TT_NUM,
    TT_STRLIT,
    TT_EQ,
    TT_OB,
    TT_CB,
    TT_OCB,
    TT_CCB,
    TT_COMMA,
    TT_EXCLMARK,
    TT_COUNT,
} TokenType;

typedef union {
    char *asStr;
    size_t asNum;
} TokenValue;

typedef struct {
    TokenType type;
    TokenValue value;
} Token;

typedef struct {
    char *data;

    size_t size;
    size_t pos;
} Buffer;

typedef struct {
    FILE *file;
    Buffer *buf;
} Lexer;

char* printablevalue(const Token *t);
void tokenexception(const Token *t);

Lexer lex_init(const char *filepath, Buffer *buf);
void lex_readfile(const Lexer *l);
char lex_peek(const Lexer *l);
void lex_incbuf(const Lexer *l);
char lex_getc(const Lexer *l);
char lex_nextc(const Lexer *l);
bool lex_skipws(const Lexer *l);
bool lex_readword(const Lexer *l, char word[WORD_MAX_SZ]);
bool lex_readstrlit(const Lexer *l, char str[STR_MAX_SZ]);
size_t lex_readnum(const Lexer *l);
Token lex_next(const Lexer *l);
void lex_expect(const Lexer *l, TokenType t);

#endif
