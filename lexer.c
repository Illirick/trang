#include "lexer.h"

Func str_to_func(char *str) {
    if (!strcmp(str, "load")) return Load;
    else if (!strcmp(str, "play")) return Play;
    return -1;
}

void readfile(FILE *f, Buffer *buf) {
    size_t bytes_read = fread(buf->data, 1, BUF_SZ, f);
    buf->pos = 0;
    buf->size = bytes_read;
    if (bytes_read < BUF_SZ && !feof(f)) {
        fprintf(stderr, "Error while reading from the file: %s\n", strerror(errno));
        exit(1);
    }
}

char buf_peek(const Buffer *buf) {
    return buf->data[buf->pos];
}

char buf_getc(FILE *f, Buffer *buf) {
    char c = buf_peek(buf);
    buf->pos++;
    if (buf->pos >= BUF_SZ) {
        readfile(f, buf);
    }
    return c;
}

char buf_nextc(FILE *f, Buffer *buf) {
    buf->pos++;
    if (buf->pos >= BUF_SZ) {
        readfile(f, buf);
    }
    return buf_peek(buf);
}

bool skipws(FILE *f, Buffer *buf) {
    if (BUF_EOF(buf)) return true;
    char c = buf_peek(buf);
    while(isspace(c)) {
        c = buf_nextc(f, buf);
        if (BUF_EOF(buf)) return true;
    }
    return false;
}

void try_read_char(FILE *f, Buffer *buf, char c) {
    if (skipws(f, buf)) {
        fprintf(stderr, "Error: unexpected end of file, expected '%c'\n", c);
        exit(1);
    }
    char gotc = buf_getc(f, buf);
    if (c != gotc) {
        fprintf(stderr, "Error: expected '%c' but got '%c'\n", c, gotc);
        exit(1);
    }
}

bool readword(FILE *f, Buffer *buf, char word[WORD_MAX_SZ]) {
    size_t j = 0;
    char c = buf_peek(buf);
    while(isalnum(c)) {
        word[j] = c;
        j++;
        if (j >= WORD_MAX_SZ) {
            fprintf(stderr, "Error: word is too big for a keyword\n");
            exit(1);
        }
        if (BUF_EOF(buf)) return true;
        c = buf_nextc(f, buf);
    }
    if (j == 0) {
        fprintf(stderr, "Error: expected a word but got '%c'\n", c);
        exit(1);
    }
    printf("Word: %s\n", word);
    return false;
}

bool readstrlit(FILE *f, Buffer *buf, char strlit[STR_MAX_SZ]) {
    try_read_char(f, buf, '"');
    size_t j = 0;
    char c = buf_getc(f, buf);
    while(c != '"') {
        strlit[j] = c;
        j++;
        if (j >= WORD_MAX_SZ) {
            fprintf(stderr, "Error: string literal is too big\n");
            exit(1);
        }
        if (BUF_EOF(buf)) {
            fprintf(stderr, "Error: unexpected end of file while parsing string literal\n");
            exit(1);
        }
        c = buf_getc(f, buf);
    }
    printf("String literal: \"%s\"\n", strlit);
    return false;
}

