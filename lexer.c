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
};


char* printable_value(const Token *t) {
    if (t->type == TT_EOF) {
        return "<End of file>";
    } else if (t->type == TT_EOL) {
        return "<End of line>";
    }
    return t->value;
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
    while(isspace(c) && c != '\n') {
        c = buf_nextc(f, buf);
        if (BUF_EOF(buf)) return true;
    }
    return false;
}

bool readword(FILE *f, Buffer *buf, char word[WORD_MAX_SZ]) {
    size_t j = 0;
    char c = buf_peek(buf);
    while(isalnum(c)) {
        word[j++] = c;
        if (j >= WORD_MAX_SZ) {
            fprintf(stderr, "Error: word is too big for a keyword\n");
            exit(1);
        }
        // shouldn't happen
        // if (BUF_EOF(buf)) return false;
        c = buf_nextc(f, buf);
    }
    if (j == 0) {
        word[0] = c;
        return false;
    }
    return true;
}
char* readstrlit(FILE *f, Buffer *buf) {
    char *strlit = (char*)malloc(0);
    size_t start = buf->pos, size = 0, len;
    while (1) {
        for (size_t i = buf->pos; i < BUF_SZ; ++i) {
            if (BUF_EOF(buf)) {
                return NULL;
            }
            switch (buf_getc(f, buf)) {
                case '"':
                    len = i - start;
                    size += len;
                    strlit = reallocarray(strlit, size + 1, sizeof(char));
                    memcpy(strlit, buf->data + start, len);
                    strlit[size] = '\0';
                    return strlit;
                case '\n':
                    //Allowed for now
                case '\\':
                    // Unimplemented
                default:
            }
        }
        len = BUF_SZ - start;
        size += len;
        strlit = reallocarray(strlit, size, sizeof(char));
        memcpy(strlit, buf->data + start, len);
        start = 0;
    }
    // Unreachable but yea
    return NULL;
}

Token lex_next(FILE *f, Buffer *buf) {
    Token t = {0};
    if (skipws(f, buf)) return t;
    char c = buf_peek(buf);
    switch(c) {
        case '\n':
            t.type = TT_EOL;
            t.value = "\n";
            buf_getc(f, buf);
            break;
        case '=':
            t.type = TT_EQ;
            t.value = "=";
            buf_getc(f, buf);
            break;
        case '(':
            t.type = TT_OB;
            t.value = "(";
            buf_getc(f, buf);
            break;
        case ')':
            t.type = TT_CB;
            t.value = ")";
            buf_getc(f, buf);
            break;
        case '{':
            t.type = TT_OCB;
            t.value = "{";
            buf_getc(f, buf);
            break;
        case '}':
            t.type = TT_CCB;
            t.value = "}";
            buf_getc(f, buf);
            break;
        case '"':
            buf_getc(f, buf);
            char *strlit = readstrlit(f, buf);
            if (strlit) {
                t.type = TT_STRLIT;
            } else {
                t.type = TT_INVALID;
            }
            t.value = strdup(strlit);
            break;
        default:
            char word[WORD_MAX_SZ] = {0};
            if (readword(f, buf, word)) {
                t.type = TT_WORD;
            } else {
                t.type = TT_INVALID;
                buf_getc(f, buf);
            }
            t.value = strdup(word);
            break;
    }
    // printf("%s, %s\n", token_type_names[t.type], printable_value(&t));
    return t;
}

void lex_expect(FILE *f, Buffer *buf, TokenType t) {
    Token got = lex_next(f, buf);
    if (got.type != t) {
        fprintf(stderr, "Error: expected %s but got %s\n", token_type_names[t], printable_value(&got));
        exit(1);
    }
}
