#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include "lexer.h"

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

void addsampleinstance(SampleInstance *si) {
    if (sample_instance_count >= SAMPLE_INSTANCES_CAP) {
        fprintf(stderr, "Error: 2 many samples\n");
        exit(1);
    }
    sample_instances[sample_instance_count] = *si;
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

    if (items != sf_write_float(file, buf, items)) {
        fprintf(stderr, "Error while writing to the file %s: %s\n", filepath, sf_strerror(file));
        exit(1);
    }

    sf_close(file);

    return items;
}

void parse_load_args(FILE *f, Buffer *buf) {
    try_read_char(f, buf, '(');
    char strlit[STR_MAX_SZ] = {0};
    if (readstrlit(f, buf, strlit)) {
        fprintf(stderr, "Error: unexpected end of file while parsing function arguments\n");
        exit(1);
    }
    try_read_char(f, buf, ')');
}
void parse_play_args(FILE *f, Buffer *buf, size_t row) {
    try_read_char(f, buf, '(');
    char word[WORD_MAX_SZ] = {0};
    skipws(f, buf);
    if (readword(f, buf, word)) {
        fprintf(stderr, "Error: unexpected end of file while parsing function arguments\n");
        exit(1);
    }

    Sample *s = NULL;
    for (size_t i = 0; i < sample_count; ++i) {
        if (!strcmp(samples[i].name, word)) {
            s = &samples[i];
            break;
        }
    }
    if (s == NULL) {
        fprintf(stderr, "Error: no sample named %s\n", word);
        exit(1);
    }

    //      row                    BPM
    // --------------- * --------------------- * samples per second * 2 channels per sample
    // 4 rows per beat   60 seconds per minute
    SampleInstance si = {s, ((float)row / 4 * 60 / BPM) * SAMPLE_RATE * 2 };
    addsampleinstance(&si);
    try_read_char(f, buf, ')');
}

void parse_block(FILE *f, Buffer *buf) {
    assert(buf_getc(f, buf) == '{');
    if (BUF_EOF(buf)) {
        fprintf(stderr, "Error: unexpected end of file while parsing music block\n");
        exit(1);
    }
    char word[WORD_MAX_SZ] = {0};
    char c = buf_peek(buf);
    while (c != '\n') {
        if (BUF_EOF(buf)) {
            fprintf(stderr, "Error: unexpected end of file while parsing music block\n");
            exit(1);
        }
        c = buf_nextc(f, buf);
    }
    size_t row = 0;
    while (c != '}') {
        while (c != '\n') {
            while (c == '\t' || c == ' ') { c = buf_nextc(f, buf); }
            memset(word, 0, WORD_MAX_SZ);
            if (readword(f, buf, word)) {
                fprintf(stderr, "Error: unexpected end of file while parsing music block\n");
                exit(1);
            }
            Func func = str_to_func(word);
            switch (func) {
                case Play:
                    parse_play_args(f, buf, row - 1);
                    c = buf_peek(buf);
                    break;
                default:
                    fprintf(stderr, "Error: the only function allowed in music block is play function\n");
                    exit(1);
                    break;
            }
        }
        row++;
        c = buf_nextc(f, buf);
    }
    buf->pos++;
}

void parse(FILE *f) {
    Buffer buf;
    char data[BUF_SZ + 1] = {0};
    buf.data = data;
    buf.size = BUF_SZ;
    buf.pos  = 0;
    char word[WORD_MAX_SZ] = {0};
    char strlit[STR_MAX_SZ] = {0};
    readfile(f, &buf);
    while(buf.pos < buf.size) {
        if (skipws(f, &buf)) break;
        char c = buf_peek(&buf);
        if (c == '{') {
            parse_block(f, &buf);
            if (skipws(f, &buf)) break;
        }
        memset(word, 0, WORD_MAX_SZ);
        if (readword(f, &buf, word)) break;
        Func func = str_to_func(word);
        switch (func) {
            case Load:
                parse_load_args(f, &buf);
                break;
            case Play:
                fprintf(stderr, "Error: play function should only be in music blocks\n");
                exit(1);
                break;
            default:
                try_read_char(f, &buf, '=');
                memset(strlit, 0, STR_MAX_SZ);
                readstrlit(f, &buf, strlit);
                SF_INFO sfinfo;
                sfinfo.format = 0;
                SNDFILE *file = sf_open(strlit, SFM_READ, &sfinfo);
                if (file == NULL) {
                    fprintf(stderr, "Error while opening the file %s: %s\n", strlit, sf_strerror(file));
                    exit(1);
                }
                size_t items = sfinfo.frames * sfinfo.channels;
                Frame *frame_buf = (Frame*) calloc(items, sizeof(Frame));
                if (items != sf_read_float(file, frame_buf, items)) {
                    fprintf(stderr, "Error while reading from the file %s: %s\n", strlit, sf_strerror(file));
                    exit(1);
                }
                sf_close(file);
                size_t sample_name_len = strlen(word);
                Sample s = {"", frame_buf, items};
                strcpy(s.name, word);
                addsample(s);
                break;
        }
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

// void printbuf(const Buffer *buf) {
//     for (size_t i = 0; i < buf->size; ++i) {
//         if (i == buf->pos) {
//             printf("\033[4m");
//         }
//         printf("%c", buf->data[i]);
//         if (i == buf->pos) {
//             printf("\033[0m");
//         }
//     }
//     printf("\n");
// }
