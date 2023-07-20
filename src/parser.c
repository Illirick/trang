#include "parser.h"

Func strtofunc(const char *str) {
    if (!strcmp(str, "load")) return FUNC_LOAD;
    else if (!strcmp(str, "play")) return FUNC_PLAY;
    return FUNC_UNKNOWN;
}

Args parse_args(Lexer *l) {
    lex_expect(l, TT_OB);

    Args args = {0};
    Tokens tokens = {0};

    Token t;
    do {
        t = lex_next(l);
        switch(t.type) {
            case TT_INVALID:
            case TT_EOF:
                tokenexception(&t);
                break;
            case TT_COMMA:
                DA_APPEND(&args, tokens);
                tokens.count = 0;
                tokens.capacity = 0;
                break;
            case TT_CB:
                DA_APPEND(&args, tokens);
                break;
            default:
                DA_APPEND(&tokens, t);
        }
    } while (t.type != TT_CB);
    return args;
}

void parse_block(Lexer *l) {
    Token t = lex_next(l);
    size_t row = 0;
    Pattern p = {0};
    while (t.type != TT_CCB) {
        if (t.type == TT_EOF) {
            fprintf(stderr, "Error: unexpected end of file while parsing music block\n");
            exit(1);
        } else if (t.type == TT_EOL) {
            row ++;
            t = lex_next(l);
            continue;
        }
        Func func = strtofunc(t.value);
        switch (func) {
            case FUNC_PLAY:
                Args args = parse_args(l);
                if (args.count > 1) {
                    fprintf(stderr, "Error: too many arguments\n");
                    exit(1);
                }
                Tokens argstoks = args.items[0];
                if (argstoks.count > 1) {
                    tokenexception(&argstoks.items[1]);
                }
                Token argt = argstoks.items[0];
                if (argt.type != TT_WORD) {
                    tokenexception(&argstoks.items[0]);
                }
                addsampleinstance(argt.value, &p, row - 1);
                break;
            default:
                fprintf(stderr, "Error: the only function allowed in music block is play function. But got: %s\n", printablevalue(&t));
                exit(1);
                break;
        }
        t = lex_next(l);
    }
    p.rows = row - 1;
    extern Patterns patterns;
    DA_APPEND(&patterns, p);
}

void parse(const char *filepath) {
    char data[BUF_SZ + 1] = {0};

    Buffer buf = {
        .data = data,
        .size = BUF_SZ,
        .pos  = 0,
    };

    Lexer l = lex_init(filepath, &buf);

    Token t = lex_next(&l);
    Func func;
    while (t.type != TT_EOF) {
        switch (t.type) {
            case TT_OCB:
                parse_block(&l);
                break;
            case TT_WORD:
                func = strtofunc(t.value);
                if (func != FUNC_UNKNOWN) {
                    fprintf(stderr, "Error: unexpected function\n");
                    exit(1);
                }
                lex_expect(&l, TT_EQ);
                Token value = lex_next(&l);
                func = strtofunc(value.value);
                if (func == FUNC_LOAD) {
                    Args args = parse_args(&l);
                    if (args.count > 1) {
                        fprintf(stderr, "Error: too many arguments\n");
                        exit(1);
                    }
                    Tokens argstoks = args.items[0];
                    if (argstoks.count > 1) {
                        tokenexception(&argstoks.items[1]);
                    }
                    Token argt = argstoks.items[0];
                    if (argt.type != TT_STRLIT) {
                        tokenexception(&argstoks.items[0]);
                    }
                    loadsample(argt.value, t.value);
                } else if (func == FUNC_UNKNOWN) {
                } else {
                    tokenexception(&t);
                }
            case TT_EOL:
                break;
            default:
                tokenexception(&t);
        }
        t = lex_next(&l);
    }
}
