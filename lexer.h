#ifndef LEXER_H_
#define LEXER_H_

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

typedef enum {
    Load,
    Play,
    FUNC_COUNT
} Func;


#define WORD_MAX_SZ 64
#define STR_MAX_SZ 256

#define BUF_SZ 1024

typedef struct {
    char *data;
    size_t size;
    size_t pos;
} Buffer;

#define BUF_EOF(buf) ((buf)->pos >= (buf)->size)

Func str_to_func(char *str);
void readfile(FILE *f, Buffer *buf);
char buf_peek(const Buffer *buf);
char buf_getc(FILE *f, Buffer *buf);
char buf_nextc(FILE *f, Buffer *buf);
bool skipws(FILE *f, Buffer *buf);
void try_read_char(FILE *f, Buffer *buf, char c);;
bool readword(FILE *f, Buffer *buf, char word[WORD_MAX_SZ]);
bool readstrlit(FILE *f, Buffer *buf, char strlit[STR_MAX_SZ]);

#endif
