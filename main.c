#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include "lexer.h"
#include "audio.h"
//#include "parser.h"

typedef enum {
    UnknownFunction,
    Load,
    Play,
    FUNC_COUNT,
} Func;

Func str_to_func(char *str) {
    if (!strcmp(str, "load")) return Load;
    else if (!strcmp(str, "play")) return Play;
    return UnknownFunction;
}

#define DA_INIT_CAP 128

#define DA_APPEND(da, item) do {\
    if ((da).count >= (da).capacity) {\
        if ((da).capacity == 0) {\
            (da).capacity = DA_INIT_CAP;\
            (da).items = malloc(DA_INIT_CAP);\
        } else {\
            (da).capacity *= 2;\
            (da).items = realloc((da).items, (da).capacity * sizeof(*(da).items));\
        }\
        assert((da).items != NULL);\
    }\
    (da).items[(da).count++] = item;\
} while (0)

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

Args parse_args(FILE *f, Buffer *buf) {
    lex_expect(f, buf, TT_OB);

    Args args = {0};
    Tokens tokens = {0};

    Token t;
    do {
        t = lex_next(f, buf);
        switch(t.type) {
            case TT_INVALID:
            case TT_EOF:
                fprintf(stderr, "Error: unexpected token %s", printable_value(&t));
                exit(1);
            case TT_COMMA:
                DA_APPEND(args, tokens);
                tokens.count = 0;
                tokens.capacity = 0;
                break;
            case TT_CB:
                DA_APPEND(args, tokens);
                break;
            default:
                DA_APPEND(tokens, t);
        }
    } while (t.type != TT_CB);
    return args;
}

void parse_block(FILE *f, Buffer *buf) {
    Token t = lex_next(f, buf);
    size_t row = 0;

    while (t.type != TT_CCB) {
        if (t.type == TT_EOF) {
            fprintf(stderr, "Error: unexpected end of file while parsing music block\n");
            exit(1);
        } else if (t.type == TT_EOL) {
            row ++;
            t = lex_next(f, buf);
            continue;
        }
        Func func = str_to_func(t.value);
        switch (func) {
            case Play:
                Args args = parse_args(f, buf);
                if (args.count > 1) {
                    fprintf(stderr, "Error: too many arguments\n");
                    exit(1);
                }
                Tokens argstoks = args.items[0];
                if (argstoks.count > 1) {
                    fprintf(stderr, "Error: unexpected token %s\n", printable_value(&argstoks.items[1]));
                    exit(1);
                }
                Token argt = argstoks.items[0];
                if (argt.type != TT_WORD) {
                    fprintf(stderr, "Error: unexpected token %s\n", printable_value(&argstoks.items[0]));
                    exit(1);
                }
                addsampleinstance(argt.value, row - 1);
                break;
            default:
                fprintf(stderr, "Error: the only function allowed in music block is play function. But got: %s\n", printable_value(&t));
                exit(1);
                break;
        }
        t = lex_next(f, buf);
    }
}

void parse(FILE *f) {
    char data[BUF_SZ + 1] = {0};

    Buffer buf;
    buf.data = data;
    buf.size = BUF_SZ;
    buf.pos  = 0;

    readfile(f, &buf);

    Token t = lex_next(f, &buf);
    Func func;
    while (t.type != TT_EOF) {
        switch (t.type) {
            case TT_OCB:
                parse_block(f, &buf);
                break;
            case TT_WORD:
                func = str_to_func(t.value);
                if (func != UnknownFunction) {
                    fprintf(stderr, "Error: unexpected function\n");
                    exit(1);
                }
                lex_expect(f, &buf, TT_EQ);
                Token value = lex_next(f, &buf);
                func = str_to_func(value.value);
                if (func == Load) {
                    Args args = parse_args(f, &buf);
                    if (args.count > 1) {
                        fprintf(stderr, "Error: too many arguments\n");
                        exit(1);
                    }
                    Tokens argstoks = args.items[0];
                    if (argstoks.count > 1) {
                        fprintf(stderr, "Error: unexpected token %s\n", printable_value(&argstoks.items[1]));
                        exit(1);
                    }
                    Token argt = argstoks.items[0];
                    if (argt.type != TT_STRLIT) {
                        fprintf(stderr, "Error: unexpected token %s\n", printable_value(&argstoks.items[0]));
                        exit(1);
                    }
                    load_sample(argt.value, t.value);
                } else if (func == UnknownFunction) {
                } else {
                    fprintf(stderr, "Error: unexpected token %s\n", printable_value(&t));
                    exit(1);
                }
            case TT_EOL:
                break;
            default:
                fprintf(stderr, "Error: unexpected token %s\n", printable_value(&t));
                exit(1);
        }
        t = lex_next(f, &buf);
    }
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: expected 1 command line arguments but got none\n");
        exit(1);
    }
    char *filepath = argv[1];
    FILE *prog_file = fopen(filepath, "r");
    if (prog_file == NULL) {
        fprintf(stderr, "Error while opening the file %s: %s\n", filepath, strerror(errno));
        exit(1);
    }
    parse(prog_file);
    save_audio("out.wav");
    return 0;
}
