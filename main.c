#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include "lexer.h"
//#include "parser.h"

#include <sndfile.h>

#define SAMPLE_RATE       44100
#define BPM               80

typedef float Frame;

#define SAMPLE_CAP 1024
#define SAMPLE_INSTANCES_CAP 1024

typedef struct {
    char name[WORD_MAX_SZ];
    Frame *frames;
    size_t count;
} Sample;

typedef struct {
    Sample *sample;
    size_t pos;
} SampleInstance;

Sample samples[SAMPLE_CAP];
size_t sample_count = 0;

SampleInstance sample_instances[SAMPLE_INSTANCES_CAP];
size_t sample_instance_count = 0;

void addsample(Sample sample) {
    if (sample_count >= SAMPLE_CAP) {
        fprintf(stderr, "Error: 2 many samples\n");
        exit(1);
    }
    samples[sample_count] = sample;
    sample_count++;
}

void addsampleinstance(char *sample_name, size_t row) {
    if (sample_instance_count >= SAMPLE_INSTANCES_CAP) {
        fprintf(stderr, "Error: 2 many samples\n");
        exit(1);
    }
    Sample *s = NULL;

    for (size_t i = 0; i < sample_count; ++i) {
        if (!strcmp(samples[i].name, sample_name)) {
            s = &samples[i];
            break;
        }
    }
    if (s == NULL) {
        fprintf(stderr, "Error: no sample named %s\n", sample_name);
        exit(1);
    }

    //      row          60 seconds per minute
    // --------------- * --------------------- * samples per second * 2 samples per channel
    // 4 rows per beat     beats per minute
    SampleInstance si = {s, ((float)row / 4 * 60 / BPM) * SAMPLE_RATE * 2 };
    sample_instances[sample_instance_count] = si;
    sample_instance_count++;
}

float sin_sound(float i, float freq, float volume, float samplerate) {
    return sinf(i * M_PI * 2 * freq / samplerate) * volume * (0xFFFF / 2 - 1);
}

float raise_pitch(float base, float semitones) {
    return base * pow(2, 1/semitones);
}

float add_sounds(float s1, float s2) {
    return ((double)s1 - (0xFFFF / 2 - 1) + (double)s2 - (0xFFFF / 2 - 1)) / 2 + (0xFFFF / 2 - 1);
}

size_t save_audio(char *filepath) {
    if (sample_instance_count == 0) {
        return 0;
    }
    SF_INFO sfinfo;
    sfinfo.samplerate = SAMPLE_RATE;
    sfinfo.channels = 2;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_FILE;
    SNDFILE *file = sf_open(filepath, SFM_WRITE, &sfinfo);
    if (file == NULL) {
        fprintf(stderr, "Error while opening the file %s: %s\n", filepath, sf_strerror(file));
        exit(1);
    }

    SampleInstance *latest_sample = &sample_instances[0];
    for (size_t i = 1; i < sample_instance_count; ++i) {
        SampleInstance *si = &sample_instances[i];
        if (si->pos > latest_sample->pos) {
            latest_sample = si;
        }
    }
    size_t items = latest_sample->pos + latest_sample->sample->count;
    if (items % 2 != 0) items ++;
    Frame *buf = (Frame*)malloc(sizeof(Frame) * items);

    for (size_t i = 0; i < sample_instance_count; ++i) {
        SampleInstance *si = &sample_instances[i];
        for (size_t i = 0; i < si->sample->count; ++i) {
            buf[si->pos + i] = add_sounds(buf[si->pos + i], si->sample->frames[i]);
        }
    }

    if (buf == NULL) {
        fprintf(stderr, "Error while allocation memory for the final audio: %s\n", strerror(errno));
        exit(1);
    }

    if ((int) items != sf_write_float(file, buf, items)) {
        fprintf(stderr, "Error while writing to the file %s: %s\n", filepath, sf_strerror(file));
        exit(1);
    }

    sf_close(file);

    return items;
}

Sample* str_to_sample(char *str) {
    for (size_t i = 0; i < sample_count; ++i) {
        if (!strcmp(samples[i].name, str)) {
            return &samples[i];
        }
    }
    return NULL;
}

void load_sample(char *path, char *name) {
    SF_INFO sfinfo;
    sfinfo.format = 0;
    SNDFILE *file = sf_open(path, SFM_READ, &sfinfo);
    if (file == NULL) {
        fprintf(stderr, "Error while opening the file %s: %s\n", path, sf_strerror(file));
        exit(1);
    }
    size_t items = sfinfo.frames * sfinfo.channels;
    Frame *frame_buf = (Frame*) calloc(items, sizeof(Frame));
    if ((int)items != sf_read_float(file, frame_buf, items)) {
        fprintf(stderr, "Error while reading from the file %s: %s\n", path, sf_strerror(file));
        exit(1);
    }
    sf_close(file);


    Sample *s = str_to_sample(name);
    if (s) {
        s->frames = frame_buf;
    } else {
        //printf("Adding sample %s\n", name);
        Sample sample = {.frames = frame_buf, .count = items};
        strcpy(sample.name, name);
        addsample(sample);
    }
}

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
                if (args.count < 0) {
                    fprintf(stderr, "Error: too few arguments\n");
                    exit(1);
                }
                if (args.count > 1) {
                    fprintf(stderr, "Error: too many arguments\n");
                    exit(1);
                }
                Tokens argstoks = args.items[0];
                if (argstoks.count < 0) {
                    fprintf(stderr, "Error: too few argument expected\n");
                    exit(1);
                }
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
                    if (args.count < 0) {
                        fprintf(stderr, "Error: too few arguments\n");
                        exit(1);
                    }
                    if (args.count > 1) {
                        fprintf(stderr, "Error: too many arguments\n");
                        exit(1);
                    }
                    Tokens argstoks = args.items[0];
                    if (argstoks.count < 0) {
                        fprintf(stderr, "Error: too few argument expected\n");
                        exit(1);
                    }
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
