#include <string.h>

#include "parser.h"
#include "audio.h"

Func strtofunc(const Token *t) {
    if (t->type == TT_NUM) return FUNC_UNKNOWN;
    const char *str = t->value.asStr;
    if (!strcmp(str, "load")) return FUNC_LOAD;
    else if (!strcmp(str, "play")) return FUNC_PLAY;
    else if (!strcmp(str, "add_to_sequence")) return FUNC_ADDPAT;
    else if (!strcmp(str, "set_bpm")) return FUNC_BPM;
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
                break;
        }
    } while (t.type != TT_CB);
    return args;
}

void parse_block(Lexer *l, const char *name) {
    lex_expect(l, TT_EOL);
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
        Func func = strtofunc(&t);
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
                addsampleinstance(argt.value.asStr, &p, row);
                break;
            case FUNC_BPM:
                args = parse_args(l);
                if (args.count > 1) {
                    fprintf(stderr, "Error: too many arguments\n");
                    exit(1);
                }
                argstoks = args.items[0];
                if (argstoks.count > 1) {
                    tokenexception(&argstoks.items[1]);
                }
                argt = argstoks.items[0];
                if (argt.type != TT_NUM) {
                    tokenexception(&argstoks.items[0]);
                }
                addbpmchange(argt.value.asNum, &p, row);
                break;
            default:
                if (t.type != TT_WORD) {
                    fprintf(stderr, "Error: expected sample name or play function. But got: %s\n", printablevalue(&t));
                    exit(1);
                }
                addsampleinstance(t.value.asStr, &p, row);
                break;
        }
        t = lex_next(l);
    }
    p.rows = row;
    addpattern(&p, name);
}

void parse_declaration(Lexer *l, const Token *t) {
    if (t->type != TT_WORD) {
        tokenexception(t);
    }
    lex_expect(l, TT_EQ);
    Token value = lex_next(l);
    Func func = strtofunc(&value);
    if (func == FUNC_LOAD) {
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
        if (argt.type != TT_STRLIT) {
            tokenexception(&argstoks.items[0]);
        }
        loadsample(argt.value.asStr, t->value.asStr);
    } else if (func == FUNC_UNKNOWN) {
        lex_expect(l, TT_OCB);
        parse_block(l, t->value.asStr);
    } else {
        tokenexception(t);
    }
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
    while (t.type != TT_EOF) {
        switch (t.type) {
            case TT_OCB:
                parse_block(&l, NULL);
                break;
            case TT_WORD:
                Func f = strtofunc(&t);
                if (f == FUNC_ADDPAT) {
                    Args args = parse_args(&l);
                    for (size_t i = 0; i < args.count; ++i) {
                        Tokens arg = args.items[i];
                        if (arg.count != 1) {
                            fprintf(stderr, "Error when parsing `add_to_sequence()` function arguments\n");
                            exit(1);
                        }
                        Token argt = arg.items[0];
                        if (argt.type != TT_WORD) {
                            fprintf(stderr, "Error when parsing `add_to_sequence()` function arguments\n");
                            exit(1);
                        }
                        addtosequence(argt.value.asStr);
                    }
                } else if (f == FUNC_UNKNOWN) {
                    parse_declaration(&l, &t);
                } else {
                    fprintf(stderr, "Error: unexpected function: %s\n", printablevalue(&t));
                    exit(1);
                }
            case TT_EOL:
                break;
            default:
                tokenexception(&t);
        }
        t = lex_next(&l);
    }
}
