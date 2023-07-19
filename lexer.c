#include "lexer.h"

static char *token_type_names[TT_COUNT] = {
    [TT_EOF] = "end of file",
    [TT_EOL] = "newline",
    [TT_INVALID] = "invalid token",
    [TT_WORD] = "word",
    [TT_STRLIT] = "string literal",
    [TT_EQ] = "equal sign",
    [TT_OB] = "opening bracket",
    [TT_CB] = "closing bracket",
    [TT_OCB] = "opening curly bracket",
    [TT_CCB] = "closing curly bracket",
    [TT_COMMA] = "comma",
};

// Seems like it zero-initializes all the other elements since
// it's a global variable and I hope this is not UB
TokenType literaltokens[256] = {
    ['\n'] = TT_EOL,
    ['='] = TT_EQ,
    ['('] = TT_OB,
    [')'] = TT_CB,
    ['{'] = TT_OCB,
    ['}'] = TT_CCB,
    [','] = TT_COMMA,
};

char* printable_value(const Token *t) {
    if (t->type == TT_EOF) {
        return "<End of file>";
    } else if (t->type == TT_EOL) {
        return "<End of line>";
    }
    return t->value;
}

Lexer lex_init(const char *filepath, Buffer *buf) {
    Lexer l = { .buf = buf };

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error while opening the file %s: %s\n", filepath, strerror(errno));
        exit(1);
    }
    l.file = file;

    l.buf = buf;
    lex_readfile(&l);

    return l;
}

void lex_readfile(const Lexer *l) {
    size_t bytes_read = fread(l->buf->data, 1, BUF_SZ, l->file);
    l->buf->pos = 0;
    l->buf->size = bytes_read;
    if (bytes_read < BUF_SZ && !feof(l->file)) {
        fprintf(stderr, "Error while reading from the file: %s\n", strerror(errno));
        exit(1);
    }
}

char lex_peek(const Lexer *l) {
    return l->buf->data[l->buf->pos];
}

void lex_incbuf(const Lexer *l) {
    l->buf->pos++;
    if (l->buf->pos >= BUF_SZ) {
        lex_readfile(l);
    }
}

char lex_getc(const Lexer *l) {
    char c = lex_peek(l);
    lex_incbuf(l);
    return c;
}
char lex_nextc(const Lexer *l) {
    lex_incbuf(l);
    return lex_peek(l);
}

bool lex_skipws(const Lexer *l) {
    if (BUF_EOF(l->buf)) return true;
    char c = lex_peek(l);
    while(isspace(c) && c != '\n') {
        c = lex_nextc(l);
        if (BUF_EOF(l->buf)) return true;
    }
    return false;
}

bool lex_readword(const Lexer *l, char word[WORD_MAX_SZ]) {
    size_t j = 0;
    char c = lex_peek(l);
    while(isalnum(c)) {
        word[j++] = c;
        if (j >= WORD_MAX_SZ) {
            fprintf(stderr, "Error: word is too big for a keyword\n");
            exit(1);
        }
        c = lex_nextc(l);
    }
    if (j == 0) {
        word[0] = c;
        return false;
    }
    return true;
}
bool lex_readstrlit(const Lexer *l, char str[STR_MAX_SZ]) {
    size_t j = 0;
    char c = lex_getc(l);
    while(c != '"') {
        str[j++] = c;
        if (j >= STR_MAX_SZ) {
            fprintf(stderr, "Error: word is too big for a keyword\n");
            exit(1);
        }
        if (BUF_EOF(l->buf)) return false;
        c = lex_getc(l);
    }
    return true;
}

Token lex_next(const Lexer *l) {
    Token t = {0};
    if (lex_skipws(l)) return t;
    char c = lex_peek(l);
    TokenType lit_tt = literaltokens[(uint8_t) c];
    if (lit_tt != 0) {
        t.value = (char*)calloc(2, sizeof(char));
        t.value[0] = c;
        t.type = lit_tt;

        lex_incbuf(l);
        // printf("%s, %s\n", token_type_names[t.type], printable_value(&t));
        return t;
    }
    switch(c) {
        case '"':
            lex_getc(l);
            char strlit[STR_MAX_SZ] = {0};
            if (lex_readstrlit(l, strlit)) {
                t.type = TT_STRLIT;
            } else {
                t.type = TT_INVALID;
            }
            t.value = strdup(strlit);
            break;
        default:
            char word[WORD_MAX_SZ] = {0};
            if (lex_readword(l, word)) {
                t.type = TT_WORD;
            } else {
                t.type = TT_INVALID;
                lex_incbuf(l);
            }
            t.value = strdup(word);
            break;
    }
    // printf("%s, %s\n", token_type_names[t.type], printable_value(&t));
    return t;
}

void lex_expect(const Lexer *l, TokenType t) {
    Token got = lex_next(l);
    if (got.type != t) {
        fprintf(stderr, "Error: expected %s but got %s\n", token_type_names[t], printable_value(&got));
        exit(1);
    }
}
