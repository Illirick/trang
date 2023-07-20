#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <sndfile.h>

#define SAMPLE_RATE       44100
#define BPM               140

typedef float Frame;

#define SAMPLE_CAP 1024
#define SAMPLE_INSTANCE_CAP 1024

#define WORD_MAX_SZ 64

#define DA_INIT_CAP 128

#define DA_APPEND(da, item) do {\
    if ((da)->count >= (da)->capacity) {\
        if ((da)->capacity == 0) {\
            (da)->capacity = DA_INIT_CAP;\
            (da)->items = malloc(DA_INIT_CAP);\
        } else {\
            (da)->capacity *= 2;\
            (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));\
        }\
        assert((da)->items != NULL);\
    }\
    (da)->items[(da)->count++] = item;\
} while (0)

typedef struct {
    char name[WORD_MAX_SZ];
    Frame *frames;
    size_t count;
} Sample;

typedef struct {
    Sample *sample;
    size_t pos;
} SampleInstance;

typedef struct {
    Sample *items;
    size_t count;
    size_t capacity;
} Samples;

typedef struct {
    SampleInstance *items;
    size_t count;
    size_t capacity;
    size_t rows;
} Pattern;

typedef struct {
    Pattern *items;
    size_t count;
    size_t capacity;
} Patterns;

size_t framecount(const Pattern *p);
size_t rowtoframecount(size_t row);

void addsample(Sample sample);
void addsampleinstance(const char *sample_name, Pattern *pat, size_t row);
// float sinsound(float i, float freq, float volume, float samplerate); not needed yet
// float raisepitch(float base, float semitones);
// float addsounds(float s1, float s2);
size_t saveaudio(const char *filepath);
Sample* strtosample(const char *str);
void loadsample(const char *path, const char *name);

#endif