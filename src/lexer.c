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

char* printablevalue(const Token *t) {
    if (t->type == TT_EOF) {
        return "<End of file>";
    } else if (t->type == TT_EOL) {
        return "<End of line>";
    } else if (t->type == TT_STRLIT) {
        char *value = (char*)malloc(strlen(t->value.asStr) + 3);
        value[0] = '\0';
        strcat(value, "\"");
        strcat(value, t->value.asStr);
        strcat(value, "\"");
        return value;
    } else if (t->type == TT_NUM) {
        int l = snprintf(NULL, 0, "%zu", t->value.asNum);
        assert(l >= 0);
        char *str = calloc(l + 1, sizeof(char));
        sprintf(str, "%zu", t->value.asNum);
        return str;
    }
    return t->value.asStr;
}

void tokenexception(const Token *t) {
    fprintf(stderr, "Error: unexpected token %s\n", printablevalue(t));
    exit(1);
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
    while((isalnum(c) || c == '_') && !BUF_EOF(l->buf)) {
        word[j++] = c;
        if (j >= WORD_MAX_SZ) {
            fprintf(stderr, "Error: word is too big for a keyword: %s\n", word);
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

size_t lex_readnum(const Lexer *l) {
    size_t num = 0;
    char c = lex_peek(l);
    while(isdigit(c) && !BUF_EOF(l->buf)) {
        uint8_t digit = c-48;
        if (num > (SIZE_MAX - digit) / 10) {
            fprintf(stderr, "Error: too big of a number\n");
            exit(1);
        }
        num *= 10;
        num += digit;
        c = lex_getc(l);
    }
    return num;
}

Token lex_next(const Lexer *l) {
    Token t = {0};
    if (lex_skipws(l)) return t;
    char c = lex_peek(l);
    TokenType lit_tt = literaltokens[(uint8_t) c];
    if (lit_tt != 0) {
        t.value.asStr = (char*)calloc(2, sizeof(char));
        t.value.asStr[0] = c;
        t.type = lit_tt;

        lex_incbuf(l);
        // printf("%s, %s\n", token_type_names[t.type], printablevalue(&t));
        return t;
    }
    if (isdigit(c)) {
        size_t num = lex_readnum(l);
        t.type = TT_NUM;
        t.value.asNum = num;
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
            t.value.asStr = strdup(strlit);
            break;
        default:
            char word[WORD_MAX_SZ] = {0};
            if (lex_readword(l, word)) {
                t.type = TT_WORD;
            } else {
                t.type = TT_INVALID;
                lex_incbuf(l);
            }
            t.value.asStr = strdup(word);
            break;
    }
    // printf("%s, %s\n", token_type_names[t.type], printablevalue(&t));
    return t;
}

void lex_expect(const Lexer *l, TokenType t) {
    Token got = lex_next(l);
    if (got.type != t) {
        fprintf(stderr, "Error: expected %s but got %s\n", token_type_names[t], printablevalue(&got));
        exit(1);
    }
}
